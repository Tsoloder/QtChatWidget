#pragma once

#include <QWidget>
#include <QString>

class QTextEdit;
class SyntaxHighlighter;
class QLabel;

// A read-only code block: header (language label + copy button) + highlighted editor.
class CodeEditor : public QWidget
{
    Q_OBJECT
public:
    explicit CodeEditor(const QString &code, const QString &language, QWidget *parent = nullptr);

private slots:
    void onCopy();

private:
    QLabel *m_langLabel;
    QTextEdit *m_editor;
    SyntaxHighlighter *m_highlighter;
};
