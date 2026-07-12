#pragma once

#include <QMainWindow>
#include <QSettings>
#include "ChatWidget.h"
#include "Theme.h"
#include "LLMClient.h"
#include "PythonProcess.h"
#include "SessionListPanel.h"

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
    void onClearConversation();
    void onExportConversation();

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    ChatWidget *m_chat;
    LLMClient *m_llm;
    PythonProcess *m_pyProc;
    SessionListPanel *m_sessionPanel;
    LLMClient::Config m_config;
    QString m_currentSessionId;
    QString m_lastMessage;
    QJsonArray m_lastSelectedSkills;

    void initSession();
    void loadSession(const QString &id);
    void createNewSession();
    bool sessionExists(const QString &id);
    QStringList skillRoots() const;
};
