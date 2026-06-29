#include "SkillPicker.h"
#include "Skill.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QGraphicsDropShadowEffect>
#include <QSize>
#include <QDebug>
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QMouseEvent>

class RichTextDelegate : public QStyledItemDelegate
{
public:
    explicit RichTextDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();

        QTextDocument doc;
        doc.setHtml(opt.text);
        doc.setTextWidth(opt.rect.width());

        opt.text = QString();
        opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        QRect textRect = opt.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
        painter->translate(textRect.topLeft());
        QAbstractTextDocumentLayout::PaintContext ctx;
        if (opt.state & QStyle::State_Selected) {
            ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active, QPalette::HighlightedText));
        }
        doc.documentLayout()->draw(painter, ctx);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QTextDocument doc;
        doc.setHtml(opt.text);
        doc.setTextWidth(opt.rect.width());
        QSize size = doc.size().toSize();
        if (size.height() < 36)
            size.setHeight(36);
        size.setHeight(size.height() + 8);
        return size;
    }
};

SkillPicker::SkillPicker(QWidget *parent)
    : QFrame(parent)
    , m_list(nullptr)
    , m_selectedIndex(-1)
{
    setObjectName(QStringLiteral("skillPicker"));
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFocusPolicy(Qt::NoFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    m_list = new QListWidget;
    m_list->setObjectName("skillList");
    m_list->setFixedHeight(220);
    m_list->setUniformItemSizes(false);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->installEventFilter(this);
    m_list->setItemDelegate(new RichTextDelegate(m_list));

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(16);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(shadow);

    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemActivated, this, &SkillPicker::onItemActivated);
    connect(m_list, &QListWidget::itemClicked, this, &SkillPicker::onItemClicked);
}

void SkillPicker::setSkills(const QList<Skill> &skills)
{
    m_allSkills = skills;
    filter(QString());
}

void SkillPicker::filter(const QString &keyword)
{
    m_filteredSkills.clear();

    if (keyword.isEmpty()) {
        m_filteredSkills = m_allSkills;
    } else {
        const QString kw = keyword.toLower();
        for (const Skill &s : m_allSkills) {
            if (s.id.toLower().startsWith(kw) ||
                s.name.toLower().startsWith(kw) ||
                s.description.toLower().contains(kw)) {
                m_filteredSkills.append(s);
                continue;
            }
            bool found = false;
            for (const QString &alias : s.aliases) {
                if (alias.toLower().startsWith(kw)) {
                    found = true;
                    break;
                }
            }
            if (found)
                m_filteredSkills.append(s);
        }
    }

    updateList();

    if (m_filteredSkills.isEmpty())
        m_selectedIndex = -1;
    else
        m_selectedIndex = 0;

    setSelectedIndex(m_selectedIndex);
}

void SkillPicker::updateList()
{
    m_list->clear();
    if (m_filteredSkills.isEmpty()) {
        auto *item = new QListWidgetItem(m_list);
        item->setText(QStringLiteral("<span style=\"color:#888;font-size:12px;\">No matching skills found</span>"));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setSizeHint(QSize(0, 42));
        m_list->setFixedHeight(46);
        adjustSize();
        return;
    }
    m_list->setFixedHeight(220);
    for (const Skill &s : m_filteredSkills) {
        auto *item = new QListWidgetItem(m_list);
        QString text = QStringLiteral("<b>%1</b><br><span style=\"color:%2;font-size:11px;\">%3</span>")
                           .arg(s.name, QStringLiteral("#888"), s.description);
        if (!s.category.isEmpty()) {
            text = QStringLiteral("<span style=\"color:%3;font-size:10px;\">[%1]</span> %2")
                       .arg(s.category, text, QStringLiteral("#666"));
        }
        item->setText(text);
        item->setData(Qt::UserRole, s.id);
        item->setSizeHint(QSize(0, 42));
    }
}

Skill SkillPicker::currentSkill() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_filteredSkills.size())
        return m_filteredSkills.at(m_selectedIndex);
    return Skill();
}

void SkillPicker::setSelectedIndex(int idx)
{
    if (idx < 0 || idx >= m_filteredSkills.size()) {
        m_selectedIndex = -1;
        m_list->clearSelection();
        return;
    }
    m_selectedIndex = idx;
    m_list->setCurrentRow(idx);
}

void SkillPicker::moveSelection(int delta)
{
    if (m_filteredSkills.isEmpty())
        return;
    int newIdx = m_selectedIndex + delta;
    if (newIdx < 0)
        newIdx = m_filteredSkills.size() - 1;
    else if (newIdx >= m_filteredSkills.size())
        newIdx = 0;
    setSelectedIndex(newIdx);
}

void SkillPicker::acceptCurrent()
{
    Skill s = currentSkill();
    if (s.isValid()) {
        emit skillSelected(s);
        hide();
    }
}

void SkillPicker::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        moveSelection(-1);
        event->accept();
        return;
    case Qt::Key_Down:
        moveSelection(1);
        event->accept();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        acceptCurrent();
        event->accept();
        return;
    case Qt::Key_Escape:
        emit dismissed();
        hide();
        event->accept();
        return;
    default:
        break;
    }
    QFrame::keyPressEvent(event);
}

bool SkillPicker::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    // 全局鼠标点击检测：点击 Picker 和 输入框 以外的区域时关闭
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        QWidget *clicked = QApplication::widgetAt(me->globalPos());
        if (clicked) {
            // 点击在 Picker 自身或其子控件上 → 不关闭
            for (QWidget *w = clicked; w; w = w->parentWidget()) {
                if (w == this)
                    return false;
            }
            // 点击在输入框上 → 不关闭，用户可能继续输入
            for (QWidget *w = clicked; w; w = w->parentWidget()) {
                if (w->objectName() == "inputEdit")
                    return false;
            }
        }
        hide();
        emit dismissed();
    }
    return false;
}

void SkillPicker::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    setSelectedIndex(m_selectedIndex);
    // 安装全局事件过滤器以检测点击外部
    qApp->installEventFilter(this);
}

void SkillPicker::hideEvent(QHideEvent *event)
{
    QFrame::hideEvent(event);
    qApp->removeEventFilter(this);
}

void SkillPicker::onItemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    acceptCurrent();
}

void SkillPicker::onItemClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    acceptCurrent();
}
