#pragma once

#include <QObject>
#include <QString>
#include <QJsonArray>

class QNetworkAccessManager;
class QNetworkReply;

class LLMClient : public QObject
{
    Q_OBJECT
public:
    enum ApiType {
        OpenAI     = 0,  // OpenAI-compatible (/v1/chat/completions)
        Anthropic  = 1   // Anthropic (/v1/messages)
    };

    struct Config {
        ApiType apiType = Anthropic;
        QString apiUrl;
        QString apiKey;
        QString modelId;
    };

    explicit LLMClient(QObject *parent = nullptr);

    void setConfig(const Config &config);
    Config config() const { return m_config; }

    void sendMessage(const QString &userMessage,
                     const QString &systemPrompt = QString());

    // Route: ask model which Skill to use (non-streaming, quick call)
    void routeSkill(const QString &userMessage, const QString &catalogPrompt);

    void clearHistory();
    bool isBusy() const { return m_currentReply != nullptr; }

signals:
    void replyReceived(const QString &rawReply);
    void errorOccurred(const QString &error);

    // Streaming signals
    void streamStarted();
    void streamChunk(const QString &delta);
    void streamFinished(const QString &fullText);

    // Routing signals
    void skillRouted(const QString &skillId);   // model selected a Skill
    void noSkillNeeded();                        // model decided no Skill needed

private slots:
    void onReplyFinished();
    void onReadyRead();
    void onRouteReplyFinished();

private:
    QByteArray buildOpenAIRequest(const QString &userMessage, const QString &systemPrompt);
    QByteArray buildAnthropicRequest(const QString &userMessage, const QString &systemPrompt);
    QString parseOpenAIReply(const QByteArray &data);
    QString parseAnthropicReply(const QByteArray &data);
    void processSSEData(const QByteArray &chunk);

    QNetworkAccessManager *m_net;
    Config m_config;
    QNetworkReply *m_currentReply;
    QJsonArray m_history;  // conversation history (OpenAI message format)
    bool m_streaming;
    QString m_streamBuffer;  // incomplete SSE line buffer
    QString m_streamAccumulator;  // accumulated full text
};
