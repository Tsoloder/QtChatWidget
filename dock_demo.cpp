// DockWidget 集成演示：模拟一个宿主主窗口，把 ChatWidget 嵌入到右侧 Dock。
// 仅用于截图演示，不是正式产物。

#include <QApplication>
#include <QMainWindow>
#include <QDockWidget>
#include <QTextEdit>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QTimer>
#include <QEventLoop>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QCoreApplication>
#include "ChatWidget.h"
#include "ChatBubble.h"
#include "ContentSegment.h"
#include "Theme.h"

static void seedConversation(ChatWidget *chat)
{
    // 模拟一段对话：用户问 -> 助手回（带代码 + 选项 + 参数表）
    ContentSegments user;
    ContentSegment u;
    u.type = ContentSegment::Text;
    u.text = QStringLiteral("帮我重构这个函数，让它更高效");
    user.append(u);
    chat->addBubble(ChatBubble::User, user);

    ContentSegments segs;
    ContentSegment t;
    t.type = ContentSegment::Text;
    t.text = QStringLiteral("好的，我帮你重构。这是优化后的实现：");
    segs.append(t);

    ContentSegment c;
    c.type = ContentSegment::Code;
    c.language = "cpp";
    c.text = QStringLiteral(
        "// 优化版：用 std::accumulate 替代手写循环\n"
        "#include <numeric>\n"
        "void MainWindow::doWork() {\n"
        "    int vals[] = {0,1,2,3,4,5,6,7,8,9};\n"
        "    m_count = std::accumulate(std::begin(vals),\n"
        "                              std::end(vals), 0);\n"
        "}");
    segs.append(c);

    ContentSegment o;
    o.type = ContentSegment::Options;
    o.options.append({QStringLiteral("Apply this change"),  QStringLiteral("apply"),      QStringLiteral("primary")});
    o.options.append({QStringLiteral("Explain the code"),   QStringLiteral("explain"),    QStringLiteral("default")});
    o.options.append({QStringLiteral("Suggest alternative"),QStringLiteral("alternative"),QStringLiteral("default")});
    o.options.append({QStringLiteral("Cancel"),             QStringLiteral("cancel"),     QStringLiteral("danger")});
    segs.append(o);

    ContentSegment tp;
    tp.type = ContentSegment::ToolParams;
    tp.toolName = QStringLiteral("edit_file");
    tp.params = {
        {QStringLiteral("path"),        QStringLiteral("目标文件路径"),     QStringLiteral("src/MainWindow.cpp")},
        {QStringLiteral("new_content"), QStringLiteral("新文件内容"),       QStringLiteral("// refactored")},
        {QStringLiteral("backup"),      QStringLiteral("是否备份原文件"),   QStringLiteral("true")},
    };
    segs.append(tp);

    chat->addBubble(ChatBubble::Assistant, segs);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    const ThemeId themes[] = {
        ThemeId::MilitaryTech,
        ThemeId::FutureTechBlue,
        ThemeId::WhiteMinimal,
        ThemeId::OneDarkPro,
        ThemeId::WeChatLight,
    };

    QVector<QImage> cells;
    const int W = 1100, H = 760;

    for (ThemeId id : themes) {
        // ---- 模拟宿主主窗口 ----
        QMainWindow host;
        host.setWindowTitle(QStringLiteral("My IDE - host application"));
        host.resize(W, H);

        // 菜单栏（宿主自己的）
        auto *mb = host.menuBar();
        mb->addMenu(QStringLiteral("File"))->addAction(QStringLiteral("Open"));
        mb->addMenu(QStringLiteral("Edit"))->addAction(QStringLiteral("Undo"));
        mb->addMenu(QStringLiteral("View"));
        mb->addMenu(QStringLiteral("Help"));

        // 工具栏（宿主自己的）
        auto *tb = host.addToolBar(QStringLiteral("main"));
        tb->setMovable(false);
        tb->addAction(QStringLiteral("Build"));
        tb->addAction(QStringLiteral("Run"));
        tb->addSeparator();
        tb->addAction(QStringLiteral("Debug"));

        // 中央部件：模拟代码编辑器（宿主自己的）
        auto *editor = new QTextEdit(&host);
        editor->setPlainText(QStringLiteral(
            "// 这是宿主程序自己的代码编辑器\n"
            "#include <QMainWindow>\n\n"
            "class MyIDE : public QMainWindow {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    explicit MyIDE(QWidget *p = nullptr);\n"
            "private:\n"
            "    ChatWidget *m_chat = nullptr;\n"
            "};"));
        editor->setReadOnly(true);
        host.setCentralWidget(editor);

        // 状态栏（宿主自己的）
        host.statusBar()->showMessage(QStringLiteral("Ready - Ln 9, Col 24"));

        // ---- 把 ChatWidget 嵌入右侧 Dock ----
        auto *dock = new QDockWidget(QStringLiteral("AI Assistant"), &host);
        dock->setObjectName("aiAssistantDock");
        dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

        auto *chat = new ChatWidget(dock);
        chat->setHeaderVisible(false);  // dock 已有标题栏，关掉 ChatWidget 自带头部
        dock->setWidget(chat);
        host.addDockWidget(Qt::RightDockWidgetArea, dock);
        dock->resize(420, H);

        host.show();

        // 等布局完成
        QEventLoop loop;
        QTimer::singleShot(500, &loop, &QEventLoop::quit);
        loop.exec();

        // 先创建对话内容，再切换主题，确保 QSS 应用到已存在的气泡上
        seedConversation(chat);
        chat->setTheme(id);

        // 强制处理所有待处理事件，确保 QSS 完全刷新后再截图
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        chat->repaint();
        host.repaint();
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        QTimer::singleShot(600, &loop, &QEventLoop::quit);
        loop.exec();

        // 截图整个宿主窗口
        QPixmap pix = host.grab();
        cells.append(pix.toImage());
    }

    // 纵向拼接三张
    const int gap = 24;
    const int labelH = 32;
    int totalH = gap;
    for (const auto &img : cells)
        totalH += labelH + img.height() + gap;

    QImage grid(W, totalH, QImage::Format_RGB32);
    grid.fill(QColor(30, 30, 30));

    QPainter gp(&grid);
    gp.setRenderHint(QPainter::Antialiasing);
    QFont f(QStringLiteral("Consolas"), 12, QFont::Bold);
    gp.setFont(f);
    int y = gap;
    const QString names[] = {
        QStringLiteral("MilitaryTech  -  ChatWidget embedded in right QDockWidget"),
        QStringLiteral("FutureTechBlue  -  ChatWidget embedded in right QDockWidget"),
        QStringLiteral("WhiteMinimal  -  ChatWidget embedded in right QDockWidget"),
        QStringLiteral("OneDarkPro  -  ChatWidget embedded in right QDockWidget"),
        QStringLiteral("WeChatLight  -  ChatWidget embedded in right QDockWidget"),
    };
    for (int i = 0; i < cells.size(); ++i) {
        gp.setPen(QColor(200, 220, 255));
        gp.drawText(QRect(gap, y, W - 2 * gap, labelH),
                    Qt::AlignLeft | Qt::AlignVCenter, names[i]);
        y += labelH;
        gp.drawImage(gap, y, cells[i]);
        y += cells[i].height() + gap;
    }
    gp.end();

    const QString path = QStringLiteral("/workspace/screenshot_dock.png");
    grid.save(path, "PNG");
    qInfo("Saved dock demo to %s (%dx%d)", qPrintable(path), grid.width(), grid.height());
    return 0;
}
