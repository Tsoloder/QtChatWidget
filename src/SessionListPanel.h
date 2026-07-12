#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QNetworkAccessManager>
#include "Theme.h"

class SessionListPanel : public QDockWidget
{
    Q_OBJECT
public:
    explicit SessionListPanel(QWidget *parent = nullptr);

    void setNetworkManager(QNetworkAccessManager *net) { m_net = net; }
    void setBaseUrl(const QString &url) { m_baseUrl = url; }
    void setTheme(ThemeId id);

    void refresh();

signals:
    void sessionSelected(const QString &id);
    void newSessionRequested();
    void deleteSessionRequested(const QString &id);
    void sessionRenamed(const QString &id, const QString &newTitle);

private slots:
    void onNewClicked();
    void onRefreshClicked();
    void onDeleteClicked();
    void onItemClicked(QListWidgetItem *item);
    void onItemDoubleClicked(QListWidgetItem *item);
    void onContextMenu(const QPoint &pos);
    void onRenameAction();

private:
    void renameSession(const QString &id, const QString &oldTitle);
    void applyRenameToItem(QListWidgetItem *item, const QString &newTitle);

    QListWidget *m_list;
    QPushButton *m_newBtn;
    QPushButton *m_refreshBtn;
    QPushButton *m_delBtn;
    QLabel *m_titleBar = nullptr;
    QNetworkAccessManager *m_net = nullptr;
    QString m_baseUrl;
    ThemeId m_theme = ThemeId::OneDarkPro;
};
