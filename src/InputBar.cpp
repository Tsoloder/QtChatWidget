#include "InputBar.h"

#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>

InputBar::InputBar(QWidget *parent)
    : QWidget(parent)
    , m_edit(nullptr)
{
    setObjectName(QStringLiteral("inputBar"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(6);

    // editor + send
    auto *editRow = new QHBoxLayout;
    editRow->setSpacing(8);
    m_edit = new QTextEdit;
    m_edit->setObjectName("inputEdit");
    m_edit->setPlaceholderText(QStringLiteral("Type a message... (Enter to send, Shift+Enter for newline)"));
    m_edit->setFixedHeight(64);
    m_edit->installEventFilter(this);
    auto *editShadow = new QGraphicsDropShadowEffect(m_edit);
    editShadow->setBlurRadius(14);
    editShadow->setOffset(0, 2);
    editShadow->setColor(QColor(0, 0, 0, 60));
    m_edit->setGraphicsEffect(editShadow);

    auto *sendBtn = new QPushButton("Send");
    sendBtn->setObjectName("sendBtn");
    sendBtn->setCursor(Qt::PointingHandCursor);
    sendBtn->setFixedSize(76, 64);
    sendBtn->setFocusPolicy(Qt::NoFocus);
    auto *sendShadow = new QGraphicsDropShadowEffect(sendBtn);
    sendShadow->setBlurRadius(12);
    sendShadow->setOffset(0, 2);
    sendShadow->setColor(QColor(0, 0, 0, 70));
    sendBtn->setGraphicsEffect(sendShadow);
    connect(sendBtn, &QPushButton::clicked, this, &InputBar::onSend);

    editRow->addWidget(m_edit);
    editRow->addWidget(sendBtn);
    layout->addLayout(editRow);
}

bool InputBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_edit && event->type() == QEvent::KeyPress) {
        auto *e = static_cast<QKeyEvent *>(event);
        if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
            !(e->modifiers() & Qt::ShiftModifier)) {
            onSend();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void InputBar::onSend()
{
    const QString text = m_edit->toPlainText().trimmed();
    if (text.isEmpty())
        return;
    m_edit->clear();
    emit send(text);
}
