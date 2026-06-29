#include "OptionsWidget.h"

#include <QPushButton>
#include <QVBoxLayout>

OptionsWidget::OptionsWidget(const QVector<ContentSegment::Option> &options, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    for (int i = 0; i < options.size(); ++i) {
        const auto &opt = options.at(i);
        auto *btn = new QPushButton(opt.label);
        // 根据 style 给按钮不同的 objectName，QSS 据此配色
        const QString style = opt.style.isEmpty()
            ? QStringLiteral("default") : opt.style.toLower();
        btn->setObjectName(QStringLiteral("optionBtn_") + style);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(34);
        btn->setFocusPolicy(Qt::NoFocus);
        // 点击后回传 value（若为空则回传 label）
        const QString value = opt.value.isEmpty() ? opt.label : opt.value;
        connect(btn, &QPushButton::clicked, this, [this, i, value]() {
            emit optionSelected(i, value);
        });
        layout->addWidget(btn);
    }
}
