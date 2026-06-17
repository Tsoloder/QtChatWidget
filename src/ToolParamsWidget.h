#pragma once

#include <QWidget>
#include <QVector>
#include "ContentSegment.h"

class QTableWidget;

// Editable parameter table for a tool call.
// Columns: Name (read-only) | Description (read-only) | Value (editable).
// A "Confirm" button below the table emits paramsConfirmed with the (possibly
// edited) values. After confirmation the table is locked.
class ToolParamsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolParamsWidget(const QString &toolName,
                              const QVector<ContentSegment::Param> &params,
                              QWidget *parent = nullptr);

signals:
    void paramsConfirmed(const QVector<ContentSegment::Param> &params);

private slots:
    void onConfirm();

private:
    QTableWidget *m_table;
    QString m_toolName;
};
