#pragma once

#include <QFrame>
#include <QList>
#include "Skill.h"
#include "Theme.h"

class QListWidget;
class QListWidgetItem;

class SkillPicker : public QFrame
{
    Q_OBJECT
public:
    explicit SkillPicker(QWidget *parent = nullptr);

    void setSkills(const QList<Skill> &skills);
    void filter(const QString &keyword);

    Skill currentSkill() const;

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx);

    void setTheme(ThemeId id);

signals:
    void skillSelected(const Skill &skill);
    void dismissed();
    void forwardKeyEvent(QKeyEvent *event);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void onItemActivated(QListWidgetItem *item);
    void onItemClicked(QListWidgetItem *item);

private:
    void updateList();
    void moveSelection(int delta);
    void acceptCurrent();

    QListWidget *m_list;
    QList<Skill> m_allSkills;
    QList<Skill> m_filteredSkills;
    int m_selectedIndex;
    ThemeId m_theme = ThemeId::OneDarkPro;
};
