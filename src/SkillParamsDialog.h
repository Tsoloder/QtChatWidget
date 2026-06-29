#pragma once

#include <QDialog>
#include <QVector>
#include <QMap>
#include "Skill.h"

class QTableWidget;
class QLabel;
class QDialogButtonBox;

class SkillParamsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SkillParamsDialog(const Skill &skill, QWidget *parent = nullptr);

    QMap<QString, QString> paramValues() const;

private:
    void setupTable();

    Skill m_skill;
    QLabel *m_titleLabel;
    QLabel *m_descLabel;
    QTableWidget *m_table;
    QDialogButtonBox *m_buttons;
};
