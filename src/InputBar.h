#pragma once

#include <QWidget>
#include <QString>

class QTextEdit;
class QPushButton;

// Bottom input area: multi-line editor + Send button.
// Enter sends, Shift+Enter inserts a newline.
class InputBar : public QWidget
{
    Q_OBJECT
public:
    explicit InputBar(QWidget *parent = nullptr);

signals:
    void send(const QString &text);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSend();

private:
    QTextEdit *m_edit;
};
