# ClineLikeChat — Qt 聊天面板组件

一个仿 VSCode Cline 插件风格的 Qt 聊天面板，可显示代码、把模型选项渲染成按钮、提供工具参数表格供用户确认。支持 5 套主题，可作为一个 `QWidget` 嵌入到任意宿主程序（`QDockWidget` / `QSplitter` / `QTabWidget` 等）。

- 目标环境：Qt 5.12.2 + VS2022（开发环境 Qt 5.15 + Linux 也可编译）
- 构建系统：CMake
- 编码：所有源码 UTF-8（含 BOM），MSVC 下通过 `/utf-8` 强制按 UTF-8 编译

> 本项目使用 TRAE + GLM-5.2 制作

---

## 一、整体架构

```
┌─────────────────────────────────────────────────────────┐
│  宿主程序（你自己的 QMainWindow / QDockWidget）          │
│                                                          │
│   ┌───────────────────────────────────────────────────┐ │
│   │  ChatWidget  (src/ChatWidget.h)                    │ │
│   │  —— 对外入口，可嵌入任意布局                         │ │
│   │                                                    │ │
│   │   ┌──────────────────────────────────────────┐    │ │
│   │   │  QScrollArea                             │    │ │
│   │   │   ┌──────────────────────────────────┐   │    │ │
│   │   │   │  ChatBubble  (可有多条)            │   │    │ │
│   │   │   │   ├─ QLabel        (Text 段)       │   │    │ │
│   │   │   │   ├─ CodeEditor     (Code 段)      │   │    │ │
│   │   │   │   ├─ OptionsWidget  (Options 段)   │   │    │ │
│   │   │   │   ├─ ToolParamsWidget(ToolParams)  │   │    │ │
│   │   │   │   └─ 工具批准面板   (ToolApproval) │   │    │ │
│   │   │   └──────────────────────────────────┘   │    │ │
│   │   └──────────────────────────────────────────┘    │ │
│   │   ┌──────────────────────────────────────────┐    │ │
│   │   │  InputBar  (输入框 + 发送按钮)            │    │ │
│   │   └──────────────────────────────────────────┘    │ │
│   └───────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
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
| `src/InputBar.h/.cpp` | 底部输入框，Enter 发送 / Shift+Enter 换行 |
| `src/Theme.h/.cpp` | 5 套主题的颜色 token 定义 + `themeById()` 工厂 |
| `src/ReplyParser.h/.cpp` | **模型回复解析器**。把 LLM 原始文本解析成 `ContentSegments` |
| `src/MainWindow.h/.cpp` | 兼容外壳，内部托管一个 `ChatWidget`，仅用于独立 demo |

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

## 九、构建说明

### 9.1 CMake 配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(ClineLikeChat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core)

# 强制源码按 UTF-8 编码编译（MSVC 默认按系统 ANSI 解析，会导致中文乱码）
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/execution-charset:utf-8)
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
)
target_include_directories(ClineLikeChat PRIVATE src)
target_link_libraries(ClineLikeChat PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core)
```

### 9.2 构建步骤

```bash
mkdir build && cd build
cmake .. -DQt5_DIR=/path/to/Qt/5.12.2/msvc2017_64/lib/cmake/Qt5
cmake --build . --config Release
```

### 9.3 需要加入现有项目时

把 `src/` 目录下所有文件加入你的工程，确保：
- `CMAKE_AUTOMOC ON`（处理 Q_OBJECT 宏）
- MSVC 下加 `/utf-8` 编译选项
- 链接 `Qt5::Widgets Qt5::Gui Qt5::Core`

然后在你的代码里 `#include "ChatWidget.h"` 即可使用。

---

## 十、注意事项

1. **源码编码**：所有 `.cpp/.h` 文件均为 UTF-8（含 BOM），MSVC 下必须配合 `/utf-8` 编译选项，否则中文会乱码。

2. **主题与气泡创建顺序**：`setTheme` 只对当时已存在的子部件生效。`ChatWidget::addBubble` 已内部处理——会对新气泡传播当前 palette。但如果你直接操作气泡内部的子部件，需注意这点。

3. **Palette 隔离**：`ChatWidget` 的 palette 只作用于自身子树，不会调 `qApp->setPalette()`，因此不会污染宿主程序里其他窗口的外观。

4. **QSS 对原生 viewport 的局限**：`QTableWidget` / `QComboBox` / `QHeaderView` 的 viewport 不完全受 QSS 控制，必须靠 `QPalette` 设置底色。这也是为什么主题切换时除了 QSS 还要 `applyPalette`。

5. **ReplyParser 的容错**：如果模型返回的 JSON 解析失败，或者根本没有 JSON 块，`parseAssistantReply` 会把原始回复当作一条 `Text` 段返回，不会出现空气泡。
