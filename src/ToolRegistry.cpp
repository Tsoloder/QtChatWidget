#include "ToolRegistry.h"

ToolRegistry::ToolRegistry(QObject *parent)
    : QObject(parent)
{
    registerBuiltinTools();
}

ToolRegistry *ToolRegistry::instance()
{
    static ToolRegistry *s_instance = nullptr;
    if (!s_instance)
        s_instance = new ToolRegistry;
    return s_instance;
}

void ToolRegistry::registerBuiltinTools()
{
    QStringList builtinTools = QStringList()
        << "read_file" << "write_file" << "edit_file" << "search_code"
        << "run_command" << "list_directory" << "file_info"
        << "git_status" << "git_diff" << "git_log"
        << "web_search" << "web_fetch";

    QStringList descriptions = QStringList()
        << "读取文件内容" << "写入文件内容" << "编辑文件" << "在代码中搜索"
        << "执行命令" << "列出目录内容" << "获取文件信息"
        << "Git 状态" << "Git 差异" << "Git 历史"
        << "网页搜索" << "网页内容获取";

    for (int i = 0; i < builtinTools.size(); ++i) {
        ToolInfo t;
        t.name = builtinTools[i];
        t.description = i < descriptions.size() ? descriptions[i] : QString();
        t.builtin = true;
        t.category = t.name.startsWith(QLatin1String("git")) ? QStringLiteral("Git")
                   : t.name.startsWith(QLatin1String("web")) ? QStringLiteral("Web")
                   : QStringLiteral("File");
        m_tools.insert(t.name, t);
    }
}

void ToolRegistry::registerTool(const ToolInfo &tool)
{
    if (tool.name.isEmpty())
        return;
    m_tools.insert(tool.name, tool);
}

void ToolRegistry::unregisterTool(const QString &name)
{
    m_tools.remove(name);
}

bool ToolRegistry::hasTool(const QString &name) const
{
    return m_tools.contains(name);
}

ToolInfo ToolRegistry::toolByName(const QString &name) const
{
    return m_tools.value(name);
}

QStringList ToolRegistry::allToolNames() const
{
    return m_tools.keys();
}

QList<ToolInfo> ToolRegistry::allTools() const
{
    return m_tools.values();
}

bool ToolRegistry::isToolAllowed(const QString &toolName, const QStringList &allowedTools) const
{
    if (allowedTools.isEmpty())
        return true;
    if (allowedTools.contains(QStringLiteral("*")))
        return true;
    for (const QString &pattern : allowedTools) {
        if (pattern == toolName)
            return true;
        if (pattern.endsWith(QLatin1Char('*'))) {
            QString prefix = pattern.left(pattern.length() - 1);
            if (toolName.startsWith(prefix))
                return true;
        }
    }
    return false;
}

QStringList ToolRegistry::filterAllowedTools(const QStringList &toolNames, const QStringList &allowedTools) const
{
    QStringList result;
    for (const QString &name : toolNames) {
        if (isToolAllowed(name, allowedTools))
            result.append(name);
    }
    return result;
}
