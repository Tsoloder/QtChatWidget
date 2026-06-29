#include "ReplyParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

// 从原始回复里抽出最后一个 ```json ... ``` 代码块的文本和位置。
// 返回 true 表示找到；false 表示没有 JSON 块。
static bool findLastJsonBlock(const QString &raw,
                              QString *outText,
                              int *outStart = nullptr,
                              int *outEnd = nullptr)
{
    static const QRegularExpression re(
        QStringLiteral("```json[ \t]*\\n(.*?)\\n```"),
        QRegularExpression::DotMatchesEverythingOption);

    bool found = false;
    auto it = re.globalMatch(raw);
    while (it.hasNext()) {
        auto m = it.next();
        found = true;
        if (outText)  *outText  = m.captured(1).trimmed();
        if (outStart) *outStart = m.capturedStart(0);
        if (outEnd)   *outEnd   = m.capturedEnd(0);
    }
    return found;
}

// 把一段 Markdown 文本按代码块拆分成 Text / Code 段。
static void splitTextIntoSegments(const QString &md, ContentSegments &out)
{
    static const QRegularExpression re(
        QStringLiteral("```([A-Za-z0-9_+-]*)[ \t]*\\n(.*?)\\n```"),
        QRegularExpression::DotMatchesEverythingOption);

    int cursor = 0;
    auto it = re.globalMatch(md);
    while (it.hasNext()) {
        auto m = it.next();

        // 代码块之前的普通文本
        if (m.capturedStart(0) > cursor) {
            QString before = md.mid(cursor, m.capturedStart(0) - cursor).trimmed();
            if (!before.isEmpty()) {
                ContentSegment seg;
                seg.type = ContentSegment::Text;
                seg.text = before;
                out.append(seg);
            }
        }

        // 代码块本身
        ContentSegment code;
        code.type = ContentSegment::Code;
        code.language = m.captured(1).toLower();
        code.text = m.captured(2);
        out.append(code);

        cursor = m.capturedEnd(0);
    }

    // 末尾剩余的文本
    if (cursor < md.length()) {
        QString rest = md.mid(cursor).trimmed();
        if (!rest.isEmpty()) {
            ContentSegment seg;
            seg.type = ContentSegment::Text;
            seg.text = rest;
            out.append(seg);
        }
    }
}

ContentSegments parseAssistantReply(const QString &rawReply)
{
    ContentSegments result;

    // 1. 找最后一个 ```json 块（约定它出现在回复的最后）
    QString jsonText;
    int jsonStart = -1;
    findLastJsonBlock(rawReply, &jsonText, &jsonStart);

    // 2. JSON 块之前的全部内容当作"正文"（自然语言 + 普通代码块）
    QString textPart = (jsonStart >= 0) ? rawReply.left(jsonStart) : rawReply;
    textPart = textPart.trimmed();
    if (!textPart.isEmpty())
        splitTextIntoSegments(textPart, result);

    // 3. 解析 JSON 块，按 tool_params → options 的顺序追加段
    if (jsonStart >= 0 && !jsonText.isEmpty()) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();

            // ---- 工具参数表（表格）----
            if (obj.contains(QStringLiteral("tool_params"))) {
                QJsonObject tp = obj.value(QStringLiteral("tool_params")).toObject();
                ContentSegment seg;
                seg.type = ContentSegment::ToolParams;
                seg.toolName = tp.value(QStringLiteral("tool")).toString();
                const QJsonArray params = tp.value(QStringLiteral("params")).toArray();
                for (const QJsonValue &pv : params) {
                    QJsonObject po = pv.toObject();
                    seg.params.append({
                        po.value(QStringLiteral("name")).toString(),
                        po.value(QStringLiteral("description")).toString(),
                        po.value(QStringLiteral("value")).toString()
                    });
                }
                if (!seg.params.isEmpty())
                    result.append(seg);
            }

            // ---- 选项按钮（位于表格之后）----
            if (obj.contains(QStringLiteral("options"))) {
                const QJsonArray opts = obj.value(QStringLiteral("options")).toArray();
                ContentSegment seg;
                seg.type = ContentSegment::Options;
                for (const QJsonValue &ov : opts) {
                    QJsonObject oo = ov.toObject();
                    seg.options.append({
                        oo.value(QStringLiteral("label")).toString(),
                        oo.value(QStringLiteral("value")).toString(),
                        oo.value(QStringLiteral("style")).toString()
                    });
                }
                if (!seg.options.isEmpty())
                    result.append(seg);
            }
        }
    }

    // 4. 如果什么都没解析出来，至少把原始回复当一条文本返回，避免空气泡
    if (result.isEmpty()) {
        ContentSegment seg;
        seg.type = ContentSegment::Text;
        seg.text = rawReply.trimmed();
        result.append(seg);
    }

    return result;
}
