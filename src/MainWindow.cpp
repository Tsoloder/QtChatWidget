#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chat(new ChatWidget(this))
{
    setWindowTitle(QStringLiteral("AI Chat Assistant // Qt 5.12 + VS2022"));
    resize(940, 720);
    setCentralWidget(m_chat);
}
