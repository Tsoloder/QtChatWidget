#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include "Skill.h"

class QTextEdit;
class QPushButton;
class QLabel;
class QFrame;
class QHBoxLayout;
class QTimer;
class SkillPicker;
class SkillManager;

class InputBar : public QWidget
{
    Q_OBJECT
public:
    explicit InputBar(QWidget *parent = nullptr);

    void setSkillManager(SkillManager *manager);
    SkillManager *skillManager() const { return m_skillManager; }

    void addActiveSkill(const Skill &skill);
    void removeActiveSkill(const QString &skillId);
    void clearActiveSkills();
    bool hasActiveSkills() const { return !m_activeSkills.isEmpty(); }
    int activeSkillCount() const { return m_activeSkills.size(); }
    QList<Skill> activeSkills() const { return m_activeSkills; }
    QString combinedSystemPrompt() const;
    QStringList combinedAllowedTools() const;

    void clearEdit();

signals:
    void send(const QString &text);
    void skillActivated(const Skill &skill);
    void skillsChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onSend();
    void onTextChanged();
    void onSkillSelected(const Skill &skill);
    void onSkillPickerDismissed();
    void onSuggestionTimer();
    void onSuggestionClicked(const QString &skillId);
    void onForwardKeyEvent(QKeyEvent *event);

private:
    void updateSkillTags();
    void showSkillPicker();
    void hideSkillPicker();
    bool isInSkillContext() const;
    QString skillTriggerText() const;
    bool hasActiveSkill(const QString &id) const;
    void updateSuggestionBar(const QList<Skill> &suggestions);

    QTextEdit *m_edit;
    QPushButton *m_sendBtn;
    QFrame *m_skillTagBar;
    QHBoxLayout *m_tagLayout;
    QFrame *m_suggestionBar;
    QHBoxLayout *m_suggestionLayout;
    QLabel *m_suggestionLabel;
    QTimer *m_suggestionTimer;
    SkillPicker *m_skillPicker;
    SkillManager *m_skillManager;
    QList<Skill> m_activeSkills;
    bool m_skillPickerVisible;
};
