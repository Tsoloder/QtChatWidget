# ClineLikeChat — Qt + Python 混合架构 Coding Agent

一个仿 VSCode Cline 插件风格的 Coding Agent，采用 **C++ Qt UI + Python FastAPI 后端** 的混合架构。Qt 端负责聊天界面、Skill 系统与主题；Python 端负责 LLM 推理、MCP 工具调用、上下文压缩与会话持久化。两端通过本地 HTTP + SSE 通信。

集成 **Skill 系统**、**LLM 双协议客户端**、**流式输出**、**MCP 工具自动执行**、**会话持久化与历史恢复**、**上下文压缩** 等特性。支持 5 套主题，可作为一个 `QWidget` 嵌入到任意宿主程序。

- 目标环境：Qt 5.12.2 + VS2022（开发环境 Qt 5.15 + Linux 也可编译）+ Python 3.9+
- 构建系统：CMake（C++）+ pip（Python 依赖）
- 编码：所有源码 UTF-8（含 BOM），MSVC 下通过 `/utf-8` 强制按 UTF-8 编译
- **LLM 兼容**：OpenAI / Anthropic 双协议，由 Python 后端处理流式输出（SSE）
- **Skill 系统**：5 个内置 Skill + 用户自定义，C++ 端关键词匹配路由（无 LLM 二次调用）
- **MCP 工具**：通过 FastMCP 桥接，LLM 触发 tool_call 后自动执行，结果回流进同一气泡
- **会话持久化**：左侧 SessionListPanel 列出全部会话，关闭重开程序可恢复历史

> 本项目使用 TRAE + GLM-5.2 制作

---

## 快速开始

```powershell
# 1. 安装 Python 依赖（首次）
.\run.ps1 pyinstall

# 2. 一键运行（自动配置 Qt DLL 路径 + 构建 + 启动 Qt + 拉起 Python 后端）
.\run.ps1 chat

# 检查 Python 依赖是否就绪
.\run.ps1 pycheck

# 生成集成截图
.\run.ps1 shot

# 构建 + 截图 + 自动打开
.\run.ps1 all
```

启动后 Qt 主程序会自动在后台拉起 `py/agent/app.py`（随机空闲端口），状态栏依次显示 `STARTING BACKEND...` → `READY`。首次运行前请在 **File → Settings** (Ctrl+,) 中配置 API 信息，配置会通过 `POST /config` 推送到 Python 后端。

---

## 一、整体架构

```
┌──────────────────────────────────────────────────────────────┐
│  Qt 主程序 (ClineLikeChat.exe)                                │
│                                                                │
│   ┌──────────────────────────────────────────────────────┐   │
│   │  MainWindow                                            │   │
│   │   ├─ ChatWidget (中央)                                 │   │
│   │   │   ├─ ChatBubble * N                                 │   │
│   │   │   └─ InputBar (+ SkillPicker)                      │   │
│   │   ├─ SessionListPanel (左 QDockWidget)                 │   │
│   │   ├─ PythonProcess (QProcess 拉起/健康检查/自动重启)   │   │
│   │   └─ LLMClient (HTTP + SSE 客户端)                     │   │
│   └──────────────────────────────────────────────────────┘   │
│              │ HTTP POST /chat/stream                         │
│              │ SSE  event: text_chunk / tool_call / done      │
│              ▼                                                │
└──────────────┼────────────────────────────────────────────────┘
               │
   ┌───────────▼───────────────────────────────────┐
   │  Python 后端 (py/agent/app.py, FastAPI)        │
   │   ├─ /health        健康检查                   │
   │   ├─ /config        POST ApiConfig             │
   │   ├─ /sessions      CRUD 会话                  │
   │   └─ /chat/stream   Agent Loop (SSE)           │
   │       ├─ agent_loop.py    LLM↔tool 循环         │
   │       ├─ llm_client.py    OpenAI/Anthropic 双协议 │
   │       ├─ context.py       4 层上下文压缩         │
   │       ├─ session.py       会话持久化 (atomic)   │
   │       ├─ mcp_bridge.py    FastMCP 工具桥接       │
   │       └─ retry.py         指数退避重试           │
   └───────────────────────────────────────────────┘
                │ stdio
                ▼
   ┌───────────────────────────────────────────────┐
   │  py/server.py (MCP 工具服务: get_weather 等)   │
   └───────────────────────────────────────────────┘
```

### 文件结构

| 文件 | 职责 |
|------|------|
| `src/ChatWidget.h/.cpp` | **对外主入口**。管理滚动区、输入栏、主题切换，对外暴露 `addBubble` / `setTheme` 等接口 |
| `src/ChatBubble.h/.cpp` | 单条消息气泡。根据 `ContentSegments` 依次渲染各段内容 |
| `src/ContentSegment.h` | **核心数据模型**。定义一条消息由哪些"段"组成 |
| `src/CodeEditor.h/.cpp` | 代码块：头部（语言标签 + 复制按钮）+ 带语法高亮的编辑器 |
| `src/SyntaxHighlighter.h/.cpp` | 多语言语法高亮（cpp/python/js/json/bash 等） |
| `src/OptionsWidget.h/.cpp` | 选项按钮列表，每个按钮按 `style` 配色不同 |
| `src/ToolParamsWidget.h/.cpp` | 工具参数表格，只有 Value 列可编辑，下方有确认按钮 |
| `src/InputBar.h/.cpp` | 底部输入框 + Skill 触发（`/` 弹出 SkillPicker）+ 多 Skill 叠加管理 |
| `src/Theme.h/.cpp` | 5 套主题的颜色 token 定义 + `themeById()` 工厂 |
| `src/ReplyParser.h/.cpp` | **模型回复解析器**。把 LLM 原始文本解析成 `ContentSegments` |
| `src/MainWindow.h/.cpp` | 主窗口：集成 PythonProcess + LLMClient + SessionListPanel + Skill 路由 |
| **Python 进程 & 会话面板** | |
| `src/PythonProcess.h/.cpp` | QProcess 拉起 Python 后端，500ms 健康轮询，崩溃自动重启（max 3） |
| `src/SessionListPanel.h/.cpp` | 左侧 QDockWidget：列出/新建/删除会话，点击切换 |
| **Skill 系统** | |
| `src/Skill.h/.cpp` | Skill 数据结构 + SkillParam 参数定义 |
| `src/SkillManager.h/.cpp` | Skill 加载/搜索/匹配/排序/统计，`matchByKeywords()` 关键词路由 |
| `src/SkillMdParser.h/.cpp` | SKILL.md 文件解析器（YAML frontmatter + 正文） |
| `src/SkillPicker.h/.cpp` | Skill 选择器（富文本显示 + 实时过滤） |
| `src/SkillParamsDialog.h/.cpp` | Skill 参数输入对话框 |
| `src/ToolRegistry.h/.cpp` | 工具元数据注册 + allowed-tools 权限校验 |
| **LLM 集成（HTTP 客户端）** | |
| `src/LLMClient.h/.cpp` | **HTTP + SSE 客户端**：POST /chat/stream，解析多行 `data:` 事件，分发 streamChunk/toolCallReceived 等信号 |
| `src/SettingsDialog.h/.cpp` | API 配置对话框（URL/Key/Model/Type），持久化到 `config.json`，启动时 POST 给 Python |
| **Python 后端** | |
| `py/agent/app.py` | FastAPI 入口：/health /config /sessions CRUD /chat/stream (SSE) |
| `py/agent/agent_loop.py` | Agent Loop：LLM→tool_call→MCP execute→tool_result→LLM，max 20 轮 |
| `py/agent/llm_client.py` | OpenAI/Anthropic 双协议流式，tool_call 分片累积 |
| `py/agent/context.py` | TokenCounter + 4 层上下文压缩（truncate/snip/microcompact/auto-compact） |
| `py/agent/session.py` | Session dataclass + atomic_write + CRUD |
| `py/agent/mcp_bridge.py` | McpBridge 单例：asyncio.Lock 保护 connect/disconnect/call_tool |
| `py/agent/config.py` / `retry.py` / `logging_setup.py` | 配置持久化 / 指数退避 / 日志 |
| `py/server.py` | MCP 工具服务（示例：get_weather, get_year） |
| **资源** | |
| `resources/skills/*/SKILL.md` | 5 个内置 Skill 定义文件 |
| `run.ps1` | 一键运行脚本（构建+启动+pycheck+pyinstall） |

### 核心数据流

```
用户输入 ──► InputBar::send(text)
                │
                ▼
        ChatWidget::messageSent(text)  ──► 宿主程序调用 LLM
                                                │
                                                ▼
                                    LLM 返回原始文本 rawReply
                                                │
                                                ▼
                          ContentSegments segs = parseAssistantReply(rawReply)
                                                │
                                                ▼
                          ChatWidget::addBubble(Assistant, segs)
                                                │
                                                ▼
                          ChatBubble 按 segs 顺序渲染各段
                                                │
                ┌───────────────────────────────┴───────────────────┐
                ▼                       ▼                           ▼
        optionSelected(...)     toolApproved(...)           paramsConfirmed(...)
        (用户点了选项按钮)       (用户批准/拒绝工具)          (用户确认参数表)
                │                       │                           │
                └───────────────────────┴───────────────────────────┘
                                        │
                                        ▼
                              宿主程序继续驱动后续流程
```

---

## 二、内容段模型（ContentSegment）

一条消息（气泡）= 多个 `ContentSegment` 从上到下排列。这是整个组件的核心数据结构。

```cpp
struct ContentSegment
{
    enum Type {
        Text,          // 纯文本/富文本
        Code,          // 带语法高亮的代码块
        Options,       // 模型给出的可选项，渲染成按钮
        ToolApproval,  // 工具调用，等待用户批准
        ToolParams     // 可编辑参数表，等待用户确认
    };

    Type type = Text;

    // Text / Code
    QString text;
    QString language;          // 仅 Code（cpp, python, javascript, json, bash, ...）

    // Options：结构化选项
    struct Option {
        QString label;   // 按钮上显示的文字
        QString value;   // 用户点击后回传给业务的值（可与 label 不同）
        QString style;   // primary | default | danger | success（控制按钮配色）
    };
    QVector<Option> options;

    // ToolApproval
    QString toolName;
    QString toolDescription;

    // ToolParams：一行 = {名称, 描述, 值}。只有值可编辑。
    struct Param {
        QString name;
        QString description;
        QString value;
    };
    QVector<Param> params;
};

using ContentSegments = QVector<ContentSegment>;
```

**段顺序即显示顺序**：你把 `ContentSegments` 按 `Text → Code → ToolParams → Options` 的顺序 append，气泡里就从上到下这样显示。

---

## 三、ChatWidget 公开接口

`ChatWidget` 是对外主入口，基类 `QWidget`，可嵌入任何布局。

```cpp
class ChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChatWidget(QWidget *parent = nullptr);

    // 运行时切换主题（QPalette + QSS 都会刷新）
    void setTheme(ThemeId id);
    ThemeId currentTheme() const;

    // 是否显示顶部标题栏（含主题切换下拉框）。默认显示。
    // 嵌入 DockWidget 时如果宿主已有标题，可关掉。
    void setHeaderVisible(bool visible);

public slots:
    // 追加一条消息气泡
    void addBubble(ChatBubble::Role role, const ContentSegments &segments);
    // 清空所有气泡
    void clear();
    // 滚动到底部
    void scrollToEnd();

signals:
    // 用户在输入框点发送。宿主接到后调用 LLM，再把回复 addBubble 回来
    void messageSent(const QString &text);

    // 用户在气泡内的交互
    void optionSelected(ChatBubble *bubble, int index, const QString &value);
    void toolApproved(ChatBubble *bubble, bool approved, bool alwaysAllow);
    void paramsConfirmed(ChatBubble *bubble, const QVector<ContentSegment::Param> &params);
};
```

### Role 枚举

```cpp
ChatBubble::User       // 用户消息（右侧风格）
ChatBubble::Assistant  // 助手消息（左侧风格）
```

---

## 四、快速集成示例

### 4.1 嵌入到 QDockWidget（推荐方式）

```cpp
// my_ide.cpp
#include "ChatWidget.h"
#include "ContentSegment.h"
#include "ReplyParser.h"

class MyIDE : public QMainWindow {
    Q_OBJECT
public:
    MyIDE(QWidget *parent = nullptr) : QMainWindow(parent) {
        // ... 你的中央编辑器、菜单栏等 ...

        // 把 ChatWidget 嵌入右侧 Dock
        auto *dock = new QDockWidget(QStringLiteral("AI Assistant"), this);
        m_chat = new ChatWidget(dock);
        m_chat->setHeaderVisible(false);  // dock 已有标题栏
        dock->setWidget(m_chat);
        addDockWidget(Qt::RightDockWidgetArea, dock);

        // 连接信号
        connect(m_chat, &ChatWidget::messageSent, this, &MyIDE::onUserSent);
        connect(m_chat, &ChatWidget::optionSelected, this, &MyIDE::onOptionSelected);
        connect(m_chat, &ChatWidget::paramsConfirmed, this, &MyIDE::onParamsConfirmed);
        connect(m_chat, &ChatWidget::toolApproved, this, &MyIDE::onToolApproved);

        // 设置主题
        m_chat->setTheme(ThemeId::OneDarkPro);
    }

private slots:
    void onUserSent(const QString &text) {
        // 1. 把用户消息上屏
        ContentSegments userSegs;
        ContentSegment u;
        u.type = ContentSegment::Text;
        u.text = text;
        userSegs.append(u);
        m_chat->addBubble(ChatBubble::User, userSegs);

        // 2. 调用 LLM（伪代码）
        QString rawReply = callLLM(text);

        // 3. 解析回复并上屏
        ContentSegments replySegs = parseAssistantReply(rawReply);
        m_chat->addBubble(ChatBubble::Assistant, replySegs);
    }

    void onOptionSelected(ChatBubble *bubble, int index, const QString &value) {
        // 用户点了第 index 个选项，value 是回传值
        qDebug() << "Option selected:" << index << value;
    }

    void onParamsConfirmed(ChatBubble *bubble, const QVector<ContentSegment::Param> &params) {
        // 用户确认了参数表，params 里是（可能被编辑过的）最终值
        for (const auto &p : params)
            qDebug() << p.name << "=" << p.value;
    }

    void onToolApproved(ChatBubble *bubble, bool approved, bool alwaysAllow) {
        if (approved) executeTool();
    }

private:
    ChatWidget *m_chat = nullptr;
};
```

### 4.2 手动构造消息（不经过解析器）

如果你不想用 `parseAssistantReply`，可以直接构造 `ContentSegments`：

```cpp
ContentSegments segs;

// 文字说明
ContentSegment t;
t.type = ContentSegment::Text;
t.text = QStringLiteral("这是优化后的实现：");
segs.append(t);

// 代码块
ContentSegment c;
c.type = ContentSegment::Code;
c.language = "cpp";
c.text = QStringLiteral("void foo() { /* ... */ }");
segs.append(c);

// 参数表（用户可编辑 Value 列）
ContentSegment tp;
tp.type = ContentSegment::ToolParams;
tp.toolName = QStringLiteral("edit_file");
tp.params = {
    {QStringLiteral("path"),   QStringLiteral("目标文件路径"), QStringLiteral("src/main.cpp")},
    {QStringLiteral("backup"), QStringLiteral("是否备份"),     QStringLiteral("true")},
};
segs.append(tp);

// 选项按钮（位于表格之后）
ContentSegment o;
o.type = ContentSegment::Options;
o.options.append({QStringLiteral("应用此修改"), QStringLiteral("apply"),  QStringLiteral("primary")});
o.options.append({QStringLiteral("取消"),       QStringLiteral("cancel"), QStringLiteral("danger")});
segs.append(o);

m_chat->addBubble(ChatBubble::Assistant, segs);
```

---

## 五、模型回复解析（ReplyParser）

### 5.1 约定的回复格式

通过提示词约束模型，让它的回复遵循以下结构：

```
（正文：自然语言 / Markdown，可含 ```cpp 等普通代码块）

（回复最末尾，一个 ```json 代码块：）
```json
{
  "tool_params": {
    "tool": "edit_file",
    "params": [
      {"name": "path", "description": "目标文件路径", "value": "src/main.cpp"}
    ]
  },
  "options": [
    {"label": "应用此修改", "value": "apply", "style": "primary"},
    {"label": "取消", "value": "cancel", "style": "danger"}
  ]
}
```
```

**关键约定**：
- JSON 块必须是回复的**最后一个**内容
- `tool_params`（表格）在前，`options`（按钮）在后
- 两者都是可选的；都没有时不输出 JSON 块

### 5.2 解析函数

```cpp
#include "ReplyParser.h"

ContentSegments parseAssistantReply(const QString &rawReply);
```

解析后返回的段顺序固定为：`Text / Code 段` → `ToolParams 段` → `Options 段`，与显示顺序一致。

### 5.3 推荐的提示词模板

```
你是一个编码助手。你的回复必须遵守以下格式约定：

1. 先用自然语言 / Markdown 回答用户，可以包含普通代码块（如 ```cpp）。
2. 如果本次需要让用户确认工具参数，或者需要用户在若干选项中做选择，
   在回复的【最末尾】追加一个 ```json 代码块，结构如下：

```json
{
  "tool_params": {
    "tool": "工具名称",
    "params": [
      {"name": "参数名", "description": "参数用途说明", "value": "默认值"}
    ]
  },
  "options": [
    {"label": "按钮上显示的文字", "value": "点击后回传的值", "style": "primary"}
  ]
}
```

字段说明：
- tool_params（可选）：渲染成参数表格，用户只能改 value 列。
- options（可选）：渲染成按钮列表，必须排在 tool_params 之后。
- options[].style 只能取以下四个值之一：
    primary  —— 强调操作（如"应用此修改"），高亮配色
    default  —— 普通选项
    danger   —— 危险/取消操作，红色调
    success  —— 成功确认类，绿色调
- options[].label 是给用户看的文字；value 是用户点击后回传给你的标识，
  两者可以不同（例如 label="取消" 但 value="cancel"）。

硬性规则：
- 整个 json 代码块必须是回复的最后一个内容，之后不能再有任何文字。
- tool_params 和 options 都是可选的；都没有时不要输出 json 块。
- 不要在 json 块外面再单独写参数或选项。
```

---

## 六、主题系统

### 6.1 内置主题

| ThemeId | 名称 | 风格 |
|---------|------|------|
| `MilitaryTech` | Military Tech | 军工科技质感（深绿 + 金色） |
| `FutureTechBlue` | Future Tech Blue | 未来科技蓝（深蓝 + 青色） |
| `WhiteMinimal` | White Minimal | 白色简约 |
| `OneDarkPro` | One Dark Pro | VS Code One Dark Pro 风格 |
| `WeChatLight` | WeChat Light | 微信风格白色主题（微信绿） |

### 6.2 切换主题

```cpp
// 运行时切换，QPalette + QSS 都会刷新
m_chat->setTheme(ThemeId::FutureTechBlue);

// 查询当前主题
ThemeId id = m_chat->currentTheme();
```

ChatWidget 自带的顶部下拉框也允许用户手动切换。嵌入 DockWidget 时可通过 `setHeaderVisible(false)` 隐藏整个头部。

### 6.3 主题实现机制

每个主题是一个 `Theme` 结构体，包含约 90 个颜色 token（如 `windowBg` / `userBubbleBg` / `optionBtnBg` 等）。QSS 模板里用 `${token}` 占位符，运行时通过 `QString::replace` 替换成实际颜色值。

- **QSS**：控制边框、圆角、padding、hover/pressed 状态等视觉细节
- **QPalette**：控制 `QTableWidget` / `QComboBox` 等原生 viewport 的底色（QSS 对这些部件的 viewport 不完全生效，必须靠 palette）

`ChatWidget::addBubble` 会对新创建的气泡递归传播当前 palette，确保后创建的 widget（如表格 viewport）不会回退到系统默认白底。

### 6.4 添加自定义主题

1. 在 `Theme.h` 的 `ThemeId` 枚举里加一项
2. 在 `Theme.cpp` 里写一个返回 `Theme` 的工厂函数，填满所有 token
3. 在 `themeById()` 的 switch 里加一个 case
4. （可选）在 `ChatWidget` 构造函数的主题下拉框里 `addItem`

---

## 七、选项按钮样式（Options）

选项按钮通过 `style` 字段控制配色，QSS 会根据 `objectName` 命中不同规则：

| style | objectName | 视觉效果 | 适用场景 |
|-------|-----------|----------|----------|
| `primary` | `#optionBtn_primary` | 强调色背景 + 加粗 | "应用此修改" 等主操作 |
| `default` | `#optionBtn_default` | 普通背景 + 左侧 accent 条 | 普通可选项 |
| `danger` | `#optionBtn_danger` | 红色调 | "取消" / 删除等危险操作 |
| `success` | `#optionBtn_success` | 绿色调 | 成功确认类 |
| （空） | `#optionBtn_default` | 同 default | 兼容旧格式 |

点击按钮后会 emit `optionSelected(bubble, index, value)`，其中 `value` 是选项的回传值（若为空则回传 `label`）。

---

## 八、工具参数表格（ToolParams）

渲染成一个 `QTableWidget`，三列：

| 列 | 可编辑 | 说明 |
|----|--------|------|
| Name | 否 | 参数名 |
| Description | 否 | 参数用途说明 |
| Value | 是 | 参数值（用户可修改） |

表格下方有 "Confirm parameters" 按钮，点击后：
1. 表格锁定（不可再编辑）
2. 按钮禁用
3. emit `paramsConfirmed(bubble, params)`，`params` 里是用户确认后的最终值

---

## 九、Skill 系统

本项目实现了类似 Claude Code 的 Skill 机制，通过 **system prompt 注入** 影响模型行为。

### 9.1 内置 Skill

| Skill ID | 名称 | 用途 |
|----------|------|------|
| `code-review` | Code Review | 代码审查（正确性/安全/性能/可维护性/最佳实践） |
| `refactor` | Refactor | 代码重构建议 |
| `explain-code` | Explain Code | 代码解释 |
| `doc-generate` | Doc Generate | 文档生成 |
| `test-generate` | Test Generate | 测试用例生成 |

### 9.2 SKILL.md 格式

每个 Skill 是一个目录，包含 `SKILL.md` 文件：

```markdown
---
name: code-review
description: Review code for bugs, security, and best practices
aliases:
  - cr
  - review
tags:
  - 审查
  - review
  - code
allowed-tools:
  - read_file
  - search_code
extra_params:
  - name: severity
    description: Review severity level
    default: medium
    required: true
    type: string
---

You are a senior code reviewer. When reviewing code, evaluate these aspects:
1. Correctness & Bugs
2. Security vulnerabilities
3. Performance issues
4. Maintainability
5. Best Practices
Rank findings by severity: critical, high, medium, low.
```

**字段说明**：
- `name` / `description`：基础信息
- `aliases`：触发别名（如 `/cr` 等价于 `/code-review`）
- `tags`：关键词标签，用于自动匹配
- `allowed-tools`：限制 Skill 可调用的工具
- `extra_params`：参数定义，激活时弹出对话框让用户填写
- 正文部分：注入到 system prompt 的指令内容

### 9.3 五大高级特性

#### ① Skill 参数

支持在 SKILL.md 中定义 `extra_params`，激活带参数的 Skill 时弹出对话框：

```cpp
// SkillParamsDialog 收集用户输入 → Skill::resolvedSystemPrompt() 注入参数值
Skill skill = manager.skillById("code-review");
// 用户填入 severity=high, language=cpp
QString prompt = skill.resolvedSystemPrompt();
// prompt 中会包含 "severity: high\nlanguage: cpp"
```

#### ② 多 Skill 叠加

同时激活多个 Skill，system prompt 自动拼接：

```cpp
// InputBar 维护 QList<Skill> m_activeSkills
inputBar->addActiveSkill(codeReviewSkill);
inputBar->addActiveSkill(refactorSkill);
QString combined = inputBar->combinedSystemPrompt();
// = codeReview prompt + "\n\n---\n\n" + refactor prompt
```

输入框上方会显示所有已激活 Skill 的标签，可逐个移除。

#### ③ Skill 路由（关键词匹配）

用户发送消息时，C++ 端 `SkillManager::matchByKeywords(text)` 在所有 Skill 中按 name(10 分)/id(8 分)/alias(8 分)/tag(4 分) 计分，分数 ≥ 4 且为最高分时自动激活该 Skill 的 system prompt，**无需 LLM 二次调用**：

```cpp
// MainWindow.cpp
Skill matched = m_chat->skillManager()->matchByKeywords(text);
QString sysPrompt = matched.isValid() ? matched.resolvedSystemPrompt() : QString();
m_llm->sendMessage(text, sysPrompt, m_currentSessionId);
```

用户也可在输入框键入 `/` 手动激活 Skill（`SkillPicker` 弹出），手动激活优先级高于关键词匹配。

#### ④ 工具调用集成（MCP 自动执行）

Python 后端通过 `mcp_bridge.py` 连接 `py/server.py`（FastMCP stdio 协议）。LLM 在回复中触发 `tool_call` 时，后端自动执行对应工具并把 `tool_result` 回灌给 LLM，循环直到 LLM 不再调用工具（max 20 轮）。整个流程**无需用户审批**，C++ 端通过 SSE 事件在同一个流式气泡内显示 `🔧 调用工具 name(args)` 和 `→ 结果`。

`ToolRegistry` 仍保留用于 Skill 的 `allowed-tools` 元数据校验（提示词层面约束 LLM），但实际工具执行由 Python MCP 桥接完成。

#### ⑤ 使用频率排序

`SkillManager` 记录每个 Skill 的 `useCount`，持久化到 `skill_usage.json`：

```cpp
manager.recordUsage("code-review");  // useCount++
QList<Skill> sorted = manager.allSkillsSorted();  // 按使用频率降序
```

SkillPicker 中常用 Skill 自动置顶显示。

### 9.4 添加自定义 Skill

1. 在 `resources/skills/` 下新建目录，如 `my-skill/`
2. 创建 `SKILL.md` 文件（参考 9.2 格式）
3. 在 `resources.qrc` 中注册：`<file>resources/skills/my-skill/SKILL.md</file>`
4. 重新构建——新 Skill 自动进入路由候选列表

### 9.5 触发方式

| 方式 | 操作 | 说明 |
|------|------|------|
| **手动触发** | 输入 `/` → 弹出 SkillPicker → 选择 | 跳过关键词匹配，直接使用选中 Skill（可叠加多个） |
| **关键词自动匹配** | 直接输入消息（不带 `/`） | `matchByKeywords` 按 name/id/alias/tag 计分，≥4 分自动激活 |

---

## 十、LLM 集成与流式输出（HTTP + SSE）

C++ 端的 `LLMClient` 不再直接调用 LLM API，而是作为 **HTTP + SSE 客户端** 与本地 Python 后端通信。所有 LLM 协议细节（OpenAI/Anthropic 双协议、tool_call 分片累积、上下文压缩）都在 Python 端处理。

### 10.1 通信协议

| 端点 | 方法 | 用途 |
|------|------|------|
| `/health` | GET | 健康检查（PythonProcess 轮询） |
| `/config` | POST | 推送 ApiConfig（api_type/api_url/api_key/model_id） |
| `/sessions` | GET/POST | 列出/新建会话 |
| `/sessions/{id}` | GET/DELETE | 加载/删除会话（含 messages 数组） |
| `/sessions/{id}/clear` | POST | 清空会话消息 |
| `/chat/stream` | POST | **核心**：Agent Loop + SSE 流式输出 |

`POST /chat/stream` 请求体：
```json
{"session_id": "uuid", "message": "用户输入", "system_prompt": "Skill 系统提示词（可选）"}
```

### 10.2 SSE 事件类型

Python 后端通过 `text/event-stream` 返回以下事件，C++ 端 `LLMClient::parseSSEBlock` 解析：

| event | data 字段 | C++ 信号 |
|-------|----------|----------|
| `text_chunk` | `{"delta": "增量文本"}` | `streamChunk(delta)` |
| `tool_call` | `{"name": "get_weather", "args": {...}}` | `toolCallReceived(name, args)` |
| `tool_result` | `{"name": "...", "result": "..."}` | `toolResultReceived(name, result)` |
| `usage` | `{"total": 1234}` | `tokenUsageReceived(total)` |
| `done` | `{"session_id": "...", "tokens": 1234}` | `streamFinished(m_fullText)` |
| `error` | `{"message": "...", "retryable": false}` | `errorOccurred(message)` |

SSE 协议要求多个 `data:` 行累积成一个事件，C++ 端按 `\n\n` 分块解析。

### 10.3 流式 UI 渲染

```cpp
connect(m_llm, &LLMClient::streamStarted, []() { /* 状态栏 STREAMING... */ });
connect(m_llm, &LLMClient::streamChunk, m_chat, &ChatWidget::appendStreamChunk);
connect(m_llm, &LLMClient::toolCallReceived, [](const QString &name, const QString &args) {
    // 在同一流式气泡内追加 "🔧 调用工具 name(args)"
});
connect(m_llm, &LLMClient::streamFinished, [](const QString &fullText) {
    if (m_llm->hadToolCalls()) {
        m_chat->finishStream();              // 工具调用轮次：保留原始流式气泡
    } else {
        auto segs = parseAssistantReply(fullText);
        m_chat->finishStreamWithSegments(segs);  // 普通回复：替换为解析后的段
    }
});
```

**单气泡规则**：一次 `/chat/stream` 请求（可能含多轮 LLM↔tool 循环）的所有 `text_chunk` 和 `tool_call` 都渲染在同一个流式气泡内，`done` 事件到达时才结束。

### 10.4 双协议支持（Python 端）

Python 后端的 `llm_client.py` 同时支持 OpenAI 和 Anthropic：

| 协议 | SDK | 端点 | tool_call 格式 |
|------|-----|------|----------------|
| OpenAI | `openai` Python SDK | `<api_url>/v1/chat/completions` | `tool_calls[].function.arguments` (JSON 字符串分片) |
| Anthropic | `httpx` 直接调用 | `<api_url>/v1/messages` | `content[].type == "tool_use"` |

OpenAI 协议下 URL 自动补 `/v1` 后缀。在 Settings 中切换 API Type 即可，配置通过 `POST /config` 实时推送到 Python 后端，无需重启。

### 10.5 对话历史

会话历史由 Python 端 `session.py` 持久化到 `py/agent/sessions/<uuid>/{meta,messages}.json`。每次 `text_chunk`/`tool_call`/`tool_result`/`done` 事件触发时增量保存（atomic_write：tempfile + os.replace）。重启程序后通过 `GET /sessions/{id}` 恢复历史，`ChatWidget::loadMessages()` 渲染到气泡。

---

## 十一、API 设置

### 11.1 配置对话框

菜单 **File → Settings** (Ctrl+,) 打开配置对话框：

| 字段 | 说明 | 示例 |
|------|------|------|
| API Type | 协议类型 | OpenAI-Compatible / Anthropic |
| API URL | 服务地址 | `https://api.xiaomimimo.com` |
| API Key | 密钥 | `sk-xxxx` |
| Model ID | 模型 ID | `mimo-v2.5-pro` |

### 11.2 配置持久化与推送

配置保存在 exe 同目录的 `config.json`：

```json
{
  "apiType": 0,
  "apiUrl": "https://api.xiaomimimo.com",
  "apiKey": "sk-xxxx",
  "modelId": "mimo-v2.5-pro"
}
```

启动时自动加载，Python 后端就绪后通过 `POST /config` 推送过去。在 Settings 中修改并 Save 后，新配置会立即 POST 给 Python 后端，**无需重启**。

### 11.3 Python 路径配置

`PythonProcess` 自动查找 `python`/`python3`/`py`（Windows）。如需指定，可在 `QSettings("QtChatWidget")` 中写入 `pythonPath`：

```powershell
# 例：使用 Anaconda 的 Python
Set-ItemProperty -Path "HKCU:\Software\QtChatWidget" -Name "pythonPath" -Value "D:\Application\Anaconda\python.exe"
```

或在 Settings 对话框中增加对应字段（如未在 UI 暴露，可直接编辑注册表/配置文件）。

---

## 十二、构建说明

### 12.1 依赖

**C++ 端**：
- Qt 5.12.2+（Widgets / Gui / Core / Svg / Network）
- MSVC 2017+ 或兼容编译器
- CMake 3.10+

**Python 端**：
- Python 3.9+
- 依赖见 `py/agent/requirements.txt`：`fastapi` / `uvicorn` / `openai` / `httpx` / `fastmcp`

```powershell
# 一键安装 Python 依赖
.\run.ps1 pyinstall
# 或手动
pip install -r py/agent/requirements.txt
```

### 12.2 CMake 配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(ClineLikeChat)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core Svg Network)

# 强制源码按 UTF-8 编码编译（MSVC 默认按系统 ANSI 解析，会导致中文乱码）
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(ClineLikeChat WIN32
    main.cpp
    src/MainWindow.cpp
    src/ChatWidget.cpp
    src/ChatBubble.cpp
    src/CodeEditor.cpp
    src/SyntaxHighlighter.cpp
    src/OptionsWidget.cpp
    src/InputBar.cpp
    src/ToolParamsWidget.cpp
    src/Theme.cpp
    src/ReplyParser.cpp
    src/LLMClient.cpp           # HTTP + SSE 客户端
    src/SettingsDialog.cpp      # API 设置对话框
    src/PythonProcess.cpp       # Python 进程管理
    src/SessionListPanel.cpp    # 会话列表面板
    src/Skill.cpp               # Skill 数据结构
    src/SkillManager.cpp        # Skill 管理 + matchByKeywords
    src/SkillMdParser.cpp       # SKILL.md 解析
    src/SkillPicker.cpp         # Skill 选择器
    src/SkillParamsDialog.cpp   # Skill 参数对话框
    src/ToolRegistry.cpp        # 工具注册
)
target_include_directories(ClineLikeChat PRIVATE src)
target_link_libraries(ClineLikeChat PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg Qt5::Network)
```

### 12.3 构建步骤

```bash
mkdir build && cd build
cmake .. -DQt5_DIR=/path/to/Qt/5.12.2/msvc2017_64/lib/cmake/Qt5
cmake --build . --config Release
```

或使用一键脚本：

```powershell
.\run.ps1 build
```

### 12.4 需要加入现有项目时

把 `src/` 目录下所有文件加入你的工程，确保：
- `CMAKE_AUTOMOC ON`（处理 Q_OBJECT 宏）
- MSVC 下加 `/utf-8` 编译选项
- 链接 `Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg Qt5::Network`（Network 用于 LLMClient HTTP 通信）
- 添加 `resources.qrc` 到资源文件
- Python 后端目录 `py/agent/` 需与 exe 路径关系正确（开发态 exe 在 `build/Release/`，会回溯两级查找 `../../py/agent/app.py`）

然后在你的代码里 `#include "ChatWidget.h"` 即可使用。

---

## 十三、注意事项

1. **源码编码**：所有 `.cpp/.h` 文件均为 UTF-8（含 BOM），MSVC 下必须配合 `/utf-8` 编译选项，否则中文会乱码。

2. **主题与气泡创建顺序**：`setTheme` 只对当时已存在的子部件生效。`ChatWidget::addBubble` 已内部处理——会对新气泡传播当前 palette。但如果你直接操作气泡内部的子部件，需注意这点。

3. **Palette 隔离**：`ChatWidget` 的 palette 只作用于自身子树，不会调 `qApp->setPalette()`，因此不会污染宿主程序里其他窗口的外观。

4. **QSS 对原生 viewport 的局限**：`QTableWidget` / `QComboBox` / `QHeaderView` 的 viewport 不完全受 QSS 控制，必须靠 `QPalette` 设置底色。这也是为什么主题切换时除了 QSS 还要 `applyPalette`。

5. **ReplyParser 的容错**：如果模型返回的 JSON 解析失败，或者根本没有 JSON 块，`parseAssistantReply` 会把原始回复当作一条 `Text` 段返回，不会出现空气泡。

6. **Python 进程生命周期**：Qt 主程序通过 `QProcess` 拉起 Python 后端，正常关闭时 `closeEvent` 会 `terminate()` Python；若主程序被 `taskkill /F` 强杀，Python 进程会残留，需手动结束。崩溃自动重启上限 3 次，超过后弹出错误对话框。

7. **Python 后端日志**：`py/agent/agent.log` 记录 LLM 请求、tool_call、压缩事件与异常堆栈，调试问题时优先查看。

8. **端口分配**：PythonProcess 通过 `QTcpServer` 监听随机空闲端口（不固定 8000），避免与已占用端口冲突。C++ 端通过 `ready(quint16 port)` 信号拿到实际端口。
