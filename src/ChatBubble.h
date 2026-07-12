#pragma once

#include <QWidget>
#include "ContentSegment.h"

// One chat message: a role label + a vertical stack of content segments
// (text, code, options, tool-approval panels).
class ChatBubble : public QWidget
{
    Q_OBJECT
public:
    enum Role { User, Assistant };

    explicit ChatBubble(Role role, const ContentSegments &segments, QWidget *parent = nullptr);

    // Update the first text label's content (for streaming)
    void updateText(const QString &html);

    // Return the original segments used to build this bubble (for export)
    const ContentSegments &segments() const { return m_segments; }
    Role role() const { return m_role; }

signals:
    void optionSelected(ChatBubble *bubble, int index, const QString &text);
    void toolApproved(ChatBubble *bubble, bool approved, bool alwaysAllow);
    void paramsConfirmed(ChatBubble *bubble, const QVector<ContentSegment::Param> &params);

private:
    ContentSegments m_segments;
    Role m_role = User;
};
