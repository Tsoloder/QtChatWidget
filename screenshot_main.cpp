#include <QApplication>
#include <QTimer>
#include <QEventLoop>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "ChatWidget.h"
#include "ChatBubble.h"
#include "ContentSegment.h"
#include "Theme.h"
#include "OptionsWidget.h"
#include "ToolParamsWidget.h"
#include "ReplyParser.h"

// 截图脚本：模拟一条"模型回复"原始字符串（包含正文 + 代码 + 末尾 JSON 块），
// 用 parseAssistantReply 解析后渲染到每个主题，对比 Options + ToolParams 的样式。

static const char *kModelReply = R"REPLY(
我已经帮你写好了优化版本，使用 `std::accumulate` 替代手写循环，更简洁也更安全：

```cpp
// 优化版：用 std::accumulate 替代手写循环
#include <numeric>
void MainWindow::doWork() {
    int vals[] = {0,1,2,3,4,5,6,7,8,9};
    m_count = std::accumulate(std::begin(vals),
                              std::end(vals), 0);
}
```

接下来我需要修改 `src/MainWindow.cpp`，请确认下面的参数，然后选择一个操作：

```json
{
  "tool_params": {
    "tool": "edit_file",
    "params": [
      {"name": "path",        "description": "目标文件路径",       "value": "src/MainWindow.cpp"},
      {"name": "new_content", "description": "新文件内容",         "value": "// refactored with std::accumulate"},
      {"name": "backup",      "description": "是否备份原文件",     "value": "true"},
      {"name": "timeout",     "description": "执行超时(秒)",       "value": "30"}
    ]
  },
  "options": [
    {"label": "应用此修改",   "value": "apply",     "style": "primary"},
    {"label": "先解释代码",   "value": "explain",   "style": "default"},
    {"label": "建议替代方案", "value": "alternative","style": "default"},
    {"label": "取消",         "value": "cancel",    "style": "danger"}
  ]
}
```
)REPLY";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    // Linux 上没有 Consolas / Courier New，QSS 的 font-family fallback 又不可靠，
    // 用 insertSubstitution 强制把 Consolas 替换成含中文的等宽字体，
    // 这样表格/按钮里的中文不会显示成方框。
    QFont::insertSubstitution("Consolas", "Noto Sans Mono CJK SC");
    QFont::insertSubstitution("Courier New", "Noto Sans Mono CJK SC");

    const ThemeId themes[] = {
        ThemeId::MilitaryTech,
        ThemeId::FutureTechBlue,
        ThemeId::WhiteMinimal,
        ThemeId::OneDarkPro,
        ThemeId::WeChatLight,
    };

    // 优先从 model_reply.txt 读取真实模型回复；不存在则用内置示例。
    QString modelReply;
    const QString replyPath = QStringLiteral("/workspace/model_reply.txt");
    if (QFileInfo::exists(replyPath)) {
        QFile f(replyPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            modelReply = QString::fromUtf8(f.readAll());
            qInfo("Loaded model reply from %s (%d chars)",
                  qPrintable(replyPath), modelReply.length());
        }
    }
    if (modelReply.trimmed().isEmpty()) {
        qInfo("model_reply.txt not found, using built-in sample.");
        modelReply = QString::fromUtf8(kModelReply);
    }

    // 用 parseAssistantReply 把"模型原始回复"解析成 ContentSegments
    const ContentSegments segs = parseAssistantReply(modelReply);

    QVector<QImage> cells;
    const int cellW = 560;

    for (ThemeId id : themes) {
        ChatWidget w;
        w.resize(cellW, 900);
        w.show();

        // 先创建气泡内容，再切换主题，确保 QSS + palette 应用到已存在的子部件
        w.addBubble(ChatBubble::Assistant, segs);
        w.setTheme(id);

        QEventLoop loop;
        QTimer::singleShot(900, &loop, &QEventLoop::quit);
        loop.exec();

        // 抓取 OptionsWidget 和 ToolParamsWidget
        const auto opts = w.findChildren<OptionsWidget*>();
        const auto tabs = w.findChildren<ToolParamsWidget*>();
        if (opts.isEmpty() || tabs.isEmpty())
            continue;

        QPixmap optPix = opts.last()->grab();
        QPixmap tabPix = tabs.last()->grab();

        const int pad = 16;
        const int gap = 14;
        const int labelH = 22;
        const int cellH = pad + labelH + optPix.height() + gap + labelH + tabPix.height() + pad;

        QImage cell(cellW, cellH, QImage::Format_RGB32);
        cell.fill(QColor(themeById(id).windowBg));

        QPainter p(&cell);
        QFont f(QStringLiteral("Consolas"), 10, QFont::Bold);
        p.setFont(f);
        QColor labelCol(themeById(id).titleColor);

        int y = pad;
        p.setPen(labelCol);
        p.drawText(QRect(pad, y, cellW - 2 * pad, labelH),
                   Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("OPTIONS (primary / default / danger)"));
        y += labelH;
        p.drawPixmap(pad, y, optPix);
        y += optPix.height() + gap;

        p.setPen(labelCol);
        p.drawText(QRect(pad, y, cellW - 2 * pad, labelH),
                   Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("PARAMETER TABLE"));
        y += labelH;
        p.drawPixmap(pad, y, tabPix);
        p.end();

        cells.append(cell);
    }

    // 拼成 3 列网格
    const int cols = 3;
    const int rows = (cells.size() + cols - 1) / cols;
    const int gap = 16;
    const int labelH = 26;
    const int gridW = cols * cellW + (cols + 1) * gap;
    const int gridH = rows * (cells.first().height() + labelH) + (rows + 1) * gap;
    QImage grid(gridW, gridH, QImage::Format_RGB32);
    grid.fill(QColor(10, 10, 10));

    QPainter gp(&grid);
    QFont gf(QStringLiteral("Consolas"), 12, QFont::Bold);
    gp.setFont(gf);
    for (int i = 0; i < cells.size(); ++i) {
        const int r = i / cols, c = i % cols;
        const int x = gap + c * (cellW + gap);
        const int y = gap + r * (cells[i].height() + labelH + gap);

        Theme t = themeById(themes[i]);
        gp.setPen(QColor(t.titleColor));
        gp.drawText(QRect(x, y, cellW, labelH),
                    Qt::AlignLeft | Qt::AlignVCenter, t.name);
        gp.drawImage(x, y + labelH, cells[i]);
    }
    gp.end();

    const QString path = QStringLiteral("/workspace/screenshot.png");
    grid.save(path, "PNG");
    qInfo("Saved screenshot to %s (%dx%d)", qPrintable(path), grid.width(), grid.height());
    return 0;
}
