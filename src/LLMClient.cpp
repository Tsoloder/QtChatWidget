#include "LLMClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>
#include <QTimer>

LLMClient::LLMClient(QObject *parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_streaming(false)
{
}

void LLMClient::setConfig(const Config &config)
{
    m_config = config;
}

static QNetworkRequest prepareRequest(const QString &url)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    return request;
}

void LLMClient::clearHistory()
{
    m_history = QJsonArray();
}

QByteArray LLMClient::buildOpenAIRequest(const QString &userMessage, const QString &systemPrompt)
{
    // Append user message to history
    QJsonObject userMsg;
    userMsg[QStringLiteral("role")] = QStringLiteral("user");
    userMsg[QStringLiteral("content")] = userMessage;
    m_history.append(userMsg);

    QJsonObject payload;
    payload[QStringLiteral("model")] = m_config.modelId;
    payload[QStringLiteral("max_tokens")] = 8192;
    payload[QStringLiteral("stream")] = true;

    // Build messages array: system + history
    QJsonArray messages;
    if (!systemPrompt.isEmpty()) {
        QJsonObject sysMsg;
        sysMsg[QStringLiteral("role")] = QStringLiteral("system");
        sysMsg[QStringLiteral("content")] = systemPrompt;
        messages.append(sysMsg);
    }
    for (const QJsonValue &v : m_history) {
        messages.append(v);
    }
    payload[QStringLiteral("messages")] = messages;

    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

QByteArray LLMClient::buildAnthropicRequest(const QString &userMessage, const QString &systemPrompt)
{
    QJsonObject payload;
    payload[QStringLiteral("model")] = m_config.modelId;
    payload[QStringLiteral("max_tokens")] = 8192;
    payload[QStringLiteral("stream")] = true;

    if (!systemPrompt.isEmpty()) {
        payload[QStringLiteral("system")] = systemPrompt;
    }

    QJsonArray messages;
    QJsonObject userMsg;
    userMsg[QStringLiteral("role")] = QStringLiteral("user");
    userMsg[QStringLiteral("content")] = userMessage;
    messages.append(userMsg);
    payload[QStringLiteral("messages")] = messages;

    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

QString LLMClient::parseOpenAIReply(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return QString();

    QJsonObject obj = doc.object();

    if (obj.contains(QStringLiteral("error"))) {
        QJsonValue errVal = obj.value(QStringLiteral("error"));
        if (errVal.isObject()) {
            return QString();
        }
    }

    if (obj.contains(QStringLiteral("choices"))) {
        QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
        if (!choices.isEmpty()) {
            QJsonObject msg = choices[0].toObject().value(QStringLiteral("message")).toObject();
            QString text = msg.value(QStringLiteral("content")).toString();
            QString reasoning = msg.value(QStringLiteral("reasoning_content")).toString();

            if (text.isEmpty() && !reasoning.isEmpty()) {
                text = reasoning;
            }

            if (text.isEmpty())
                return QString();

            QJsonObject assistantMsg;
            assistantMsg[QStringLiteral("role")] = QStringLiteral("assistant");
            assistantMsg[QStringLiteral("content")] = text;
            m_history.append(assistantMsg);

            return text;
        }
    }
    return QString();
}

QString LLMClient::parseAnthropicReply(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return QString();

    QJsonObject obj = doc.object();
    QString text;

    if (obj.contains(QStringLiteral("content")) && obj.value(QStringLiteral("content")).isArray()) {
        QJsonArray content = obj.value(QStringLiteral("content")).toArray();
        for (const QJsonValue &v : content) {
            if (v.isObject()) {
                QJsonObject co = v.toObject();
                if (co.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                    text += co.value(QStringLiteral("text")).toString();
                }
            }
        }
    }
    return text;
}

void LLMClient::processSSEData(const QByteArray &chunk)
{
    m_streamBuffer += QString::fromUtf8(chunk);

    // Process complete lines
    int newlineIdx;
    while ((newlineIdx = m_streamBuffer.indexOf(QLatin1Char('\n'))) >= 0) {
        QString line = m_streamBuffer.left(newlineIdx).trimmed();
        m_streamBuffer = m_streamBuffer.mid(newlineIdx + 1);

        if (line.isEmpty())
            continue;
        if (!line.startsWith(QStringLiteral("data: ")))
            continue;

        QString data = line.mid(6).trimmed();  // remove "data: " prefix
        if (data == QStringLiteral("[DONE]"))
            return;

        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject())
            continue;

        QJsonObject obj = doc.object();
        QString delta;

        // OpenAI stream format: choices[0].delta.content
        if (obj.contains(QStringLiteral("choices"))) {
            QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
            if (!choices.isEmpty()) {
                QJsonObject deltaObj = choices[0].toObject().value(QStringLiteral("delta")).toObject();
                delta = deltaObj.value(QStringLiteral("content")).toString();
                // Also handle reasoning_content delta for reasoning models
                if (delta.isEmpty()) {
                    delta = deltaObj.value(QStringLiteral("reasoning_content")).toString();
                }
            }
        }
        // Anthropic stream format: delta.text (for content_block_delta)
        else if (obj.contains(QStringLiteral("delta"))) {
            QJsonObject deltaObj = obj.value(QStringLiteral("delta")).toObject();
            delta = deltaObj.value(QStringLiteral("text")).toString();
            if (delta.isEmpty()) {
                delta = deltaObj.value(QStringLiteral("thinking")).toString();
            }
        }

        if (!delta.isEmpty()) {
            m_streamAccumulator += delta;
            emit streamChunk(delta);
        }
    }
}

void LLMClient::onReadyRead()
{
    if (!m_currentReply || !m_streaming)
        return;
    QByteArray data = m_currentReply->readAll();
    processSSEData(data);
}

void LLMClient::sendMessage(const QString &userMessage, const QString &systemPrompt)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    if (m_config.apiUrl.isEmpty() || m_config.apiKey.isEmpty() || m_config.modelId.isEmpty()) {
        emit errorOccurred(QStringLiteral("LLM config is incomplete"));
        return;
    }

    // Reset streaming state
    m_streaming = true;
    m_streamBuffer.clear();
    m_streamAccumulator.clear();

    // Determine endpoint URL and build request body
    QString endpoint;
    QByteArray body;
    QNetworkRequest request;

    if (m_config.apiType == OpenAI) {
        endpoint = m_config.apiUrl;
        if (!endpoint.endsWith(QStringLiteral("/v1/chat/completions"))) {
            if (endpoint.endsWith(QLatin1Char('/')))
                endpoint += QStringLiteral("v1/chat/completions");
            else
                endpoint += QStringLiteral("/v1/chat/completions");
        }
        body = buildOpenAIRequest(userMessage, systemPrompt);
        request = prepareRequest(endpoint);
        request.setRawHeader("Authorization", QByteArray("Bearer ") + m_config.apiKey.toUtf8());
    } else {
        endpoint = m_config.apiUrl;
        if (!endpoint.endsWith(QStringLiteral("/v1/messages"))) {
            if (endpoint.endsWith(QLatin1Char('/')))
                endpoint += QStringLiteral("v1/messages");
            else
                endpoint += QStringLiteral("/v1/messages");
        }
        body = buildAnthropicRequest(userMessage, systemPrompt);
        request = prepareRequest(endpoint);
        request.setRawHeader("x-api-key", m_config.apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    }

    emit streamStarted();

    // Debug: log the request to verify Skill prompt is included
    qDebug() << "=== LLM Request ===" << endpoint;
    qDebug() << "Body:" << body.left(2000);

    m_currentReply = m_net->post(request, body);
    connect(m_currentReply, &QNetworkReply::readyRead, this, &LLMClient::onReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &LLMClient::onReplyFinished);
}

void LLMClient::onReplyFinished()
{
    if (!m_currentReply)
        return;

    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        QByteArray body = reply->readAll();
        if (!body.isEmpty())
            err += QStringLiteral("\n") + QString::fromUtf8(body);
        m_streaming = false;
        emit errorOccurred(err);
        return;
    }

    if (m_streaming) {
        // Process any remaining data in the buffer
        QByteArray remaining = reply->readAll();
        if (!remaining.isEmpty())
            processSSEData(remaining);

        m_streaming = false;

        // Add to history
        if (!m_streamAccumulator.isEmpty()) {
            QJsonObject assistantMsg;
            assistantMsg[QStringLiteral("role")] = QStringLiteral("assistant");
            assistantMsg[QStringLiteral("content")] = m_streamAccumulator;
            m_history.append(assistantMsg);
            emit streamFinished(m_streamAccumulator);
        } else {
            // Fallback: try non-streaming parse
            QByteArray data = remaining;
            if (data.isEmpty())
                data = reply->readAll();
            // Re-read from what we got (already consumed by readyRead, use empty)
            // If accumulator is empty, something went wrong
            emit errorOccurred(QStringLiteral("No content received from stream"));
        }
    } else {
        // Non-streaming fallback
        QByteArray data = reply->readAll();
        QString text;
        if (m_config.apiType == OpenAI)
            text = parseOpenAIReply(data);
        else
            text = parseAnthropicReply(data);
        if (text.isEmpty()) {
            text = parseOpenAIReply(data);
            if (text.isEmpty())
                text = parseAnthropicReply(data);
            if (text.isEmpty())
                text = QString::fromUtf8(data);
        }
        emit replyReceived(text);
    }
}

void LLMClient::routeSkill(const QString &userMessage, const QString &catalogPrompt)
{
    if (catalogPrompt.isEmpty()) {
        emit noSkillNeeded();
        return;
    }

    // Build a non-streaming request with tool definition for use_skill
    QJsonObject payload;
    payload[QStringLiteral("model")] = m_config.modelId;
    payload[QStringLiteral("max_tokens")] = 2048;  // reasoning models need more tokens
    payload[QStringLiteral("stream")] = false;

    // Define the use_skill tool
    QJsonObject paramProps;
    QJsonObject skillIdProp;
    skillIdProp[QStringLiteral("type")] = QStringLiteral("string");
    skillIdProp[QStringLiteral("description")] = QStringLiteral("The id of the skill to use");
    paramProps[QStringLiteral("skill_id")] = skillIdProp;

    QJsonObject parameters;
    parameters[QStringLiteral("type")] = QStringLiteral("object");
    parameters[QStringLiteral("properties")] = paramProps;
    parameters[QStringLiteral("required")] = QJsonArray({QStringLiteral("skill_id")});

    QJsonObject tool;
    tool[QStringLiteral("type")] = QStringLiteral("function");
    QJsonObject func;
    func[QStringLiteral("name")] = QStringLiteral("use_skill");
    func[QStringLiteral("description")] = QStringLiteral("Use a Skill to handle the user's request. "
        "Call this if the user's message matches one of the available Skills. "
        "Do NOT call this if no Skill is relevant.");
    func[QStringLiteral("parameters")] = parameters;
    tool[QStringLiteral("function")] = func;

    QJsonArray tools;
    tools.append(tool);
    payload[QStringLiteral("tools")] = tools;
    payload[QStringLiteral("tool_choice")] = QStringLiteral("auto");

    // Build messages
    QJsonArray messages;
    QJsonObject sysMsg;
    sysMsg[QStringLiteral("role")] = QStringLiteral("system");
    sysMsg[QStringLiteral("content")] = catalogPrompt;
    messages.append(sysMsg);

    QJsonObject userMsg;
    userMsg[QStringLiteral("role")] = QStringLiteral("user");
    userMsg[QStringLiteral("content")] = userMessage;
    messages.append(userMsg);

    payload[QStringLiteral("messages")] = messages;

    // Build request
    QString endpoint = m_config.apiUrl;
    if (!endpoint.endsWith(QStringLiteral("/v1/chat/completions"))) {
        if (endpoint.endsWith(QLatin1Char('/')))
            endpoint += QStringLiteral("v1/chat/completions");
        else
            endpoint += QStringLiteral("/v1/chat/completions");
    }

    QUrl routeUrl(endpoint);
    QNetworkRequest request(routeUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_config.apiKey.toUtf8());

    QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    qDebug() << "=== Skill Routing Request ===" << endpoint;
    qDebug() << "Body:" << body.left(1000);

    QNetworkReply *routeReply = m_net->post(request, body);
    routeReply->setProperty("userMessage", QVariant(userMessage));
    // Timeout: if routing takes too long, fall back to keyword matching
    QTimer::singleShot(15000, routeReply, [routeReply]() {
        if (routeReply->isRunning()) {
            routeReply->abort();
        }
    });
    connect(routeReply, &QNetworkReply::finished, this, &LLMClient::onRouteReplyFinished);
}

void LLMClient::onRouteReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Route error:" << reply->errorString() << reply->readAll();
        emit noSkillNeeded();
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "Route response:" << data.left(500);

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit noSkillNeeded();
        return;
    }

    QJsonObject obj = doc.object();
    QString skillId;

    // Parse tool_calls from response
    if (obj.contains(QStringLiteral("choices"))) {
        QJsonArray choices = obj[QStringLiteral("choices")].toArray();
        if (!choices.isEmpty()) {
            QJsonObject message = choices[0].toObject()[QStringLiteral("message")].toObject();

            if (message.contains(QStringLiteral("tool_calls"))) {
                QJsonArray toolCalls = message[QStringLiteral("tool_calls")].toArray();
                if (!toolCalls.isEmpty()) {
                    QJsonObject call = toolCalls[0].toObject();
                    QJsonObject func = call[QStringLiteral("function")].toObject();
                    if (func[QStringLiteral("name")].toString() == QStringLiteral("use_skill")) {
                        QJsonDocument argsDoc = QJsonDocument::fromJson(
                            func[QStringLiteral("arguments")].toString().toUtf8());
                        if (argsDoc.isObject()) {
                            skillId = argsDoc.object()[QStringLiteral("skill_id")].toString();
                        }
                    }
                }
            }
        }
    }

    if (!skillId.isEmpty()) {
        qDebug() << "Model selected Skill:" << skillId;
        emit skillRouted(skillId);
    } else {
        qDebug() << "Model decided no Skill needed";
        emit noSkillNeeded();
    }
}
