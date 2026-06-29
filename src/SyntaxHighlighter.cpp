#include "SyntaxHighlighter.h"

#include <QTextDocument>

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent, const QString &language)
    : QSyntaxHighlighter(parent)
{
    setup(language.toLower().trimmed());
}

QTextCharFormat SyntaxHighlighter::fmt(const char *color, bool bold, bool italic)
{
    QTextCharFormat f;
    f.setForeground(QColor(color));
    f.setFontWeight(bold ? QFont::Bold : QFont::Normal);
    f.setFontItalic(italic);
    return f;
}

void SyntaxHighlighter::setup(const QString &language)
{
    const QTextCharFormat keywordFmt = fmt("#569cd6", true);
    const QTextCharFormat stringFmt  = fmt("#ce9178");
    const QTextCharFormat numberFmt  = fmt("#b5cea8");
    const QTextCharFormat commentFmt = fmt("#6a9955", false, true);
    const QTextCharFormat funcFmt    = fmt("#dcdcaa");

    m_multiLineCommentFormat = commentFmt;

    QStringList keywords;
    bool cLike = false;

    if (language == "cpp" || language == "c++" || language == "cxx" ||
        language == "cc" || language == "h" || language == "hpp" || language == "c") {
        cLike = true;
        keywords << "int" << "float" << "double" << "char" << "void" << "bool" << "long"
                 << "short" << "unsigned" << "signed" << "const" << "static" << "class"
                 << "struct" << "public" << "private" << "protected" << "virtual" << "override"
                 << "final" << "namespace" << "using" << "template" << "typename" << "return"
                 << "if" << "else" << "for" << "while" << "do" << "switch" << "case" << "break"
                 << "continue" << "default" << "new" << "delete" << "this" << "true" << "false"
                 << "nullptr" << "auto" << "enum" << "union" << "sizeof" << "explicit" << "inline"
                 << "operator" << "friend" << "mutable" << "volatile" << "throw" << "try" << "catch";
    } else if (language == "python" || language == "py") {
        keywords << "def" << "class" << "return" << "if" << "elif" << "else" << "for" << "while"
                 << "import" << "from" << "as" << "try" << "except" << "finally" << "with"
                 << "lambda" << "yield" << "global" << "nonlocal" << "pass" << "break"
                 << "continue" << "True" << "False" << "None" << "and" << "or" << "not" << "in"
                 << "is" << "assert" << "del" << "raise" << "self" << "async" << "await";
    } else if (language == "javascript" || language == "js" ||
               language == "typescript" || language == "ts") {
        cLike = true;
        keywords << "var" << "let" << "const" << "function" << "return" << "if" << "else"
                 << "for" << "while" << "do" << "switch" << "case" << "break" << "continue"
                 << "new" << "this" << "class" << "extends" << "super" << "import" << "export"
                 << "from" << "default" << "try" << "catch" << "finally" << "throw" << "typeof"
                 << "instanceof" << "true" << "false" << "null" << "undefined" << "async" << "await"
                 << "yield" << "void" << "delete" << "in" << "of" << "interface" << "type" << "enum";
    } else if (language == "json") {
        keywords << "true" << "false" << "null";
    } else if (language == "bash" || language == "sh" || language == "shell") {
        keywords << "if" << "then" << "else" << "elif" << "fi" << "for" << "while" << "do" << "done"
                 << "case" << "esac" << "function" << "return" << "echo" << "export" << "local"
                 << "in" << "break" << "continue" << "source" << "cd" << "pwd" << "mkdir" << "rm";
    } else {
        // generic fallback
        keywords << "function" << "return" << "if" << "else" << "for" << "while" << "class"
                 << "def" << "import" << "var" << "let" << "const" << "true" << "false" << "null"
                 << "void" << "int" << "string" << "bool";
    }

    for (const auto &kw : keywords) {
        Rule r;
        r.pattern = QRegularExpression(QString("\\b%1\\b").arg(kw));
        r.format = keywordFmt;
        m_rules.append(r);
    }

    // strings: "..." '...' `...`
    Rule strRule;
    strRule.pattern = QRegularExpression("\"(?:\\\\.|[^\"\\\\])*\"|'(?:\\\\.|[^'\\\\])*'|`(?:\\\\.|[^`\\\\])*`");
    strRule.format = stringFmt;
    m_rules.append(strRule);

    // numbers
    Rule numRule;
    numRule.pattern = QRegularExpression("\\b\\d+(?:\\.\\d+)?\\b");
    numRule.format = numberFmt;
    m_rules.append(numRule);

    // function calls
    Rule funcRule;
    funcRule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
    funcRule.format = funcFmt;
    m_rules.append(funcRule);

    // single-line comments
    Rule slashComment;
    slashComment.pattern = QRegularExpression("//[^\n]*");
    slashComment.format = commentFmt;
    m_rules.append(slashComment);

    Rule hashComment;
    hashComment.pattern = QRegularExpression("#[^\n]*");
    hashComment.format = commentFmt;
    m_rules.append(hashComment);

    // multi-line comments (C-like /* */)
    if (cLike) {
        m_commentStart = QRegularExpression("/\\*");
        m_commentEnd = QRegularExpression("\\*/");
    }
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const Rule &rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.format);
        }
    }

    // multi-line comment state machine
    setCurrentBlockState(0);
    if (m_commentStart.pattern().isEmpty())
        return;

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(m_commentStart);

    while (startIndex >= 0) {
        auto endMatch = m_commentEnd.match(text, startIndex);
        int endIndex = endMatch.hasMatch() ? endMatch.capturedStart() : -1;
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }
        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStart, startIndex + commentLength);
    }
}
