#include "Skill.h"
#include <QRegularExpression>

bool Skill::matches(const QString &text) const
{
    if (text.isEmpty())
        return false;

    const QString t = text.toLower().trimmed();

    if (id.toLower() == t || name.toLower() == t)
        return true;

    for (const QString &alias : aliases) {
        if (alias.toLower() == t)
            return true;
    }

    return false;
}

QString Skill::resolvedSystemPrompt() const
{
    QString result = systemPrompt;
    QMapIterator<QString, QString> it(paramValues);
    while (it.hasNext()) {
        it.next();
        result.replace(QStringLiteral("{{%1}}").arg(it.key()), it.value());
    }
    return result;
}
