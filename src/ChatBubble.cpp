#include "ChatBubble.h"
#include "CodeEditor.h"
#include "OptionsWidget.h"
#include "ToolParamsWidget.h"
#include "SvgIcon.h"
#include "Theme.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>

ChatBubble::ChatBubble(Role role, const ContentSegments &segments, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 6);
    layout->setSpacing(0);

    auto *bubble = new QFrame;
    bubble->setObjectName(role == User ? "userBubble" : "assistantBubble");
    bubble->setAttribute(Qt::WA_StyledBackground, true);
    // soft drop shadow for a refined, elevated feel
    auto *shadow = new QGraphicsDropShadowEffect(bubble);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 3);
    shadow->setColor(QColor(0, 0, 0, 90));
    bubble->setGraphicsEffect(shadow);
    auto *inner = new QVBoxLayout(bubble);
    inner->setContentsMargins(14, 10, 14, 12);
    inner->setSpacing(8);

    auto *roleLabel = new QLabel(role == User ? QStringLiteral("You") : QStringLiteral("Assistant"));
    roleLabel->setObjectName("roleLabel");
    inner->addWidget(roleLabel);

    for (const ContentSegment &seg : segments) {
        switch (seg.type) {
        case ContentSegment::Text: {
            auto *lbl = new QLabel(seg.text);
            lbl->setObjectName("msgText");
            lbl->setWordWrap(true);
            lbl->setTextFormat(Qt::RichText);
            lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
            inner->addWidget(lbl);
            break;
        }
        case ContentSegment::Code: {
            auto *editor = new CodeEditor(seg.text, seg.language);
            auto *shadow = new QGraphicsDropShadowEffect(editor);
            shadow->setBlurRadius(14);
            shadow->setOffset(0, 2);
            shadow->setColor(QColor(0, 0, 0, 70));
            editor->setGraphicsEffect(shadow);
            inner->addWidget(editor);
            break;
        }
        case ContentSegment::Options: {
            auto *opts = new OptionsWidget(seg.options);
            connect(opts, &OptionsWidget::optionSelected, this,
                    [this](int idx, const QString &t) {
                        emit optionSelected(this, idx, t);
                    });
            inner->addWidget(opts);
            break;
        }
        case ContentSegment::ToolApproval: {
            auto *panel = new QFrame;
            panel->setObjectName("toolPanel");
            auto *shadow = new QGraphicsDropShadowEffect(panel);
            shadow->setBlurRadius(14);
            shadow->setOffset(0, 2);
            shadow->setColor(QColor(0, 0, 0, 70));
            panel->setGraphicsEffect(shadow);
            auto *pl = new QVBoxLayout(panel);
            pl->setContentsMargins(12, 10, 12, 10);
            pl->setSpacing(6);

            auto *titleRow = new QHBoxLayout;
            titleRow->setContentsMargins(0, 0, 0, 0);
            titleRow->setSpacing(6);
            auto *iconLbl = new QLabel;
            iconLbl->setPixmap(svgTintedPixmap(QStringLiteral(":/icons/gear.svg"), 16,
                                               QColor(currentTheme().toolTitleColor)));
            auto *title = new QLabel(QStringLiteral("[") + seg.toolName + QStringLiteral("]")); // name
            title->setObjectName("toolTitle");
            titleRow->addWidget(iconLbl);
            titleRow->addWidget(title);
            titleRow->addStretch();
            pl->addLayout(titleRow);
            auto *desc = new QLabel(seg.toolDescription);
            desc->setObjectName("toolDesc");
            desc->setWordWrap(true);
            desc->setTextInteractionFlags(Qt::TextSelectableByMouse);
            pl->addWidget(desc);

            auto *btnRow = new QWidget;
            auto *bl = new QHBoxLayout(btnRow);
            bl->setContentsMargins(0, 0, 0, 0);
            bl->setSpacing(8);
            auto *yes = new QPushButton("Yes");
            auto *no = new QPushButton("No");
            auto *always = new QPushButton("Yes, and don't ask again");
            yes->setObjectName("approveBtn");
            no->setObjectName("rejectBtn");
            always->setObjectName("approveBtn");
            yes->setCursor(Qt::PointingHandCursor);
            no->setCursor(Qt::PointingHandCursor);
            always->setCursor(Qt::PointingHandCursor);
            bl->addWidget(yes);
            bl->addWidget(no);
            bl->addWidget(always);
            bl->addStretch();
            pl->addWidget(btnRow);

            connect(yes, &QPushButton::clicked, this, [this, panel]() {
                panel->setEnabled(false);
                emit toolApproved(this, true, false);
            });
            connect(no, &QPushButton::clicked, this, [this, panel]() {
                panel->setEnabled(false);
                emit toolApproved(this, false, false);
            });
            connect(always, &QPushButton::clicked, this, [this, panel]() {
                panel->setEnabled(false);
                emit toolApproved(this, true, true);
            });

            inner->addWidget(panel);
            break;
        }
        case ContentSegment::ToolParams: {
            auto *wrap = new QFrame;
            wrap->setObjectName("toolPanel");
            auto *shadow = new QGraphicsDropShadowEffect(wrap);
            shadow->setBlurRadius(14);
            shadow->setOffset(0, 2);
            shadow->setColor(QColor(0, 0, 0, 70));
            wrap->setGraphicsEffect(shadow);
            auto *wl = new QVBoxLayout(wrap);
            wl->setContentsMargins(12, 10, 12, 10);
            wl->setSpacing(6);

            auto *tpw = new ToolParamsWidget(seg.toolName, seg.params);
            connect(tpw, &ToolParamsWidget::paramsConfirmed, this,
                    [this](const QVector<ContentSegment::Param> &p) {
                        emit paramsConfirmed(this, p);
                    });
            wl->addWidget(tpw);

            inner->addWidget(wrap);
            break;
        }
        }
    }

    layout->addWidget(bubble);
}
