#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

struct ToolInfo {
    QString name;
    QString description;
    QString category;
    bool builtin;

    ToolInfo() : builtin(false) {}
};

class ToolRegistry : public QObject
{
    Q_OBJECT
public:
    static ToolRegistry *instance();

    void registerTool(const ToolInfo &tool);
    void unregisterTool(const QString &name);
    bool hasTool(const QString &name) const;
    ToolInfo toolByName(const QString &name) const;
    QStringList allToolNames() const;
    QList<ToolInfo> allTools() const;

    bool isToolAllowed(const QString &toolName, const QStringList &allowedTools) const;
    QStringList filterAllowedTools(const QStringList &toolNames, const QStringList &allowedTools) const;

private:
    explicit ToolRegistry(QObject *parent = nullptr);
    void registerBuiltinTools();

    QMap<QString, ToolInfo> m_tools;
};
