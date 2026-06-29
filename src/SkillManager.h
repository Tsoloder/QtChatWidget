#pragma once

#include <QObject>
#include <QMap>
#include <QList>
#include "Skill.h"

class SkillManager : public QObject
{
    Q_OBJECT
public:
    explicit SkillManager(QObject *parent = nullptr);

    bool installFromMarkdownFile(const QString &filePath);
    bool installFromDirectory(const QString &dirPath);
    bool uninstall(const QString &id);

    Skill skillById(const QString &id) const;
    QList<Skill> allSkills() const;
    QList<Skill> allSkillsSorted() const;
    QList<Skill> search(const QString &keyword) const;
    QList<Skill> matchPrefix(const QString &prefix) const;
    Skill matchTrigger(const QString &triggerText) const;

    void recordUsage(const QString &skillId);
    QList<Skill> suggestRelevant(const QString &text, int maxCount = 3) const;

    // Generate a catalog of all available Skills for model routing
    QString catalogPrompt() const;

    QString userSkillsDir() const;

signals:
    void skillInstalled(const QString &id);
    void skillUninstalled(const QString &id);

private:
    void loadBuiltinSkills();
    void loadUserSkills();
    bool loadSkillFromDirectory(const QString &dirPath, bool builtin);
    void loadUsageStats();
    void saveUsageStats() const;
    QString usageStatsPath() const;

    QMap<QString, Skill> m_skills;
};
