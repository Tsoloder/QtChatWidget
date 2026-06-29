#include "SkillManager.h"
#include "Skill.h"
#include "SkillMdParser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#else
#define SKIP_EMPTY_PARTS QString::SkipEmptyParts
#endif

static bool skillCompareByUseCount(const Skill &a, const Skill &b)
{
    if (a.useCount != b.useCount)
        return a.useCount > b.useCount;
    return a.name < b.name;
}

SkillManager::SkillManager(QObject *parent)
    : QObject(parent)
{
    loadBuiltinSkills();
    loadUserSkills();
    loadUsageStats();
}

QString SkillManager::userSkillsDir() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(base + QStringLiteral("/skills"));
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));
    return dir.absolutePath();
}

void SkillManager::loadBuiltinSkills()
{
    const QString builtinDir = QStringLiteral(":/skills");
    QDir dir(builtinDir);
    if (!dir.exists())
        return;

    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &name : entries) {
        const QString subDir = builtinDir + QLatin1Char('/') + name;
        QString skillMdPath = subDir + QStringLiteral("/SKILL.md");
        QFileInfo fi(skillMdPath);
        if (fi.exists()) {
            loadSkillFromDirectory(subDir, true);
        }
    }
}

void SkillManager::loadUserSkills()
{
    const QString base = userSkillsDir();
    QDir dir(base);
    if (!dir.exists())
        return;

    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &name : entries) {
        const QString subDir = base + QLatin1Char('/') + name;
        loadSkillFromDirectory(subDir, false);
    }
}

bool SkillManager::loadSkillFromDirectory(const QString &dirPath, bool builtin)
{
    QFileInfo skillMd(dirPath + QStringLiteral("/SKILL.md"));
    if (!skillMd.exists() || !skillMd.isFile())
        return false;

    Skill skill;
    if (!SkillMdParser::parseFile(skillMd.absoluteFilePath(), &skill))
        return false;

    if (skill.id.isEmpty()) {
        QDir dir(dirPath);
        skill.id = dir.dirName();
        if (skill.name.isEmpty())
            skill.name = skill.id;
    }

    skill.builtin = builtin;
    m_skills.insert(skill.id, skill);
    return true;
}

bool SkillManager::installFromMarkdownFile(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile())
        return false;

    Skill skill;
    if (!SkillMdParser::parseFile(filePath, &skill))
        return false;

    if (skill.id.isEmpty()) {
        QString baseName = fi.baseName();
        skill.id = baseName;
        if (skill.name.isEmpty())
            skill.name = baseName;
    }

    QString targetDir = userSkillsDir() + QLatin1Char('/') + skill.id;
    QDir dir;
    if (!dir.mkpath(targetDir))
        return false;

    QString targetFile = targetDir + QStringLiteral("/SKILL.md");
    if (!QFile::copy(filePath, targetFile)) {
        if (QFile::exists(targetFile)) {
            QFile::remove(targetFile);
            if (!QFile::copy(filePath, targetFile))
                return false;
        } else {
            return false;
        }
    }

    skill.builtin = false;
    skill.sourcePath = targetFile;
    m_skills.insert(skill.id, skill);

    emit skillInstalled(skill.id);
    return true;
}

bool SkillManager::installFromDirectory(const QString &dirPath)
{
    QFileInfo skillMd(dirPath + QStringLiteral("/SKILL.md"));
    if (!skillMd.exists() || !skillMd.isFile())
        return false;

    return installFromMarkdownFile(skillMd.absoluteFilePath());
}

bool SkillManager::uninstall(const QString &id)
{
    if (!m_skills.contains(id))
        return false;

    const Skill skill = m_skills.value(id);
    if (skill.builtin)
        return false;

    QString targetDir = userSkillsDir() + QLatin1Char('/') + id;
    QDir dir(targetDir);
    if (dir.exists()) {
        dir.removeRecursively();
    }

    m_skills.remove(id);
    emit skillUninstalled(id);
    return true;
}

Skill SkillManager::skillById(const QString &id) const
{
    return m_skills.value(id);
}

QList<Skill> SkillManager::allSkills() const
{
    return m_skills.values();
}

QList<Skill> SkillManager::search(const QString &keyword) const
{
    QList<Skill> result;
    if (keyword.isEmpty())
        return allSkills();

    const QString kw = keyword.toLower();
    QMapIterator<QString, Skill> it(m_skills);
    while (it.hasNext()) {
        it.next();
        const Skill &s = it.value();
        if (s.id.toLower().contains(kw) ||
            s.name.toLower().contains(kw) ||
            s.description.toLower().contains(kw) ||
            s.category.toLower().contains(kw)) {
            result.append(s);
            continue;
        }
        bool found = false;
        for (const QString &alias : s.aliases) {
            if (alias.toLower().contains(kw)) {
                found = true;
                break;
            }
        }
        if (found)
            result.append(s);
    }
    return result;
}

QList<Skill> SkillManager::matchPrefix(const QString &prefix) const
{
    QList<Skill> result;
    if (prefix.isEmpty())
        return allSkills();

    const QString p = prefix.toLower();
    QMapIterator<QString, Skill> it(m_skills);
    while (it.hasNext()) {
        it.next();
        const Skill &s = it.value();
        if (s.id.toLower().startsWith(p) ||
            s.name.toLower().startsWith(p)) {
            result.append(s);
            continue;
        }
        for (const QString &alias : s.aliases) {
            if (alias.toLower().startsWith(p)) {
                result.append(s);
                break;
            }
        }
    }
    return result;
}

Skill SkillManager::matchTrigger(const QString &triggerText) const
{
    if (triggerText.isEmpty())
        return Skill();

    const QString t = triggerText.toLower().trimmed();

    QMapIterator<QString, Skill> it(m_skills);
    while (it.hasNext()) {
        it.next();
        const Skill &s = it.value();
        if (s.id.toLower() == t || s.name.toLower() == t)
            return s;
        for (const QString &alias : s.aliases) {
            if (alias.toLower() == t)
                return s;
        }
    }

    return Skill();
}

QList<Skill> SkillManager::allSkillsSorted() const
{
    QList<Skill> result = m_skills.values();
    qSort(result.begin(), result.end(), skillCompareByUseCount);
    return result;
}

void SkillManager::recordUsage(const QString &skillId)
{
    if (!m_skills.contains(skillId))
        return;
    m_skills[skillId].useCount++;
    saveUsageStats();
}

QList<Skill> SkillManager::suggestRelevant(const QString &text, int maxCount) const
{
    if (text.isEmpty())
        return QList<Skill>();

    const QString lowerText = text.toLower();
    QList<QPair<int, Skill>> scored;

    QMapIterator<QString, Skill> it(m_skills);
    while (it.hasNext()) {
        it.next();
        const Skill &s = it.value();
        int score = 0;

        if (lowerText.contains(s.name.toLower()))
            score += 10;
        if (lowerText.contains(s.id.toLower()))
            score += 8;
        if (lowerText.contains(s.description.toLower()))
            score += 5;
        if (lowerText.contains(s.category.toLower()))
            score += 3;
        for (const QString &tag : s.tags) {
            if (lowerText.contains(tag.toLower()))
                score += 4;
        }
        for (const QString &alias : s.aliases) {
            if (lowerText.contains(alias.toLower()))
                score += 7;
        }

        const QStringList keywords = s.systemPrompt.toLower().split(
            QRegularExpression(QStringLiteral("[\\s.,;:!?(){}\\[\\]\"'<>/\\\\]+")), SKIP_EMPTY_PARTS);
        int kwMatches = 0;
        for (const QString &kw : keywords) {
            if (kw.length() >= 4 && lowerText.contains(kw)) {
                kwMatches++;
            }
        }
        score += qMin(kwMatches, 5) * 2;

        score += s.useCount;

        if (score > 0) {
            scored.append(qMakePair(score, s));
        }
    }

    qSort(scored.begin(), scored.end(),
          [](const QPair<int, Skill> &a, const QPair<int, Skill> &b) {
              return a.first > b.first;
          });

    QList<Skill> result;
    for (int i = 0; i < qMin(maxCount, scored.size()); ++i) {
        result.append(scored[i].second);
    }
    return result;
}

QString SkillManager::catalogPrompt() const
{
    QList<Skill> sorted = allSkillsSorted();
    if (sorted.isEmpty())
        return QString();

    QString catalog = QStringLiteral("You have access to the following Skills. "
                                      "If the user's message matches a Skill, call the `use_skill` function "
                                      "with the skill's id. If no Skill is relevant, reply normally without calling any function.\n\n"
                                      "Available Skills:\n");

    for (const Skill &s : sorted) {
        catalog += QStringLiteral("- **%1** (id: `%2`): %3\n")
            .arg(s.name, s.id, s.description);
        if (!s.aliases.isEmpty())
            catalog += QStringLiteral("  aliases: `%1`\n").arg(s.aliases.join(QStringLiteral("`, `")));
        if (!s.tags.isEmpty())
            catalog += QStringLiteral("  tags: `%1`\n").arg(s.tags.join(QStringLiteral("`, `")));
    }

    return catalog;
}

QString SkillManager::usageStatsPath() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + QStringLiteral("/skill_usage.json");
}

void SkillManager::loadUsageStats()
{
    const QString path = usageStatsPath();
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    QStringList keys = obj.keys();
    for (const QString &id : keys) {
        if (m_skills.contains(id)) {
            m_skills[id].useCount = obj[id].toInt(0);
        }
    }
}

void SkillManager::saveUsageStats() const
{
    QJsonObject obj;
    QMapIterator<QString, Skill> it(m_skills);
    while (it.hasNext()) {
        it.next();
        if (it.value().useCount > 0) {
            obj[it.key()] = it.value().useCount;
        }
    }

    QJsonDocument doc(obj);
    const QString path = usageStatsPath();
    QFileInfo fi(path);
    QDir dir;
    if (!dir.exists(fi.dir().path()))
        dir.mkpath(fi.dir().path());

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson());
    }
}
