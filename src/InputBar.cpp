#include "InputBar.h"
#include "SkillPicker.h"
#include "SkillManager.h"
#include "Skill.h"
#include "SkillParamsDialog.h"

#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QFrame>
#include <QCoreApplication>
#include <QTextCursor>
#include <QResizeEvent>
#include <QDebug>
#include <QColor>
#include <QPropertyAnimation>
#include <QTimer>

InputBar::InputBar(QWidget *parent)
    : QWidget(parent)
    , m_edit(nullptr)
    , m_sendBtn(nullptr)
    , m_skillTagBar(nullptr)
    , m_tagLayout(nullptr)
    , m_suggestionBar(nullptr)
    , m_suggestionLayout(nullptr)
    , m_suggestionLabel(nullptr)
    , m_suggestionTimer(nullptr)
    , m_skillPicker(nullptr)
    , m_skillManager(nullptr)
    , m_skillPickerVisible(false)
{
    setObjectName(QStringLiteral("inputBar"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(6);

    m_suggestionBar = new QFrame;
    m_suggestionBar->setObjectName("skillSuggestionBar");
    m_suggestionBar->setVisible(false);
    m_suggestionBar->setFixedHeight(26);
    m_suggestionLayout = new QHBoxLayout(m_suggestionBar);
    m_suggestionLayout->setContentsMargins(8, 2, 8, 2);
    m_suggestionLayout->setSpacing(6);
    m_suggestionLabel = new QLabel(QStringLiteral("💡 建议使用 Skill："));
    m_suggestionLabel->setObjectName("suggestionLabel");
    m_suggestionLabel->setStyleSheet(QStringLiteral("font-size: 11px;"));
    m_suggestionLayout->addWidget(m_suggestionLabel);
    m_suggestionLayout->addStretch(1);
    layout->addWidget(m_suggestionBar);

    m_skillTagBar = new QFrame;
    m_skillTagBar->setObjectName("skillTagBar");
    m_skillTagBar->setVisible(false);
    m_skillTagBar->setMinimumHeight(28);
    m_tagLayout = new QHBoxLayout(m_skillTagBar);
    m_tagLayout->setContentsMargins(6, 2, 6, 2);
    m_tagLayout->setSpacing(6);
    m_tagLayout->addStretch(1);
    layout->addWidget(m_skillTagBar);

    auto *editRow = new QHBoxLayout;
    editRow->setSpacing(8);
    m_edit = new QTextEdit;
    m_edit->setObjectName("inputEdit");
    m_edit->setPlaceholderText(QStringLiteral("Type a message... (Enter to send, Shift+Enter for newline, / for skills)"));
    m_edit->setFixedHeight(64);
    m_edit->installEventFilter(this);
    auto *editShadow = new QGraphicsDropShadowEffect(m_edit);
    editShadow->setBlurRadius(14);
    editShadow->setOffset(0, 2);
    editShadow->setColor(QColor(0, 0, 0, 60));
    m_edit->setGraphicsEffect(editShadow);

    m_sendBtn = new QPushButton(QStringLiteral("Send"));
    m_sendBtn->setObjectName("sendBtn");
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setFixedSize(76, 64);
    m_sendBtn->setFocusPolicy(Qt::NoFocus);
    auto *sendShadow = new QGraphicsDropShadowEffect(m_sendBtn);
    sendShadow->setBlurRadius(12);
    sendShadow->setOffset(0, 2);
    sendShadow->setColor(QColor(0, 0, 0, 70));
    m_sendBtn->setGraphicsEffect(sendShadow);
    connect(m_sendBtn, &QPushButton::clicked, this, &InputBar::onSend);

    editRow->addWidget(m_edit);
    editRow->addWidget(m_sendBtn);
    layout->addLayout(editRow);

    m_skillPicker = new SkillPicker(this);
    m_skillPicker->hide();
    connect(m_skillPicker, &SkillPicker::skillSelected, this, &InputBar::onSkillSelected);
    connect(m_skillPicker, &SkillPicker::dismissed, this, &InputBar::onSkillPickerDismissed);
    connect(m_skillPicker, &SkillPicker::forwardKeyEvent, this, &InputBar::onForwardKeyEvent);

    connect(m_edit, &QTextEdit::textChanged, this, &InputBar::onTextChanged);

    m_suggestionTimer = new QTimer(this);
    m_suggestionTimer->setSingleShot(true);
    m_suggestionTimer->setInterval(500);
    connect(m_suggestionTimer, &QTimer::timeout, this, &InputBar::onSuggestionTimer);

    // 初始主题
    updateThemeStyles(themeById(m_themeId));
}

void InputBar::setSkillManager(SkillManager *manager)
{
    m_skillManager = manager;
    if (m_skillPicker && m_skillManager) {
        m_skillPicker->setSkills(m_skillManager->allSkillsSorted());
    }
}

void InputBar::addActiveSkill(const Skill &skill)
{
    if (!skill.isValid())
        return;
    if (hasActiveSkill(skill.id))
        return;

    Skill s = skill;
    if (s.hasParams()) {
        SkillParamsDialog dlg(s, this);
        if (dlg.exec() != QDialog::Accepted)
            return;
        s.paramValues = dlg.paramValues();
    }

    m_activeSkills.append(s);
    if (m_skillManager)
        m_skillManager->recordUsage(s.id);
    updateSkillTags();
    emit skillActivated(s);
    emit skillsChanged();
}

void InputBar::removeActiveSkill(const QString &skillId)
{
    for (int i = 0; i < m_activeSkills.size(); ++i) {
        if (m_activeSkills[i].id == skillId) {
            m_activeSkills.removeAt(i);
            updateSkillTags();
            emit skillsChanged();
            return;
        }
    }
}

void InputBar::clearActiveSkills()
{
    if (m_activeSkills.isEmpty())
        return;
    m_activeSkills.clear();
    updateSkillTags();
    emit skillsChanged();
}

bool InputBar::hasActiveSkill(const QString &id) const
{
    for (const Skill &s : m_activeSkills) {
        if (s.id == id)
            return true;
    }
    return false;
}

QString InputBar::combinedSystemPrompt() const
{
    QStringList parts;
    for (const Skill &s : m_activeSkills) {
        parts.append(s.resolvedSystemPrompt());
    }
    QString result = parts.join(QStringLiteral("\n\n---\n\n"));

    QStringList allowedTools = combinedAllowedTools();
    if (!allowedTools.isEmpty()) {
        result += QStringLiteral("\n\n---\n\n"
                                 "## 工具使用限制\n"
                                 "你只能使用以下工具：%1\n"
                                 "不要使用未在列表中的工具。").arg(allowedTools.join(QStringLiteral(", ")));
    }

    return result;
}

QStringList InputBar::combinedAllowedTools() const
{
    QStringList result;
    bool hasAnySkill = false;
    for (const Skill &s : m_activeSkills) {
        if (!s.allowedTools.isEmpty()) {
            hasAnySkill = true;
            for (const QString &t : s.allowedTools) {
                if (!result.contains(t))
                    result.append(t);
            }
        }
    }
    if (!hasAnySkill)
        return QStringList();
    return result;
}

void InputBar::updateSkillTags()
{
    QLayoutItem *child;
    while ((child = m_tagLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }

    if (m_activeSkills.isEmpty()) {
        m_skillTagBar->setVisible(false);
        return;
    }

    m_skillTagBar->setVisible(true);

    const Theme t = themeById(m_themeId);
    const QString accent = t.inputFocusAccent;
    const QString textColor = t.msgTextColor;
    const QString dimColor = t.statusColor;

    for (const Skill &s : m_activeSkills) {
        auto *tagFrame = new QFrame;
        tagFrame->setObjectName("skillTagItem");
        tagFrame->setFixedHeight(24);
        tagFrame->setStyleSheet(QStringLiteral(
            "QFrame#skillTagItem {"
            "  background: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 12px;"
            "}"
        ).arg(rgba(accent, 30), rgba(accent, 60)));

        auto *tagHLayout = new QHBoxLayout(tagFrame);
        tagHLayout->setContentsMargins(10, 2, 4, 2);
        tagHLayout->setSpacing(4);

        auto *slashLabel = new QLabel(QStringLiteral("/"));
        slashLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(accent));
        tagHLayout->addWidget(slashLabel);

        auto *nameLabel = new QLabel(s.name);
        nameLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;").arg(textColor));
        tagHLayout->addWidget(nameLabel);

        if (s.hasParams()) {
            auto *paramBadge = new QLabel(QStringLiteral("%1 param(s)").arg(s.params.size()));
            paramBadge->setStyleSheet(QStringLiteral(
                "color: %1; font-size: 10px; padding: 1px 4px;"
                "background: %2; border-radius: 6px;"
            ).arg(dimColor, rgba(textColor, 12)));
            tagHLayout->addWidget(paramBadge);
        }

        auto *clearBtn = new QPushButton(QStringLiteral("×"));
        clearBtn->setFixedSize(18, 18);
        clearBtn->setCursor(Qt::PointingHandCursor);
        clearBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: transparent; color: %1; border: none;"
            "  font-size: 14px; font-weight: bold; padding: 0;"
            "}"
            "QPushButton:hover { color: %2; }"
        ).arg(dimColor, textColor));
        QString skillId = s.id;
        connect(clearBtn, &QPushButton::clicked, this, [this, skillId]() {
            removeActiveSkill(skillId);
        });
        tagHLayout->addWidget(clearBtn);

        m_tagLayout->addWidget(tagFrame);
    }

    if (m_activeSkills.size() > 1) {
        auto *clearAllBtn = new QPushButton(QStringLiteral("Clear all"));
        clearAllBtn->setFixedHeight(20);
        clearAllBtn->setCursor(Qt::PointingHandCursor);
        clearAllBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: transparent; color: %1; border: none;"
            "  font-size: 11px; text-decoration: underline;"
            "}"
            "QPushButton:hover { color: %2; }"
        ).arg(dimColor, textColor));
        connect(clearAllBtn, &QPushButton::clicked, this, &InputBar::clearActiveSkills);
        m_tagLayout->addWidget(clearAllBtn);
    }

    m_tagLayout->addStretch(1);
}

void InputBar::clearEdit()
{
    m_edit->clear();
}

void InputBar::setBusy(bool busy)
{
    m_edit->setEnabled(!busy);
    m_sendBtn->setEnabled(!busy);
}

bool InputBar::isInSkillContext() const
{
    if (!m_edit)
        return false;
    const QString text = m_edit->toPlainText();
    if (!text.startsWith(QLatin1Char('/')))
        return false;
    int spaceIdx = text.indexOf(QLatin1Char(' '));
    int newlineIdx = text.indexOf(QLatin1Char('\n'));
    if (spaceIdx == 0)
        return false;
    if (newlineIdx == 0)
        return false;
    return true;
}

QString InputBar::skillTriggerText() const
{
    if (!isInSkillContext())
        return QString();
    const QString text = m_edit->toPlainText();
    int spaceIdx = text.indexOf(QLatin1Char(' '));
    int newlineIdx = text.indexOf(QLatin1Char('\n'));
    int endIdx = text.length();
    if (spaceIdx >= 0 && spaceIdx < endIdx)
        endIdx = spaceIdx;
    if (newlineIdx >= 0 && newlineIdx < endIdx)
        endIdx = newlineIdx;
    return text.mid(1, endIdx - 1);
}

void InputBar::showSkillPicker()
{
    if (!m_skillPicker || !m_skillManager)
        return;

    const QString trigger = skillTriggerText();
    m_skillPicker->filter(trigger);

    QPoint editBottomLeft = m_edit->mapTo(this, QPoint(0, m_edit->height()));
    int pickerWidth = m_edit->width();
    int pickerHeight = m_skillPicker->sizeHint().height();
    if (pickerHeight > 300)
        pickerHeight = 300;

    int pickerX = editBottomLeft.x();
    int pickerY = editBottomLeft.y() + 4;

    if (pickerY + pickerHeight > this->height() + 200) {
        pickerY = m_edit->mapTo(this, QPoint(0, 0)).y() - pickerHeight - 4;
    }

    m_skillPicker->setParent(this);
    m_skillPicker->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_skillPicker->move(mapToGlobal(QPoint(pickerX, pickerY)));
    m_skillPicker->resize(pickerWidth, pickerHeight);
    m_skillPicker->show();
    m_skillPicker->raise();
    m_edit->setFocus();  // 确保焦点回到输入框
    m_skillPickerVisible = true;
}

void InputBar::hideSkillPicker()
{
    if (m_skillPicker) {
        m_skillPicker->hide();
    }
    m_skillPickerVisible = false;
}

void InputBar::onTextChanged()
{
    if (!m_skillManager)
        return;

    if (isInSkillContext()) {
        const QString trigger = skillTriggerText();
        if (m_skillPickerVisible) {
            m_skillPicker->filter(trigger);
        } else {
            showSkillPicker();
        }
        if (m_suggestionBar)
            m_suggestionBar->setVisible(false);
        if (m_suggestionTimer)
            m_suggestionTimer->stop();
    } else {
        if (m_skillPickerVisible) {
            hideSkillPicker();
        }
        if (m_suggestionTimer) {
            const QString text = m_edit->toPlainText().trimmed();
            if (text.length() >= 4) {
                m_suggestionTimer->start();
            } else {
                m_suggestionTimer->stop();
                if (m_suggestionBar)
                    m_suggestionBar->setVisible(false);
            }
        }
    }
}

void InputBar::onSkillSelected(const Skill &skill)
{
    if (!skill.isValid())
        return;

    addActiveSkill(skill);

    QString remaining;
    const QString text = m_edit->toPlainText();
    int spaceIdx = text.indexOf(QLatin1Char(' '));
    int newlineIdx = text.indexOf(QLatin1Char('\n'));
    int endIdx = text.length();
    if (spaceIdx >= 0 && spaceIdx < endIdx)
        endIdx = spaceIdx;
    if (newlineIdx >= 0 && newlineIdx < endIdx)
        endIdx = newlineIdx;
    if (endIdx < text.length())
        remaining = text.mid(endIdx + 1);

    m_edit->setPlainText(remaining);
    m_edit->moveCursor(QTextCursor::End);

    hideSkillPicker();
    m_edit->setFocus();
}

void InputBar::onSkillPickerDismissed()
{
    hideSkillPicker();
    m_edit->setFocus();
}

bool InputBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_edit && event->type() == QEvent::KeyPress) {
        auto *e = static_cast<QKeyEvent *>(event);

        if (m_skillPickerVisible && m_skillPicker) {
            if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) {
                QKeyEvent keyEv(e->type(), e->key(), e->modifiers(),
                                e->text(), e->isAutoRepeat(), e->count());
                QCoreApplication::sendEvent(m_skillPicker, &keyEv);
                return true;
            }
            if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                if (!(e->modifiers() & Qt::ShiftModifier)) {
                    QKeyEvent keyEv(e->type(), e->key(), e->modifiers(),
                                    e->text(), e->isAutoRepeat(), e->count());
                    QCoreApplication::sendEvent(m_skillPicker, &keyEv);
                    return true;
                }
            }
            if (e->key() == Qt::Key_Escape) {
                hideSkillPicker();
                return true;
            }
        }

        if (e->key() == Qt::Key_Backspace) {
            if (m_edit->toPlainText().isEmpty() && !m_activeSkills.isEmpty()) {
                removeActiveSkill(m_activeSkills.last().id);
                return true;
            }
        }

        if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
            !(e->modifiers() & Qt::ShiftModifier)) {
            if (!m_skillPickerVisible) {
                onSend();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void InputBar::onSend()
{
    const QString text = m_edit->toPlainText().trimmed();
    if (text.isEmpty())
        return;
    m_edit->clear();
    emit send(text);
}

// static
QString InputBar::rgba(const QString &hex, int alpha)
{
    QColor c(hex);
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha);
}

void InputBar::updateThemeStyles(const Theme &t)
{
    // 更新常驻部件的颜色
    m_suggestionLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;").arg(t.statusColor));
    m_skillTagBar->setStyleSheet(QString());  // 清除，让内部子部件自己的 QSS 生效
    m_suggestionBar->setStyleSheet(QString());
}

void InputBar::setTheme(ThemeId id)
{
    if (m_themeId == id)
        return;
    m_themeId = id;
    updateThemeStyles(themeById(id));
    updateSkillTags();
    // 建议栏随主题更新（如果当前可见）
    if (m_suggestionBar->isVisible()) {
        // 重建建议按钮的样式：通过重新触发 onSuggestionTimer
        onSuggestionTimer();
    }
}

void InputBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_skillPickerVisible && m_skillPicker) {
        QPoint globalPos = m_edit->mapToGlobal(QPoint(0, 0));
        m_skillPicker->move(globalPos.x(), globalPos.y() - m_skillPicker->height() - 4);
        m_skillPicker->resize(m_edit->width(), m_skillPicker->height());
    }
}

void InputBar::onSuggestionTimer()
{
    if (!m_skillManager || !m_edit)
        return;
    const QString text = m_edit->toPlainText().trimmed();
    if (text.length() < 4)
        return;

    QList<Skill> suggestions = m_skillManager->suggestRelevant(text, 3);

    QList<Skill> filtered;
    for (const Skill &s : suggestions) {
        if (!hasActiveSkill(s.id))
            filtered.append(s);
    }

    updateSuggestionBar(filtered);
}

void InputBar::updateSuggestionBar(const QList<Skill> &suggestions)
{
    if (!m_suggestionBar || !m_suggestionLayout)
        return;

    QLayoutItem *child;
    while ((child = m_suggestionLayout->takeAt(1)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }

    if (suggestions.isEmpty()) {
        m_suggestionBar->setVisible(false);
        return;
    }

    m_suggestionBar->setVisible(true);

    const Theme t = themeById(m_themeId);
    const QString accent = t.inputFocusAccent;

    for (const Skill &s : suggestions) {
        auto *btn = new QPushButton(s.name);
        btn->setFixedHeight(20);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: %1;"
            "  color: %2;"
            "  border: 1px solid %3;"
            "  border-radius: 8px;"
            "  padding: 0 10px;"
            "  font-size: 11px;"
            "}"
            "QPushButton:hover {"
            "  background: %4;"
            "}"
        ).arg(rgba(accent, 18), accent, rgba(accent, 50), rgba(accent, 35)));
        QString sid = s.id;
        connect(btn, &QPushButton::clicked, this, [this, sid]() {
            onSuggestionClicked(sid);
        });
        m_suggestionLayout->addWidget(btn);
    }

    m_suggestionLayout->addStretch(1);
}

void InputBar::onSuggestionClicked(const QString &skillId)
{
    if (!m_skillManager)
        return;
    Skill s = m_skillManager->skillById(skillId);
    if (s.isValid()) {
        addActiveSkill(s);
        if (m_suggestionBar)
            m_suggestionBar->setVisible(false);
    }
}

void InputBar::onForwardKeyEvent(QKeyEvent *event)
{
    if (!m_edit || !event)
        return;
    // 把按键事件直接转发到文本编辑框，触发 textChanged → onTextChanged → filter
    QKeyEvent ev(event->type(), event->key(), event->modifiers(),
                 event->text(), event->isAutoRepeat(), event->count());
    QCoreApplication::sendEvent(m_edit, &ev);
}
