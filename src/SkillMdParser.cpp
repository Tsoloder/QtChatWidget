#include "SkillMdParser.h"
#include "Skill.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QRegularExpression>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#else
#define SKIP_EMPTY_PARTS QString::SkipEmptyParts
#endif

bool SkillMdParser::parseFile(const QString &filePath, Skill *outSkill)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    in.setCodec("UTF-8");
    const QString content = in.readAll();
    file.close();

    return parseText(content, filePath, outSkill);
}

bool SkillMdParser::parseText(const QString &content, const QString &filePath, Skill *outSkill)
{
    if (!outSkill)
        return false;

    const QString trimmed = content.trimmed();
    if (!trimmed.startsWith(QStringLiteral("---")))
        return false;

    int firstDash = content.indexOf(QStringLiteral("---"));
    int secondDash = content.indexOf(QStringLiteral("---"), firstDash + 3);

    if (secondDash < 0)
        return false;

    const QString frontMatter = content.mid(firstDash + 3, secondDash - firstDash - 3).trimmed();
    const QString body = content.mid(secondDash + 3).trimmed();

    if (!parseFrontMatter(frontMatter, outSkill))
        return false;

    outSkill->systemPrompt = body;
    outSkill->sourcePath = filePath;

    if (outSkill->id.isEmpty() && !outSkill->name.isEmpty())
        outSkill->id = outSkill->name;

    if (outSkill->name.isEmpty() && !outSkill->id.isEmpty())
        outSkill->name = outSkill->id;

    return outSkill->isValid();
}

bool SkillMdParser::parseFrontMatter(const QString &text, Skill *outSkill)
{
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")));

    int i = 0;
    while (i < lines.size()) {
        const QString &line = lines[i];
        const QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) {
            ++i;
            continue;
        }

        if (trimmedLine.startsWith(QLatin1Char('#'))) {
            ++i;
            continue;
        }

        int colonPos = trimmedLine.indexOf(QLatin1Char(':'));
        if (colonPos <= 0) {
            ++i;
            continue;
        }

        const QString key = trimmedLine.left(colonPos).trimmed().toLower();
        const QString value = trimmedLine.mid(colonPos + 1).trimmed();

        if (key == QStringLiteral("name")) {
            outSkill->name = value;
            if (outSkill->id.isEmpty())
                outSkill->id = value;
        } else if (key == QStringLiteral("description")) {
            outSkill->description = value;
        } else if (key == QStringLiteral("version")) {
            outSkill->version = value;
        } else if (key == QStringLiteral("author")) {
            outSkill->author = value;
        } else if (key == QStringLiteral("category")) {
            outSkill->category = value;
        } else if (key == QStringLiteral("aliases")) {
            outSkill->aliases = parseArrayValue(value);
        } else if (key == QStringLiteral("tags")) {
            outSkill->tags = parseArrayValue(value);
        } else if (key == QStringLiteral("allowed-tools")) {
            outSkill->allowedTools = parseArrayValue(value);
        } else if (key == QStringLiteral("extra-params") || key == QStringLiteral("extra_params") || key == QStringLiteral("params")) {
            if (value.isEmpty() || value == QLatin1String("|")) {
                int baseIndent = line.indexOf(trimmedLine);
                ++i;
                QVector<SkillParam> params;
                while (i < lines.size()) {
                    const QString &itemLine = lines[i];
                    if (itemLine.trimmed().isEmpty()) {
                        ++i;
                        continue;
                    }
                    int itemIndent = itemLine.indexOf(itemLine.trimmed());
                    if (itemIndent <= baseIndent)
                        break;
                    if (itemLine.trimmed().startsWith(QLatin1String("- "))) {
                        SkillParam p;
                        QString rest = itemLine.trimmed().mid(2).trimmed();
                        int cPos = rest.indexOf(QLatin1Char(':'));
                        if (cPos > 0) {
                            QString fk = rest.left(cPos).trimmed().toLower();
                            QString fv = rest.mid(cPos + 1).trimmed();
                            if (fk == QLatin1String("name"))
                                p.name = fv;
                        }
                        ++i;
                        int itemBaseIndent = itemIndent + 2;
                        while (i < lines.size()) {
                            const QString &fieldLine = lines[i];
                            if (fieldLine.trimmed().isEmpty()) {
                                ++i;
                                continue;
                            }
                            int fieldIndent = fieldLine.indexOf(fieldLine.trimmed());
                            if (fieldIndent < itemBaseIndent)
                                break;
                            if (fieldLine.trimmed().startsWith(QLatin1String("- ")))
                                break;
                            int fcPos = fieldLine.trimmed().indexOf(QLatin1Char(':'));
                            if (fcPos > 0) {
                                QString fk = fieldLine.trimmed().left(fcPos).trimmed().toLower();
                                QString fv = fieldLine.trimmed().mid(fcPos + 1).trimmed();
                                if (fk == QLatin1String("name"))
                                    p.name = fv;
                                else if (fk == QLatin1String("description"))
                                    p.description = fv;
                                else if (fk == QLatin1String("default") || fk == QLatin1String("default_value"))
                                    p.defaultValue = fv;
                                else if (fk == QLatin1String("required"))
                                    p.required = (fv.toLower() == QLatin1String("true") || fv == QLatin1String("1"));
                            }
                            ++i;
                        }
                        if (!p.name.isEmpty())
                            params.append(p);
                    } else {
                        ++i;
                    }
                }
                outSkill->params = params;
                continue;
            } else {
                outSkill->params = QVector<SkillParam>();
            }
        }
        ++i;
    }

    return true;
}

QStringList SkillMdParser::parseArrayValue(const QString &value)
{
    QStringList result;

    QString v = value.trimmed();
    if (v.startsWith(QLatin1Char('[')) && v.endsWith(QLatin1Char(']'))) {
        v = v.mid(1, v.length() - 2).trimmed();
        if (v.isEmpty())
            return result;
        const QStringList parts = v.split(QLatin1Char(','), SKIP_EMPTY_PARTS);
        for (const QString &p : parts) {
            QString item = p.trimmed();
            if (item.startsWith(QLatin1Char('"')) && item.endsWith(QLatin1Char('"')))
                item = item.mid(1, item.length() - 2);
            else if (item.startsWith(QLatin1Char('\'')) && item.endsWith(QLatin1Char('\'')))
                item = item.mid(1, item.length() - 2);
            result.append(item);
        }
    } else if (!v.isEmpty()) {
        result.append(v);
    }

    return result;
}
