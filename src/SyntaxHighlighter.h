#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QVector>
#include <QTextCharFormat>

// Multi-language syntax highlighter (VS Code Dark+ inspired colors).
// Supports: C/C++, Python, JavaScript/TypeScript, JSON, Bash, plus a generic fallback.
class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit SyntaxHighlighter(QTextDocument *parent, const QString &language);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> m_rules;
    QRegularExpression m_commentStart;
    QRegularExpression m_commentEnd;
    QTextCharFormat m_multiLineCommentFormat;

    void setup(const QString &language);
    static QTextCharFormat fmt(const char *color, bool bold = false, bool italic = false);
};
