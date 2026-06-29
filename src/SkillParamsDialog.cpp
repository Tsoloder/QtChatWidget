#include "SkillParamsDialog.h"
#include "Skill.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>

SkillParamsDialog::SkillParamsDialog(const Skill &skill, QWidget *parent)
    : QDialog(parent)
    , m_skill(skill)
    , m_titleLabel(nullptr)
    , m_descLabel(nullptr)
    , m_table(nullptr)
    , m_buttons(nullptr)
{
    setWindowTitle(QStringLiteral("Skill 参数 - %1").arg(skill.name));
    setMinimumWidth(520);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    m_titleLabel = new QLabel;
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setText(QStringLiteral("%1 参数配置").arg(skill.name));
    layout->addWidget(m_titleLabel);

    m_descLabel = new QLabel;
    m_descLabel->setWordWrap(true);
    m_descLabel->setText(skill.description);
    m_descLabel->setStyleSheet(QStringLiteral("color: #888;"));
    layout->addWidget(m_descLabel);

    m_table = new QTableWidget;
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels(
        QStringList() << QStringLiteral("参数名") << QStringLiteral("说明") << QStringLiteral("值"));
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    layout->addWidget(m_table, 1);

    setupTable();

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, [this]() {
        for (int i = 0; i < m_skill.params.size(); ++i) {
            const SkillParam &p = m_skill.params[i];
            if (p.required) {
                QLineEdit *le = qobject_cast<QLineEdit*>(m_table->cellWidget(i, 2));
                if (le && le->text().trimmed().isEmpty()) {
                    QMessageBox::warning(this, QStringLiteral("参数缺失"),
                        QStringLiteral("参数 \"%1\" 是必填项，请填写。").arg(p.name));
                    m_table->setCurrentCell(i, 2);
                    return;
                }
            }
        }
        accept();
    });
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttons);
}

void SkillParamsDialog::setupTable()
{
    m_table->setRowCount(m_skill.params.size());
    for (int i = 0; i < m_skill.params.size(); ++i) {
        const SkillParam &p = m_skill.params[i];

        auto *nameItem = new QTableWidgetItem(p.name + (p.required ? QStringLiteral(" *") : QString()));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 0, nameItem);

        auto *descItem = new QTableWidgetItem(p.description);
        descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 1, descItem);

        auto *edit = new QLineEdit;
        edit->setText(p.defaultValue);
        edit->setPlaceholderText(p.defaultValue.isEmpty() ? QStringLiteral("请输入...") : p.defaultValue);
        m_table->setCellWidget(i, 2, edit);
    }
    m_table->resizeColumnsToContents();
    if (m_table->columnWidth(0) < 120)
        m_table->setColumnWidth(0, 120);
    if (m_table->columnWidth(1) < 180)
        m_table->setColumnWidth(1, 200);
}

QMap<QString, QString> SkillParamsDialog::paramValues() const
{
    QMap<QString, QString> values;
    for (int i = 0; i < m_skill.params.size(); ++i) {
        const SkillParam &p = m_skill.params[i];
        QLineEdit *le = qobject_cast<QLineEdit*>(m_table->cellWidget(i, 2));
        QString val = le ? le->text().trimmed() : p.defaultValue;
        if (val.isEmpty())
            val = p.defaultValue;
        values[p.name] = val;
    }
    return values;
}
