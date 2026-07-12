#include "SessionListPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

SessionListPanel::SessionListPanel(QWidget *parent)
    : QDockWidget(QStringLiteral("Sessions"), parent)
{
    QWidget *container = new QWidget(this);
    container->setObjectName(QStringLiteral("sessionContainer"));
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    m_list = new QListWidget(container);
    m_list->setIconSize(QSize(16, 16));
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_list, &QListWidget::itemClicked, this, &SessionListPanel::onItemClicked);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &SessionListPanel::onItemDoubleClicked);
    connect(m_list, &QListWidget::customContextMenuRequested, this, &SessionListPanel::onContextMenu);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_newBtn = new QPushButton(QStringLiteral("+ New"), container);
    m_refreshBtn = new QPushButton(QStringLiteral("Refresh"), container);
    m_delBtn = new QPushButton(QStringLiteral("Delete"), container);
    btnLayout->addWidget(m_newBtn);
    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addWidget(m_delBtn);

    layout->addWidget(m_list);
    layout->addLayout(btnLayout);

    setWidget(container);

    m_titleBar = new QLabel(QStringLiteral("Sessions"), this);
    m_titleBar->setObjectName(QStringLiteral("sessionTitleBar"));
    m_titleBar->setAutoFillBackground(true);
    setTitleBarWidget(m_titleBar);

    connect(m_newBtn, &QPushButton::clicked, this, &SessionListPanel::onNewClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SessionListPanel::onRefreshClicked);
    connect(m_delBtn, &QPushButton::clicked, this, &SessionListPanel::onDeleteClicked);
}

void SessionListPanel::setTheme(ThemeId id)
{
    m_theme = id;
    const Theme &t = themeById(id);
    QString qss = QString(
        "QDockWidget { background: %1; }"
        "QWidget#sessionContainer { background: %1; }"
        "QLabel#sessionTitleBar { background: %9; color: %2; padding: 6px 12px;"
        "  font-size: 13px; font-weight: bold; }"
        "QListWidget { background: %3; color: %2; border: 1px solid %4; border-radius: 6px;"
        "  alternate-background-color: %5; }"
        "QListWidget::item { padding: 8px 12px; border-radius: 4px; }"
        "QListWidget::item:hover { background: %6; }"
        "QListWidget::item:selected { background: %7; color: %1; }"
        "QPushButton { background: %4; color: %2; border: none; padding: 6px 14px;"
        "  border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background: %7; }"
        "QPushButton:pressed { background: %8; }")
        .arg(t.windowBg,              // %1: 窗口背景 / 选中文字色
             t.titleColor,             // %2: 标题文字色 / 列表文字色 / 按钮文字色
             t.assistantBubbleBg,      // %3: 列表背景
             t.inputBorder,            // %4: 列表边框 / 按钮默认背景
             t.tableAltBg,             // %5: 列表交替行背景
             t.optionBtnHoverBg,       // %6: 列表项 hover
             t.inputAccent,            // %7: 选中项背景 / 按钮 hover
             t.optionBtnPressedBg,     // %8: 按钮按下
             t.headerBg);              // %9: 标题栏背景
    setStyleSheet(qss);
}

void SessionListPanel::refresh()
{
    if (!m_net || m_baseUrl.isEmpty())
        return;

    QNetworkRequest req(QUrl(m_baseUrl + QStringLiteral("/sessions")));
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject())
            return;

        m_list->clear();
        QJsonArray sessions = doc.object().value(QStringLiteral("sessions")).toArray();
        for (const QJsonValue &v : sessions) {
            QJsonObject obj = v.toObject();
            QString id = obj.value(QStringLiteral("id")).toString();
            QString title = obj.value(QStringLiteral("title")).toString();
            QString updated = obj.value(QStringLiteral("updated_at")).toString();

            QString display = title;
            if (display.isEmpty())
                display = QStringLiteral("Untitled");

            QListWidgetItem *item = new QListWidgetItem(display, m_list);
            item->setData(Qt::UserRole, id);
            item->setToolTip(updated);
        }
    });
}

void SessionListPanel::onNewClicked()
{
    if (!m_net || m_baseUrl.isEmpty())
        return;

    QNetworkRequest req(QUrl(m_baseUrl + QStringLiteral("/sessions")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = m_net->post(req, QByteArray("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            QString id = doc.object().value(QStringLiteral("id")).toString();
            emit newSessionRequested();
            emit sessionSelected(id);
            refresh();
        }
    });
}

void SessionListPanel::onRefreshClicked()
{
    refresh();
}

void SessionListPanel::onDeleteClicked()
{
    QListWidgetItem *item = m_list->currentItem();
    if (!item)
        return;

    QString id = item->data(Qt::UserRole).toString();
    emit deleteSessionRequested(id);

    QNetworkRequest req(QUrl(m_baseUrl + QStringLiteral("/sessions/") + id));
    QNetworkReply *reply = m_net->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        refresh();
    });
}

void SessionListPanel::onItemClicked(QListWidgetItem *item)
{
    if (!item)
        return;
    QString id = item->data(Qt::UserRole).toString();
    emit sessionSelected(id);
}

void SessionListPanel::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item)
        return;
    QString id = item->data(Qt::UserRole).toString();
    renameSession(id, item->text());
}

void SessionListPanel::onContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item)
        return;

    QMenu menu(this);
    QAction *renameAction = menu.addAction(QStringLiteral("Rename"));
    QAction *deleteAction = menu.addAction(QStringLiteral("Delete"));
    menu.addSeparator();
    QAction *copyIdAction = menu.addAction(QStringLiteral("Copy Session ID"));

    const Theme &t = themeById(m_theme);
    menu.setStyleSheet(QString(
        "QMenu { background: %1; color: %2; border: 1px solid %3; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: %4; }")
        .arg(t.assistantBubbleBg, t.msgTextColor, t.inputBorder, t.inputAccent));

    QAction *selected = menu.exec(m_list->viewport()->mapToGlobal(pos));
    if (selected == renameAction) {
        QString id = item->data(Qt::UserRole).toString();
        renameSession(id, item->text());
    } else if (selected == deleteAction) {
        onDeleteClicked();
    } else if (selected == copyIdAction) {
        QApplication::clipboard()->setText(item->data(Qt::UserRole).toString());
    }
}

void SessionListPanel::onRenameAction()
{
    QListWidgetItem *item = m_list->currentItem();
    if (!item)
        return;
    QString id = item->data(Qt::UserRole).toString();
    renameSession(id, item->text());
}

void SessionListPanel::renameSession(const QString &id, const QString &oldTitle)
{
    if (!m_net || m_baseUrl.isEmpty() || id.isEmpty())
        return;

    bool ok = false;
    QString newTitle = QInputDialog::getText(
        this,
        QStringLiteral("Rename Session"),
        QStringLiteral("New title:"),
        QLineEdit::Normal,
        oldTitle,
        &ok);
    if (!ok || newTitle.trimmed().isEmpty() || newTitle == oldTitle)
        return;

    newTitle = newTitle.trimmed();

    QNetworkRequest req(QUrl(m_baseUrl + QStringLiteral("/sessions/") + id + QStringLiteral("/rename")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QJsonObject body;
    body[QStringLiteral("title")] = newTitle;
    QNetworkReply *reply = m_net->put(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, id, newTitle]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString errBody = reply->readAll();
            QString msg = QStringLiteral("HTTP %1: %2").arg(code).arg(errBody.isEmpty() ? reply->errorString() : errBody);
            QMessageBox::warning(this, QStringLiteral("Rename Failed"), msg);
            return;
        }
        for (int i = 0; i < m_list->count(); ++i) {
            QListWidgetItem *it = m_list->item(i);
            if (it && it->data(Qt::UserRole).toString() == id) {
                applyRenameToItem(it, newTitle);
                break;
            }
        }
        emit sessionRenamed(id, newTitle);
    });
}

void SessionListPanel::applyRenameToItem(QListWidgetItem *item, const QString &newTitle)
{
    if (!item)
        return;
    item->setText(newTitle);
}
