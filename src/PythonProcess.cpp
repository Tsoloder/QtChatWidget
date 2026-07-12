#include "PythonProcess.h"

#include <QCoreApplication>
#include <QTcpServer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>

PythonProcess::PythonProcess(QObject *parent)
    : QObject(parent)
    , m_proc(new QProcess(this))
    , m_net(new QNetworkAccessManager(this))
    , m_healthTimer(new QTimer(this))
{
    m_healthTimer->setInterval(500);
    connect(m_healthTimer, &QTimer::timeout, this, &PythonProcess::onHealthCheck);
    connect(m_proc, &QProcess::stateChanged, this, &PythonProcess::onProcessStateChanged);
}

quint16 PythonProcess::findFreePort()
{
    QTcpServer server;
    if (server.listen(QHostAddress::LocalHost, 0))
        return server.serverPort();
    return 8000;
}

QString PythonProcess::findPython()
{
    QSettings settings(QStringLiteral("QtChatWidget"), QStringLiteral("settings"));
    QString configured = settings.value(QStringLiteral("pythonPath")).toString();
    if (!configured.isEmpty() && QFileInfo::exists(configured))
        return configured;

    QStringList candidates;
#ifdef Q_OS_WIN
    candidates << QStringLiteral("python") << QStringLiteral("python3") << QStringLiteral("py");
#else
    candidates << QStringLiteral("python3") << QStringLiteral("python");
#endif

    for (const QString &cmd : candidates) {
        QProcess which;
        which.start(cmd, QStringList{QStringLiteral("--version")});
        which.waitForFinished(3000);
        if (which.exitCode() == 0)
            return cmd;
    }
    return QStringLiteral("python");
}

void PythonProcess::start()
{
    m_userStopped = false;
    m_healthy = false;
    m_healthAttempts = 0;

    m_port = findFreePort();

    QString pythonPath = findPython();
    QString appDir = QCoreApplication::applicationDirPath();
    // 候选路径：1) 与 exe 同目录的 py/agent（部署态）
    //          2) 上一级（build/py/agent）
    //          3) 上两级（build/Release → 项目根/py/agent，开发态）
    QString scriptPath = appDir + QStringLiteral("/py/agent/app.py");
    if (!QFileInfo::exists(scriptPath))
        scriptPath = QDir(appDir).filePath(QStringLiteral("../py/agent/app.py"));
    if (!QFileInfo::exists(scriptPath))
        scriptPath = QDir(appDir).filePath(QStringLiteral("../../py/agent/app.py"));

    m_proc->setWorkingDirectory(appDir);
    QStringList args;
    args << scriptPath << QStringLiteral("--port") << QString::number(m_port);
    m_proc->start(pythonPath, args);

    startHealthPolling();
}

void PythonProcess::stop()
{
    m_userStopped = true;
    m_healthTimer->stop();
    if (m_proc->state() != QProcess::NotRunning) {
        m_proc->terminate();
        if (!m_proc->waitForFinished(3000))
            m_proc->kill();
    }
}

void PythonProcess::startHealthPolling()
{
    m_healthAttempts = 0;
    m_healthTimer->start();
}

void PythonProcess::onHealthCheck()
{
    m_healthAttempts++;
    if (m_healthAttempts > 20) {
        m_healthTimer->stop();
        if (!m_healthy)
            emit failed(QStringLiteral("Python backend health check timeout"));
        return;
    }

    QNetworkRequest req(QUrl(QString(QStringLiteral("http://127.0.0.1:%1/health")).arg(m_port)));
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        // 防止多个在途健康检查同时返回成功时重复 emit ready
        if (m_healthy)
            return;
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject() && doc.object().value(QStringLiteral("status")).toString() == QStringLiteral("ok")) {
                m_healthy = true;
                m_healthTimer->stop();
                m_restartAttempts = 0;
                emit ready(m_port);
            }
        }
    });
}

void PythonProcess::onProcessStateChanged(QProcess::ProcessState state)
{
    if (state == QProcess::NotRunning && !m_userStopped) {
        m_healthy = false;
        if (m_restartAttempts < 3) {
            m_restartAttempts++;
            emit restarting();
            QTimer::singleShot(1000, this, [this]() { start(); });
        } else {
            emit failed(QStringLiteral("Python crashed repeatedly"));
        }
    }
}
