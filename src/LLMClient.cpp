#include "LLMClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

LLMClient::LLMClient(QObject *parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
}

void LLMClient::resetStreamState()
{
    m_fullText.clear();
    m_hadToolCalls = false;
    m_sseBuffer.clear();
}

void LLMClient::abortStream()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    resetStreamState();
}

void LLMClient::postConfig(const Config &cfg)
{
    if (m_baseUrl.isEmpty())
        return;

    m_config = cfg;

    QJsonObject body;
    body[QStringLiteral("api_type")] = (cfg.apiType == OpenAI)
        ? QStringLiteral("openai") : QStringLiteral("anthropic");
    body[QStringLiteral("api_url")] = cfg.apiUrl;
    body[QStringLiteral("api_key")] = cfg.apiKey;
    body[QStringLiteral("model_id")] = cfg.modelId;

    QNetworkRequest req(QUrl(m_baseUrl + QStringLiteral("/config")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    m_net->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void LLMClient::sendMessage(const QString &userMessage,
                            const QString &systemPrompt,
                            const QString &sessionId,
                            const QJsonArray &selectedSkills,
                            const QStringList &skillRoots,
                            const QString &writableSkillRoot)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    if (m_baseUrl.isEmpty()) {
        emit errorOccurred(QStringLiteral("Python backend not ready"));
        return;
    }

    resetStreamState();

    QJsonObject body;
    body[QStringLiteral("session_id")] = sessionId;
    body[QStringLiteral("message")] = userMessage;
    body[QStringLiteral("system_prompt")] = systemPrompt;
    body[QStringLiteral("selected_skills")] = selectedSkills;
    QJsonArray roots;
    for (const QString &root : skillRoots)
        roots.append(root);
    body[QStringLiteral("skill_roots")] = roots;
    body[QStringLiteral("writable_skill_root")] = writableSkillRoot;

    QNetworkRequest req(QUrl(m_baseUrl + QStringLiteral("/chat/stream")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    emit streamStarted();

    m_currentReply = m_net->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_currentReply, &QNetworkReply::readyRead, this, &LLMClient::onReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &LLMClient::onReplyFinished);
}

void LLMClient::onReadyRead()
{
    if (!m_currentReply)
        return;
    m_sseBuffer += m_currentReply->readAll();

    int idx;
    while ((idx = m_sseBuffer.indexOf("\n\n")) != -1) {
        QByteArray eventBlock = m_sseBuffer.left(idx);
        m_sseBuffer.remove(0, idx + 2);
        parseSSEBlock(eventBlock);
    }
}

QMap<QString, QString> LLMClient::parseEventBlock(const QByteArray &block)
{
    QMap<QString, QString> result;
    QString dataStr;
    for (const auto &line : block.split('\n')) {
        QString lineStr = QString::fromUtf8(line).trimmed();
        if (lineStr.startsWith(QStringLiteral("event: "))) {
            result[QStringLiteral("event")] = lineStr.mid(7).trimmed();
        } else if (lineStr.startsWith(QStringLiteral("data: "))) {
            if (!dataStr.isEmpty())
                dataStr += QStringLiteral("\n");
            dataStr += lineStr.mid(6);
        }
    }
    result[QStringLiteral("data")] = dataStr;
    return result;
}

void LLMClient::parseSSEBlock(const QByteArray &block)
{
    QMap<QString, QString> parsed = parseEventBlock(block);
    QString eventType = parsed.value(QStringLiteral("event"));
    QString dataStr = parsed.value(QStringLiteral("data"));

    if (eventType.isEmpty() || dataStr.isEmpty())
        return;

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(dataStr.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject())
        return;

    QJsonObject data = doc.object();

    if (eventType == QStringLiteral("text_chunk")) {
        QString delta = data[QStringLiteral("delta")].toString();
        m_fullText += delta;
        emit streamChunk(delta);
    } else if (eventType == QStringLiteral("tool_call")) {
        m_hadToolCalls = true;
        m_fullText.clear();
        QString name = data[QStringLiteral("name")].toString();
        QJsonDocument argsDoc(data[QStringLiteral("args")].toObject());
        QString argsStr = QString::fromUtf8(argsDoc.toJson(QJsonDocument::Compact));
        emit toolCallReceived(name, argsStr);
    } else if (eventType == QStringLiteral("tool_result")) {
        QString name = data[QStringLiteral("name")].toString();
        QString result = data[QStringLiteral("result")].toString();
        emit toolResultReceived(name, result);
    } else if (eventType == QStringLiteral("usage")) {
        emit tokenUsageReceived(data[QStringLiteral("total")].toInt());
    } else if (eventType == QStringLiteral("done")) {
        emit streamFinished(m_fullText);
        resetStreamState();
    } else if (eventType == QStringLiteral("error")) {
        emit errorOccurred(data[QStringLiteral("message")].toString());
        resetStreamState();
    }
}

void LLMClient::onReplyFinished()
{
    if (!m_currentReply)
        return;

    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;

    if (reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::OperationCanceledError) {
        QString err = reply->errorString();
        QByteArray body = reply->readAll();
        if (!body.isEmpty())
            err += QStringLiteral("\n") + QString::fromUtf8(body);
        emit errorOccurred(err);
    }

    reply->deleteLater();
}
