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

signals:
    void optionSelected(ChatBubble *bubble, int index, const QString &text);
    void toolApproved(ChatBubble *bubble, bool approved, bool alwaysAllow);
    void paramsConfirmed(ChatBubble *bubble, const QVector<ContentSegment::Param> &params);
};
