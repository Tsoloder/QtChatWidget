#include "MainWindow.h"
#include "ChatWidget.h"
#include "LLMClient.h"
#include "ReplyParser.h"
#include "SkillManager.h"
#include "Skill.h"
#include "SettingsDialog.h"

#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chat(new ChatWidget(this))
    , m_llm(new LLMClient(this))
{
    setWindowTitle(QStringLiteral("AI Chat Assistant // Qt 5.12 + VS2022"));
    resize(940, 720);
    setCentralWidget(m_chat);

    // Load config from file (with hardcoded defaults as fallback)
    LLMClient::Config defaults;
    defaults.apiType = LLMClient::OpenAI;  // xiaomimimo uses OpenAI-compatible API
    defaults.apiUrl = QStringLiteral("https://api.xiaomimimo.com");
    defaults.apiKey = QStringLiteral("sk-czjm2c9pepam0vigfbpqzpqkmbx0biw6kg0rev1tkcjuupwn");
    defaults.modelId = QStringLiteral("mimo-v2.5-pro");
    m_config = SettingsDialog::loadConfig(defaults);
    m_llm->setConfig(m_config);

    connect(m_chat, &ChatWidget::messageSentWithSkill, this,
        [this](const QString &text, const QString &sysPrompt) {
            m_llm->sendMessage(text, sysPrompt);
        });

    // Phase 1: No manual Skill — route via model to decide
    connect(m_chat, &ChatWidget::messageSent, this,
        [this](const QString &text) {
            m_pendingMessage = text;
            m_chat->setStatusText(QStringLiteral("STATUS: SELECTING SKILL..."));
            m_llm->routeSkill(text, m_chat->skillManager()->catalogPrompt());
        });

    // Phase 2a: Model selected a Skill — send with Skill prompt
    connect(m_llm, &LLMClient::skillRouted, this,
        [this](const QString &skillId) {
            Skill skill = m_chat->skillManager()->skillById(skillId);
            if (skill.isValid()) {
                // Show auto-load notification
                ContentSegments notifSegs;
                ContentSegment notif;
                notif.type = ContentSegment::Text;
                notif.text = QStringLiteral("Skill selected: **%1** — %2")
                    .arg(skill.name, skill.description);
                notifSegs.append(notif);
                m_chat->addBubble(ChatBubble::Assistant, notifSegs);

                m_chat->skillManager()->recordUsage(skillId);
                m_chat->setStatusText(QStringLiteral("STATUS: STREAMING..."));
                m_llm->sendMessage(m_pendingMessage, skill.resolvedSystemPrompt());
            } else {
                // Skill ID not found — fall back to normal
                m_chat->setStatusText(QStringLiteral("STATUS: STREAMING..."));
                m_llm->sendMessage(m_pendingMessage);
            }
            m_pendingMessage.clear();
        });

    // Phase 2b: Model decided no Skill needed
    // Fallback: if model doesn't support tool_calls, do client-side keyword matching
    connect(m_llm, &LLMClient::noSkillNeeded, this,
        [this]() {
            QList<Skill> suggestions = m_chat->skillManager()->suggestRelevant(m_pendingMessage, 1);
            if (!suggestions.isEmpty()) {
                const Skill &fallback = suggestions.first();
                // Check match strength (same threshold as before)
                QString lowerText = m_pendingMessage.toLower();
                int matchScore = 0;
                if (lowerText.contains(fallback.name.toLower())) matchScore += 10;
                if (lowerText.contains(fallback.id.toLower())) matchScore += 8;
                for (const QString &alias : fallback.aliases)
                    if (lowerText.contains(alias.toLower())) matchScore += 8;
                for (const QString &tag : fallback.tags)
                    if (lowerText.contains(tag.toLower())) matchScore += 4;

                if (matchScore >= 4) {
                    ContentSegments notifSegs;
                    ContentSegment notif;
                    notif.type = ContentSegment::Text;
                    notif.text = QStringLiteral("Skill selected: **%1** — %2")
                        .arg(fallback.name, fallback.description);
                    notifSegs.append(notif);
                    m_chat->addBubble(ChatBubble::Assistant, notifSegs);

                    m_chat->skillManager()->recordUsage(fallback.id);
                    m_chat->setStatusText(QStringLiteral("STATUS: STREAMING..."));
                    m_llm->sendMessage(m_pendingMessage, fallback.resolvedSystemPrompt());
                    m_pendingMessage.clear();
                    return;
                }
            }
            m_chat->setStatusText(QStringLiteral("STATUS: STREAMING..."));
            m_llm->sendMessage(m_pendingMessage);
            m_pendingMessage.clear();
        });

    connect(m_llm, &LLMClient::replyReceived, this,
        [this](const QString &raw) {
            ContentSegments segs = parseAssistantReply(raw);
            m_chat->addBubble(ChatBubble::Assistant, segs);
            m_chat->setStatusText(QStringLiteral("STATUS: READY"));
        });

    // Streaming signals
    connect(m_llm, &LLMClient::streamStarted, this,
        [this]() {
            m_chat->setStatusText(QStringLiteral("STATUS: STREAMING..."));
        });

    connect(m_llm, &LLMClient::streamChunk, m_chat, &ChatWidget::appendStreamChunk);

    connect(m_llm, &LLMClient::streamFinished, this,
        [this](const QString &fullText) {
            Q_UNUSED(fullText);
            m_chat->finishStream();
        });

    connect(m_llm, &LLMClient::errorOccurred, this,
        [this](const QString &err) {
            ContentSegments segs;
            ContentSegment s;
            s.type = ContentSegment::Text;
            s.text = QStringLiteral("<b>Error:</b> ") + err;
            segs.append(s);
            m_chat->addBubble(ChatBubble::Assistant, segs);
            m_chat->setStatusText(QStringLiteral("STATUS: ERROR"));
        });

    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    QAction *settingsAct = fileMenu->addAction(QStringLiteral("Settings..."));
    settingsAct->setShortcut(QStringLiteral("Ctrl+,"));
    connect(settingsAct, &QAction::triggered, this, &MainWindow::onSettings);
    fileMenu->addSeparator();
    QAction *installAct = fileMenu->addAction(QStringLiteral("Install Skill..."));
    connect(installAct, &QAction::triggered, this, &MainWindow::onInstallSkill);
    fileMenu->addSeparator();
    QAction *quitAct = fileMenu->addAction(QStringLiteral("Quit"));
    connect(quitAct, &QAction::triggered, this, &QMainWindow::close);
}

void MainWindow::onSettings()
{
    SettingsDialog dlg(m_config, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_config = dlg.config();
        m_llm->setConfig(m_config);
        SettingsDialog::saveConfig(m_config);
    }
}

void MainWindow::onInstallSkill()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        QStringLiteral("Install Skill"),
        QString(),
        QStringLiteral("Skill Files (*.md);;All Files (*)"));
    if (filePath.isEmpty())
        return;

    if (m_chat->skillManager()->installFromMarkdownFile(filePath)) {
        QMessageBox::information(this, QStringLiteral("Skill Installed"),
            QStringLiteral("Skill installed successfully."));
    } else {
        QMessageBox::warning(this, QStringLiteral("Install Failed"),
            QStringLiteral("Failed to install the skill. Please check the file format."));
    }
}
