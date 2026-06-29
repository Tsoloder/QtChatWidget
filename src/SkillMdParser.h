#pragma once

#include <QString>
#include "Skill.h"

class SkillMdParser
{
public:
    static bool parseFile(const QString &filePath, Skill *outSkill);
    static bool parseText(const QString &content, const QString &filePath, Skill *outSkill);

private:
    static bool parseFrontMatter(const QString &text, Skill *outSkill);
    static QStringList parseArrayValue(const QString &value);
};
