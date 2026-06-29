#pragma once

#include <QMainWindow>
#include "ChatWidget.h"
#include "Theme.h"
#include "LLMClient.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void setTheme(ThemeId id) { m_chat->setTheme(id); }
    ThemeId currentTheme() const { return m_chat->currentTheme(); }
    ChatWidget *chatWidget() const { return m_chat; }

private slots:
    void onInstallSkill();
    void onSettings();

private:
    ChatWidget *m_chat;
    LLMClient *m_llm;
    LLMClient::Config m_config;
    QString m_pendingMessage;  // message awaiting Skill routing
};
