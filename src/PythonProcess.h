#pragma once

#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QTimer>

class PythonProcess : public QObject
{
    Q_OBJECT
public:
    explicit PythonProcess(QObject *parent = nullptr);

    void start();
    void stop();
    quint16 port() const { return m_port; }
    bool isHealthy() const { return m_healthy; }

signals:
    void ready(quint16 port);
    void failed(const QString &reason);
    void restarting();

private slots:
    void onProcessStateChanged(QProcess::ProcessState state);
    void onHealthCheck();

private:
    QProcess *m_proc;
    QNetworkAccessManager *m_net;
    QTimer *m_healthTimer;
    quint16 m_port = 0;
    int m_healthAttempts = 0;
    int m_restartAttempts = 0;
    bool m_userStopped = false;
    bool m_healthy = false;

    quint16 findFreePort();
    QString findPython();
    void startHealthPolling();
};
