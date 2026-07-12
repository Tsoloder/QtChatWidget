#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonArray>

class QNetworkAccessManager;
class QNetworkReply;

class LLMClient : public QObject
{
    Q_OBJECT
public:
    enum ApiType {
        OpenAI     = 0,
        Anthropic  = 1
    };

    struct Config {
        ApiType apiType = OpenAI;
        QString apiUrl;
        QString apiKey;
        QString modelId;
    };

    explicit LLMClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url) { m_baseUrl = url; }
    QString baseUrl() const { return m_baseUrl; }

    // 暴露内部的 QNetworkAccessManager 供 SessionListPanel 等组件复用
    QNetworkAccessManager *networkManager() const { return m_net; }

    void setConfig(const Config &config) { m_config = config; }
    Config config() const { return m_config; }

    void sendMessage(const QString &userMessage,
                     const QString &systemPrompt,
                     const QString &sessionId,
                     const QJsonArray &selectedSkills = QJsonArray(),
                     const QStringList &skillRoots = QStringList(),
                     const QString &writableSkillRoot = QString());
    void postConfig(const Config &cfg);
    void abortStream();

    bool isBusy() const { return m_currentReply != nullptr; }
    bool hadToolCalls() const { return m_hadToolCalls; }

signals:
    void streamStarted();
    void streamChunk(const QString &delta);
    void streamFinished(const QString &fullText);
    void toolCallReceived(const QString &name, const QString &args);
    void toolResultReceived(const QString &name, const QString &result);
    void tokenUsageReceived(int total);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onReplyFinished();

private:
    QNetworkAccessManager *m_net;
    Config m_config;
    QString m_baseUrl;
    QNetworkReply *m_currentReply = nullptr;
    QByteArray m_sseBuffer;
    QString m_fullText;
    bool m_hadToolCalls = false;

    void resetStreamState();
    void parseSSEBlock(const QByteArray &block);
    QMap<QString, QString> parseEventBlock(const QByteArray &block);
};
