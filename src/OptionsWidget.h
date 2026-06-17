#pragma once

#include <QWidget>
#include <QVector>
#include "ContentSegment.h"

// 把模型给出的可选项渲染成一列可点击的按钮。
// 每个选项可以有不同的 style（primary/default/danger/success），配色不同。
class OptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OptionsWidget(const QVector<ContentSegment::Option> &options, QWidget *parent = nullptr);

signals:
    // 用户点了第 index 个选项，回传该选项的 value（若无 value 则回传 label）
    void optionSelected(int index, const QString &value);
};
