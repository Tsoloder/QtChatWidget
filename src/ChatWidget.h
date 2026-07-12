#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QList>
#include "ChatBubble.h"
#include "ContentSegment.h"
#include "Theme.h"
#include "Skill.h"

class InputBar;
class QComboBox;
class SkillManager;
class SkillPicker;

// 可嵌入的聊天面板：一个独立的 QWidget，可以放进 QDockWidget、
// QSplitter、QTabWidget 或任何布局里。
//
// 与 MainWindow 的区别：
//   - 基类是 QWidget 而非 QMainWindow，不带菜单栏/状态栏
//   - 主题 palette 只作用于本 widget 子树，不污染 qApp
//   - 对外暴露 addBubble() / setTheme() / clear() 等接口，方便宿主程序驱动
class ChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChatWidget(QWidget *parent = nullptr);

    // 运行时切换主题（QPalette + QSS 都会刷新）
    void setTheme(ThemeId id);
    ThemeId currentTheme() const { return m_themeId; }

    // 是否显示顶部标题栏（含主题切换下拉框）。默认显示。
    // 嵌入到 DockWidget 时如果宿主已有自己的标题，可以关掉。
    void setHeaderVisible(bool visible);
    bool isHeaderVisible() const { return m_headerVisible; }

    SkillManager *skillManager() const { return m_skillManager; }

    // 所有已添加的消息气泡（用于导出）
    QList<ChatBubble*> bubbles() const { return m_bubbles; }
    // 禁用/恢复输入栏（流式期间防重复发送）
    void setInputBusy(bool busy);
    InputBar *inputBar() const { return m_input; }

public slots:
    // 追加一条消息气泡
    void addBubble(ChatBubble::Role role, const ContentSegments &segments);
    // 清空所有气泡
    void clear();
    // 滚动到底部
    void scrollToEnd();
    // 设置状态文本
    void setStatusText(const QString &text);

signals:
    // 用户在输入框点发送。宿主程序接到后通常调用 LLM，再把回复 addBubble 回来
    void messageSent(const QString &text);
    void messageSentWithSkills(const QString &text, const QJsonArray &selectedSkills);

    // 用户在气泡内的交互
    void optionSelected(ChatBubble *bubble, int index, const QString &text);
    void toolApproved(ChatBubble *bubble, bool approved, bool alwaysAllow);
    void paramsConfirmed(ChatBubble *bubble, const QVector<ContentSegment::Param> &params);

    // 统一的字符串输出信号：所有交互都以 JSON 字符串形式发出，
    // 宿主程序只需连接这一个信号即可拿到所有用户操作。
    //   type: "message_sent" | "option_selected" | "tool_approved" | "params_confirmed"
    //   jsonPayload: 对应的 JSON 内容对象
    void actionTriggered(const QString &type, const QString &jsonPayload);

    // 主题切换时发出，供 SessionListPanel/SkillPicker 联动
    void themeChanged(ThemeId id);

public slots:
    void appendStreamChunk(const QString &delta);
    void finishStream();
    // 用解析后的 ContentSegments 替换流式纯文本（无工具调用时用）
    void finishStreamWithSegments(const ContentSegments &segs);
    // 加载历史消息（从会话恢复时）
    void loadMessages(const QJsonArray &messages);
    // 流中途失败时调用：删除流式气泡但不添加正式回复（让位给错误气泡）
    void abortStream();

private slots:
    void onSend(const QString &text);
    void onSkillActivated(const Skill &skill);

private:
    QScrollArea *m_scroll = nullptr;
    QVBoxLayout *m_chatLayout = nullptr;
    InputBar *m_input = nullptr;
    QLabel *m_status = nullptr;
    QComboBox *m_themeCombo = nullptr;
    QFrame *m_header = nullptr;
    ChatBubble *m_streamBubble = nullptr;
    QString m_streamText;
    QList<ChatBubble*> m_bubbles;
    ThemeId m_themeId = ThemeId::OneDarkPro;
    bool m_headerVisible = true;
    SkillManager *m_skillManager = nullptr;

    void applyPalette(const Theme &t);
    void applyStyleSheet(const Theme &t);
    // 递归把 palette 应用到本 widget 及所有子部件
    void propagatePalette(const QPalette &pal, QWidget *w);
};
