#include "ToolParamsWidget.h"

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>

ToolParamsWidget::ToolParamsWidget(const QString &toolName,
                                   const QVector<ContentSegment::Param> &params,
                                   QWidget *parent)
    : QWidget(parent)
    , m_table(nullptr)
    , m_toolName(toolName)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // header
    auto *title = new QLabel(QString::fromUtf8("\xe2\x9a\x99 ") + toolName +
                             QStringLiteral("  \xe2\x80\x94 parameters"));
    title->setObjectName("toolTitle");
    layout->addWidget(title);

    // table
    m_table = new QTableWidget(static_cast<int>(params.size()), 3);
    m_table->setObjectName("paramTable");
    m_table->setHorizontalHeaderLabels({QStringLiteral("Name"),
                                        QStringLiteral("Description"),
                                        QStringLiteral("Value")});
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked |
                             QAbstractItemView::SelectedClicked |
                             QAbstractItemView::AnyKeyPressed);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(true);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setFocusPolicy(Qt::StrongFocus);

    for (int i = 0; i < params.size(); ++i) {
        const auto &p = params.at(i);

        auto *nameItem = new QTableWidgetItem(p.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

        auto *descItem = new QTableWidgetItem(p.description);
        descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);

        auto *valueItem = new QTableWidgetItem(p.value);
        valueItem->setFlags(valueItem->flags() | Qt::ItemIsEditable);

        m_table->setItem(i, 0, nameItem);
        m_table->setItem(i, 1, descItem);
        m_table->setItem(i, 2, valueItem);
    }

    // size to content, cap height
    const int rows = qMax(1, static_cast<int>(params.size()));
    const int h = qBound(90, 28 + rows * 30 + 6, 320);
    m_table->setFixedHeight(h);

    layout->addWidget(m_table);

    // confirm button row
    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    auto *confirm = new QPushButton(QString::fromUtf8("\xe2\x9c\x93 Confirm parameters"));
    confirm->setObjectName("confirmParamsBtn");
    confirm->setCursor(Qt::PointingHandCursor);
    confirm->setMinimumHeight(32);
    connect(confirm, &QPushButton::clicked, this, &ToolParamsWidget::onConfirm);
    row->addStretch();
    row->addWidget(confirm);
    layout->addLayout(row);
}

void ToolParamsWidget::onConfirm()
{
    QVector<ContentSegment::Param> out;
    out.reserve(m_table->rowCount());
    for (int i = 0; i < m_table->rowCount(); ++i) {
        ContentSegment::Param p;
        p.name = m_table->item(i, 0)->text();
        p.description = m_table->item(i, 1)->text();
        p.value = m_table->item(i, 2)->text();
        out.append(p);
    }
    // lock the table so the user can't keep editing after confirming
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setEnabled(false);
    // disable the confirm button itself
    if (auto *btn = findChild<QPushButton *>()) {
        // findChild returns the first QPushButton; verify it's the confirm btn
        if (btn->objectName() == QStringLiteral("confirmParamsBtn"))
            btn->setEnabled(false);
    }
    emit paramsConfirmed(out);
}
