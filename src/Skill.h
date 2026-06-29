#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>

struct SkillParam {
    QString name;
    QString description;
    QString defaultValue;
    bool required;

    SkillParam() : required(false) {}
};

struct Skill {
    QString id;
    QString name;
    QString description;
    QStringList aliases;
    QString category;
    QString systemPrompt;
    QString version;
    QString author;
    QStringList tags;
    QStringList allowedTools;
    QVector<SkillParam> params;
    QMap<QString, QString> paramValues;
    QString sourcePath;
    bool builtin;
    int useCount;

    Skill() : builtin(false), useCount(0) {}

    bool matches(const QString &text) const;
    bool isValid() const { return !id.isEmpty() && !name.isEmpty(); }
    bool hasParams() const { return !params.isEmpty(); }
    QString resolvedSystemPrompt() const;
};
