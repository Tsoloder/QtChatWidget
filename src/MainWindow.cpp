#include "MainWindow.h"
#include "ChatWidget.h"
#include "InputBar.h"
#include "LLMClient.h"
#include "ReplyParser.h"
#include "SkillManager.h"
#include "Skill.h"
#include "SkillPicker.h"
#include "SettingsDialog.h"

#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCloseEvent>
#include <QEventLoop>
#include <QStatusBar>
#include <QCoreApplication>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chat(new ChatWidget(this))
    , m_llm(new LLMClient(this))
    , m_pyProc(new PythonProcess(this))
    , m_sessionPanel(new SessionListPanel(this))
{
    setWindowTitle(QStringLiteral("AI Chat Assistant // Qt + Python Agent"));
    resize(1140, 720);
    setCentralWidget(m_chat);

    // Load config
    LLMClient::Config defaults;
    defaults.apiType = LLMClient::OpenAI;
    defaults.apiUrl = QStringLiteral("https://api.xiaomimimo.com");
    defaults.apiKey = QStringLiteral("sk-czjm2c9pepam0vigfbpqzpqkmbx0biw6kg0rev1tkcjuupwn");
    defaults.modelId = QStringLiteral("mimo-v2.5-pro");
    m_config = SettingsDialog::loadConfig(defaults);
    m_llm->setConfig(m_config);

    // Session panel：网络管理器要等 Python 后端 ready 后再注入
    addDockWidget(Qt::LeftDockWidgetArea, m_sessionPanel);

    // Python process lifecycle
    connect(m_pyProc, &PythonProcess::ready, this, [this](quint16 port) {
        QString base = QString(QStringLiteral("http://127.0.0.1:%1")).arg(port);
        m_llm->setBaseUrl(base);
        m_llm->postConfig(m_config);
        m_sessionPanel->setBaseUrl(base);
        m_sessionPanel->setNetworkManager(m_llm->networkManager());
        initSession();
        m_sessionPanel->refresh();
        m_chat->setStatusText(QStringLiteral("STATUS: READY"));
    });
    connect(m_pyProc, &PythonProcess::restarting, this, [this]() {
        m_chat->setStatusText(QStringLiteral("STATUS: BACKEND RESTARTING..."));
    });
    connect(m_pyProc, &PythonProcess::failed, this, [this](const QString &err) {
        QMessageBox::critical(this, QStringLiteral("Error"),
            QStringLiteral("Python backend failed: ") + err);
        m_chat->setStatusText(QStringLiteral("STATUS: BACKEND OFFLINE"));
    });

    // Chat signals
    connect(m_chat, &ChatWidget::messageSent, this, [this](const QString &text) {
        m_lastMessage = text;
        m_lastSelectedSkills = QJsonArray();
        m_chat->setInputBusy(true);
        m_chat->setStatusText(QStringLiteral("STATUS: STREAMING..."));
        m_llm->sendMessage(text, QString(), m_currentSessionId,
                           m_lastSelectedSkills, skillRoots(),
                           m_chat->skillManager()->userSkillsDir());
        QSettings(QStringLiteral("QtChatWidget")).setValue(QStringLiteral("lastSessionId"), m_currentSessionId);
    });

    connect(m_chat, &ChatWidget::messageSentWithSkills, this,
        [this](const QString &text, const QJsonArray &selectedSkills) {
            m_lastMessage = text;
            m_lastSelectedSkills = selectedSkills;
            for (const QJsonValue &value : selectedSkills) {
                const QString id = value.toObject().value(QStringLiteral("id")).toString();
                if (!id.isEmpty())
                    m_chat->skillManager()->recordUsage(id);
            }
            m_chat->setInputBusy(true);
            m_chat->setStatusText(QStringLiteral("STATUS: STREAMING..."));
            m_llm->sendMessage(text, QString(), m_currentSessionId,
                               selectedSkills, skillRoots(),
                               m_chat->skillManager()->userSkillsDir());
        });

    // LLM streaming signals
    connect(m_llm, &LLMClient::streamStarted, this, [this]() {
        m_chat->setInputBusy(true);
    });

    connect(m_llm, &LLMClient::streamChunk, m_chat, &ChatWidget::appendStreamChunk);

    connect(m_llm, &LLMClient::toolCallReceived, this, [this](const QString &name, const QString &args) {
        m_chat->appendStreamChunk(QString(QStringLiteral("\n🔧 调用工具 %1(%2)\n")).arg(name, args));
    });

    connect(m_llm, &LLMClient::toolResultReceived, this, [this](const QString &name, const QString &result) {
        Q_UNUSED(name);
        m_chat->appendStreamChunk(QString(QStringLiteral("→ %1\n")).arg(result));
    });

    connect(m_llm, &LLMClient::tokenUsageReceived, this, [this](int total) {
        statusBar()->showMessage(QString(QStringLiteral("Tokens: %1")).arg(total));
    });

    connect(m_llm, &LLMClient::streamFinished, this, [this](const QString &fullText) {
        if (m_llm->hadToolCalls()) {
            m_chat->finishStream();
        } else {
            ContentSegments segs = parseAssistantReply(fullText);
            m_chat->finishStreamWithSegments(segs);
        }
        m_chat->setInputBusy(false);
        m_chat->setStatusText(QStringLiteral("STATUS: READY"));
        m_sessionPanel->refresh();
    });

    connect(m_llm, &LLMClient::errorOccurred, this, [this](const QString &err) {
        m_chat->abortStream();
        m_chat->setInputBusy(false);
        ContentSegments segs;
        ContentSegment s;
        s.type = ContentSegment::Text;
        s.text = QStringLiteral("<b>Error:</b> ") + err;
        segs.append(s);
        ContentSegment opt;
        opt.type = ContentSegment::Options;
        opt.options.append({QStringLiteral("Retry"), QStringLiteral("retry"), QStringLiteral("primary")});
        segs.append(opt);
        m_chat->addBubble(ChatBubble::Assistant, segs);
        m_chat->setStatusText(QStringLiteral("STATUS: ERROR"));
    });

    // Retry button
    connect(m_chat, &ChatWidget::optionSelected, this,
        [this](ChatBubble *bubble, int index, const QString &value) {
            Q_UNUSED(bubble); Q_UNUSED(index);
            if (value == QStringLiteral("retry") && !m_lastMessage.isEmpty()) {
                m_chat->setInputBusy(true);
                m_chat->setStatusText(QStringLiteral("STATUS: STREAMING..."));
                m_llm->sendMessage(m_lastMessage, QString(), m_currentSessionId,
                                   m_lastSelectedSkills, skillRoots(),
                                   m_chat->skillManager()->userSkillsDir());
            }
        });

    // Session panel signals
    connect(m_sessionPanel, &SessionListPanel::sessionSelected, this, [this](const QString &id) {
        loadSession(id);
    });
    connect(m_sessionPanel, &SessionListPanel::newSessionRequested, this, [this]() {
        m_chat->clear();
    });
    connect(m_sessionPanel, &SessionListPanel::deleteSessionRequested, this, [this](const QString &id) {
        if (id == m_currentSessionId) {
            m_chat->clear();
            m_currentSessionId.clear();
        }
    });

    // Theme propagation
    connect(m_chat, &ChatWidget::themeChanged, m_sessionPanel, &SessionListPanel::setTheme);
    connect(m_chat, &ChatWidget::themeChanged, this, [this](ThemeId id) {
        if (m_chat->inputBar() && m_chat->inputBar()->skillPicker())
            m_chat->inputBar()->skillPicker()->setTheme(id);
    });

    // 初始主题：ChatWidget 构造函数未发出 themeChanged，需要手动设置
    m_sessionPanel->setTheme(m_chat->currentTheme());

    // Start Python backend
    m_pyProc->start();
    m_chat->setStatusText(QStringLiteral("STATUS: STARTING BACKEND..."));

    // Menu
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    QAction *settingsAct = fileMenu->addAction(QStringLiteral("Settings..."));
    settingsAct->setShortcut(QStringLiteral("Ctrl+,"));
    connect(settingsAct, &QAction::triggered, this, &MainWindow::onSettings);
    fileMenu->addSeparator();
    QAction *installAct = fileMenu->addAction(QStringLiteral("Install Skill..."));
    connect(installAct, &QAction::triggered, this, &MainWindow::onInstallSkill);
    fileMenu->addSeparator();
    QAction *clearAct = fileMenu->addAction(QStringLiteral("Clear Conversation"));
    clearAct->setShortcut(QStringLiteral("Ctrl+L"));
    connect(clearAct, &QAction::triggered, this, &MainWindow::onClearConversation);
    QAction *exportAct = fileMenu->addAction(QStringLiteral("Export Conversation..."));
    exportAct->setShortcut(QStringLiteral("Ctrl+E"));
    connect(exportAct, &QAction::triggered, this, &MainWindow::onExportConversation);
    fileMenu->addSeparator();
    QAction *quitAct = fileMenu->addAction(QStringLiteral("Quit"));
    connect(quitAct, &QAction::triggered, this, &QMainWindow::close);
}

void MainWindow::initSession()
{
    QSettings settings(QStringLiteral("QtChatWidget"));
    QString lastId = settings.value(QStringLiteral("lastSessionId")).toString();
    if (!lastId.isEmpty() && sessionExists(lastId)) {
        m_currentSessionId = lastId;
        loadSession(lastId);
    } else {
        createNewSession();
    }
}

bool MainWindow::sessionExists(const QString &id)
{
    if (id.isEmpty())
        return false;
    QNetworkAccessManager *net = m_llm->networkManager();
    if (!net)
        return false;

    QNetworkRequest req(QUrl(m_llm->baseUrl() + QStringLiteral("/sessions/") + id));
    QNetworkReply *reply = net->get(req);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool exists = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    return exists;
}

void MainWindow::createNewSession()
{
    QNetworkAccessManager *net = m_llm->networkManager();
    if (!net)
        return;

    QNetworkRequest req(QUrl(m_llm->baseUrl() + QStringLiteral("/sessions")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = net->post(req, QByteArray("{}"));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        m_currentSessionId = doc.object().value(QStringLiteral("id")).toString();
        // 立即持久化，避免 Python 后端重启触发 initSession 时再创建新会话
        QSettings(QStringLiteral("QtChatWidget")).setValue(QStringLiteral("lastSessionId"), m_currentSessionId);
        m_chat->clear();
    }
    reply->deleteLater();
}

void MainWindow::loadSession(const QString &id)
{
    QNetworkAccessManager *net = m_llm->networkManager();
    if (!net)
        return;

    QNetworkRequest req(QUrl(m_llm->baseUrl() + QStringLiteral("/sessions/") + id));
    QNetworkReply *reply = net->get(req);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        m_currentSessionId = id;
        m_chat->loadMessages(obj.value(QStringLiteral("messages")).toArray());
    }
    reply->deleteLater();
}

QStringList MainWindow::skillRoots() const
{
    QStringList roots;
    roots.append(QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("resources/skills")));
    roots.append(m_chat->skillManager()->userSkillsDir());
    roots.removeDuplicates();
    return roots;
}

void MainWindow::onSettings()
{
    SettingsDialog dlg(m_config, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_config = dlg.config();
        m_llm->setConfig(m_config);
        m_llm->postConfig(m_config);
        SettingsDialog::saveConfig(m_config);
    }
}

void MainWindow::onInstallSkill()
{
    QMessageBox choice(this);
    choice.setWindowTitle(QStringLiteral("Install Skill"));
    choice.setText(QStringLiteral("请选择 Skill 来源。完整目录可保留 references、scripts 和 assets。"));
    QPushButton *directoryButton = choice.addButton(QStringLiteral("选择目录"), QMessageBox::AcceptRole);
    QPushButton *fileButton = choice.addButton(QStringLiteral("选择 SKILL.md"), QMessageBox::ActionRole);
    choice.addButton(QMessageBox::Cancel);
    choice.exec();

    bool installed = false;
    if (choice.clickedButton() == directoryButton) {
        const QString dirPath = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Select Skill Directory"));
        if (dirPath.isEmpty())
            return;
        installed = m_chat->skillManager()->installFromDirectory(dirPath);
    } else if (choice.clickedButton() == fileButton) {
        const QString filePath = QFileDialog::getOpenFileName(this,
            QStringLiteral("Install Skill"), QString(),
            QStringLiteral("Skill Files (SKILL.md *.md);;All Files (*)"));
        if (filePath.isEmpty())
            return;
        installed = m_chat->skillManager()->installFromMarkdownFile(filePath);
    } else {
        return;
    }

    if (installed) {
        QMessageBox::information(this, QStringLiteral("Skill Installed"),
            QStringLiteral("Skill installed successfully."));
    } else {
        QMessageBox::warning(this, QStringLiteral("Install Failed"),
            QStringLiteral("Failed to install the skill. Please check the file format."));
    }
}

void MainWindow::onClearConversation()
{
    m_chat->abortStream();
    m_chat->clear();
    m_lastMessage.clear();
    m_chat->setInputBusy(false);
    m_chat->setStatusText(QStringLiteral("STATUS: READY"));

    // Clear session on Python side
    if (!m_currentSessionId.isEmpty()) {
        QNetworkAccessManager *net = m_llm->networkManager();
        if (net) {
            QNetworkRequest req(QUrl(m_llm->baseUrl() + QStringLiteral("/sessions/") + m_currentSessionId + QStringLiteral("/clear")));
            req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            net->post(req, QByteArray("{}"));
        }
    }
}

void MainWindow::onExportConversation()
{
    const QList<ChatBubble*> bubbles = m_chat->bubbles();
    if (bubbles.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Export"),
            QStringLiteral("No messages to export."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this,
        QStringLiteral("Export Conversation"),
        QStringLiteral("conversation"),
        QStringLiteral("Markdown (*.md);;JSON (*.json)"));
    if (path.isEmpty())
        return;

    bool asJson = path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Export Failed"),
            QStringLiteral("Cannot open file for writing."));
        return;
    }

    if (asJson) {
        QJsonArray arr;
        for (ChatBubble *b : bubbles) {
            QJsonObject msg;
            msg[QStringLiteral("role")] = (b->role() == ChatBubble::User)
                ? QStringLiteral("user") : QStringLiteral("assistant");
            QJsonArray segArr;
            for (const ContentSegment &seg : b->segments()) {
                QJsonObject sObj;
                switch (seg.type) {
                case ContentSegment::Text:
                    sObj[QStringLiteral("type")] = QStringLiteral("text");
                    sObj[QStringLiteral("text")] = seg.text;
                    break;
                case ContentSegment::Code:
                    sObj[QStringLiteral("type")] = QStringLiteral("code");
                    sObj[QStringLiteral("language")] = seg.language;
                    sObj[QStringLiteral("text")] = seg.text;
                    break;
                case ContentSegment::Options: {
                    sObj[QStringLiteral("type")] = QStringLiteral("options");
                    QJsonArray opts;
                    for (const auto &o : seg.options) {
                        QJsonObject oo;
                        oo[QStringLiteral("label")] = o.label;
                        oo[QStringLiteral("value")] = o.value;
                        oo[QStringLiteral("style")] = o.style;
                        opts.append(oo);
                    }
                    sObj[QStringLiteral("options")] = opts;
                    break;
                }
                case ContentSegment::ToolApproval:
                    sObj[QStringLiteral("type")] = QStringLiteral("tool_approval");
                    sObj[QStringLiteral("tool")] = seg.toolName;
                    sObj[QStringLiteral("description")] = seg.toolDescription;
                    break;
                case ContentSegment::ToolParams: {
                    sObj[QStringLiteral("type")] = QStringLiteral("tool_params");
                    sObj[QStringLiteral("tool")] = seg.toolName;
                    QJsonArray params;
                    for (const auto &p : seg.params) {
                        QJsonObject po;
                        po[QStringLiteral("name")] = p.name;
                        po[QStringLiteral("description")] = p.description;
                        po[QStringLiteral("value")] = p.value;
                        params.append(po);
                    }
                    sObj[QStringLiteral("params")] = params;
                    break;
                }
                }
                segArr.append(sObj);
            }
            msg[QStringLiteral("segments")] = segArr;
            arr.append(msg);
        }
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    } else {
        QTextStream ts(&file);
        ts.setCodec("UTF-8");
        for (ChatBubble *b : bubbles) {
            ts << (b->role() == ChatBubble::User ? "## User\n" : "## Assistant\n");
            for (const ContentSegment &seg : b->segments()) {
                switch (seg.type) {
                case ContentSegment::Text:
                    ts << seg.text << "\n\n";
                    break;
                case ContentSegment::Code:
                    ts << "```" << seg.language << "\n" << seg.text << "\n```\n\n";
                    break;
                case ContentSegment::Options:
                    for (const auto &o : seg.options)
                        ts << "- [" << o.label << "] -> " << o.value << "\n";
                    ts << "\n";
                    break;
                case ContentSegment::ToolApproval:
                    ts << "**Tool:** " << seg.toolName << " — " << seg.toolDescription << "\n\n";
                    break;
                case ContentSegment::ToolParams:
                    ts << "**Tool params:** " << seg.toolName << "\n";
                    for (const auto &p : seg.params)
                        ts << "  - " << p.name << " = " << p.value << "  (" << p.description << ")\n";
                    ts << "\n";
                    break;
                }
            }
            ts << "---\n\n";
        }
    }
    file.close();
    QMessageBox::information(this, QStringLiteral("Exported"),
        QStringLiteral("Conversation exported to:\n%1").arg(path));
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    m_pyProc->stop();
    QMainWindow::closeEvent(e);
}
