#pragma once

#include <QMainWindow>
#include "ChatWidget.h"
#include "Theme.h"

// 兼容性外壳：内部托管一个 ChatWidget。
//
// 如果你只是想独立运行 demo，可以直接用 MainWindow。
// 如果要嵌入到自己的程序里（QDockWidget / QSplitter / QTabWidget），
// 请直接使用 ChatWidget，不要用这个 MainWindow。
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    // 透传到内部 ChatWidget
    void setTheme(ThemeId id) { m_chat->setTheme(id); }
    ThemeId currentTheme() const { return m_chat->currentTheme(); }
    ChatWidget *chatWidget() const { return m_chat; }

private:
    ChatWidget *m_chat;
};
