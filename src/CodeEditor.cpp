#include "CodeEditor.h"
#include "SyntaxHighlighter.h"

#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QFontMetrics>

CodeEditor::CodeEditor(const QString &code, const QString &language, QWidget *parent)
    : QWidget(parent)
    , m_langLabel(nullptr)
    , m_editor(nullptr)
    , m_highlighter(nullptr)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // header bar
    auto *header = new QWidget;
    header->setObjectName("codeHeader");
    header->setFixedHeight(26);
    auto *hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(10, 0, 6, 0);
    hLayout->setSpacing(0);

    m_langLabel = new QLabel(language.isEmpty() ? QStringLiteral("code") : language);
    m_langLabel->setObjectName("codeLang");
    auto *copyBtn = new QPushButton("Copy");
    copyBtn->setObjectName("copyBtn");
    copyBtn->setCursor(Qt::PointingHandCursor);
    copyBtn->setFlat(true);
    copyBtn->setFocusPolicy(Qt::NoFocus);
    hLayout->addWidget(m_langLabel);
    hLayout->addStretch();
    hLayout->addWidget(copyBtn);
    connect(copyBtn, &QPushButton::clicked, this, &CodeEditor::onCopy);

    // editor
    m_editor = new QTextEdit;
    m_editor->setReadOnly(true);
    m_editor->setPlainText(code);
    m_editor->setObjectName("codeEdit");
    m_editor->setFrameStyle(QFrame::NoFrame);
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    m_editor->setFont(font);

    m_highlighter = new SyntaxHighlighter(m_editor->document(), language);

    mainLayout->addWidget(header);
    mainLayout->addWidget(m_editor);

    // size to content, cap height so long snippets scroll
    const int lines = code.count('\n') + 1;
    const int lineH = QFontMetrics(font).lineSpacing();
    const int h = qBound(60, lineH * lines + 16, 420);
    m_editor->setFixedHeight(h);
}

void CodeEditor::onCopy()
{
    QApplication::clipboard()->setText(m_editor->toPlainText());
}
