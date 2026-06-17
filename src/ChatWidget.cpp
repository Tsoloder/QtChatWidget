#include "ChatWidget.h"
#include "ChatBubble.h"
#include "InputBar.h"
#include "ContentSegment.h"
#include "Theme.h"

#include <QScrollBar>
#include <QTimer>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPalette>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
{
    // 根 widget 的 objectName 用于 QSS 命中
    setObjectName(QStringLiteral("chatWidgetRoot"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // header
    m_header = new QFrame;
    m_header->setObjectName("appHeader");
    m_header->setFixedHeight(46);
    auto *hl = new QHBoxLayout(m_header);
    hl->setContentsMargins(16, 0, 16, 0);
    auto *title = new QLabel(QStringLiteral("AI CHAT ASSISTANT"));
    title->setObjectName("appTitle");
    m_status = new QLabel(QStringLiteral("STATUS: READY"));
    m_status->setObjectName("statusLabel");

    m_themeCombo = new QComboBox;
    m_themeCombo->setObjectName("themeCombo");
    m_themeCombo->addItem(QStringLiteral("Military Tech"),    static_cast<int>(ThemeId::MilitaryTech));
    m_themeCombo->addItem(QStringLiteral("Future Tech Blue"), static_cast<int>(ThemeId::FutureTechBlue));
    m_themeCombo->addItem(QStringLiteral("White Minimal"),    static_cast<int>(ThemeId::WhiteMinimal));
    m_themeCombo->addItem(QStringLiteral("One Dark Pro"),     static_cast<int>(ThemeId::OneDarkPro));
    m_themeCombo->addItem(QStringLiteral("WeChat Light"),     static_cast<int>(ThemeId::WeChatLight));
    // 默认选中 OneDarkPro（与 m_themeId 初值一致）
    m_themeCombo->setCurrentIndex(3);
    m_themeCombo->setFixedHeight(28);
    m_themeCombo->setFocusPolicy(Qt::NoFocus);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this](int) { setTheme(static_cast<ThemeId>(m_themeCombo->currentData().toInt())); });

    hl->addWidget(title);
    hl->addStretch();
    hl->addWidget(m_themeCombo);
    hl->addSpacing(12);
    hl->addWidget(m_status);
    outer->addWidget(m_header);

    // chat scroll area
    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setObjectName("chatScroll");
    m_scroll->setFrameShape(QFrame::NoFrame);
    auto *container = new QWidget;
    container->setObjectName("chatContainer");
    m_chatLayout = new QVBoxLayout(container);
    m_chatLayout->setContentsMargins(16, 16, 16, 16);
    m_chatLayout->setSpacing(12);
    m_chatLayout->addStretch();
    m_scroll->setWidget(container);
    outer->addWidget(m_scroll, 1);

    // input
    m_input = new InputBar;
    connect(m_input, &InputBar::send, this, &ChatWidget::onSend);
    outer->addWidget(m_input);

    setCurrentTheme(m_themeId);
    applyPalette(themeById(m_themeId));
    applyStyleSheet(themeById(m_themeId));

    // welcome message
    ContentSegments welcome;
    ContentSegment w;
    w.type = ContentSegment::Text;
    w.text = QStringLiteral(
        "<b>System online.</b> AI coding assistant ready (Cline-like).<br><br>"
        "Capabilities:<br>"
        "&bull; render code with syntax highlighting,<br>"
        "&bull; convert proposed choices into selectable options, and<br>"
        "&bull; request authorization before executing tools, with editable parameters.<br><br>"
        "Switch the visual style from the top-right combo box.");
    welcome.append(w);
    addBubble(ChatBubble::Assistant, welcome);
}

void ChatWidget::setHeaderVisible(bool visible)
{
    if (m_headerVisible == visible)
        return;
    m_headerVisible = visible;
    m_header->setVisible(visible);
}

void ChatWidget::addBubble(ChatBubble::Role role, const ContentSegments &segments)
{
    auto *bubble = new ChatBubble(role, segments);
    connect(bubble, &ChatBubble::optionSelected, this, &ChatWidget::optionSelected);
    connect(bubble, &ChatBubble::toolApproved,   this, &ChatWidget::toolApproved);
    connect(bubble, &ChatBubble::paramsConfirmed,this, &ChatWidget::paramsConfirmed);
    // insert before the trailing stretch
    const int idx = m_chatLayout->count() - 1;
    m_chatLayout->insertWidget(idx, bubble);
    // 对新气泡及其子部件传播当前主题的 palette，避免后创建的 widget
    //（如 QTableWidget viewport）回退到系统默认的白底色
    propagatePalette(palette(), bubble);
    QTimer::singleShot(0, this, [this]() { scrollToEnd(); });
}

void ChatWidget::clear()
{
    // 删除除末尾 stretch 外的所有项
    while (m_chatLayout->count() > 1) {
        QLayoutItem *it = m_chatLayout->takeAt(0);
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }
}

void ChatWidget::scrollToEnd()
{
    auto *bar = m_scroll->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void ChatWidget::onSend(const QString &text)
{
    // 把用户消息上屏
    ContentSegments segs;
    ContentSegment s;
    s.type = ContentSegment::Text;
    s.text = text;
    segs.append(s);
    addBubble(ChatBubble::User, segs);

    m_status->setText(QStringLiteral("STATUS: PROCESSING..."));
    // 通知宿主程序：用户发了一条消息，由宿主去调用 LLM 并把回复 addBubble 回来
    emit messageSent(text);
}

void ChatWidget::setTheme(ThemeId id)
{
    if (id == m_themeId && m_themeCombo)
        return;
    m_themeId = id;
    setCurrentTheme(id);
    if (m_themeCombo) {
        QSignalBlocker b(m_themeCombo);
        for (int i = 0; i < m_themeCombo->count(); ++i) {
            if (static_cast<ThemeId>(m_themeCombo->itemData(i).toInt()) == id) {
                m_themeCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    applyPalette(themeById(id));
    applyStyleSheet(themeById(id));
}

void ChatWidget::propagatePalette(const QPalette &pal, QWidget *w)
{
    // 只设置 widget 自己，不递归到子窗口（QComboBox 的下拉视图等会自动继承）
    w->setPalette(pal);
    const auto kids = w->children();
    for (QObject *o : kids) {
        auto *child = qobject_cast<QWidget*>(o);
        if (child && !child->isWindow())
            propagatePalette(pal, child);
    }
}

void ChatWidget::applyPalette(const Theme &t)
{
    // 注意：这里只作用于本 widget 子树，不调 qApp->setPalette()，
    // 避免污染宿主程序里其他窗口的默认外观。
    QPalette pal;
    const QColor base(t.windowBg);
    const QColor window(t.windowBg);
    const QColor text(t.msgTextColor);
    const QColor btn(t.headerBg);
    const QColor btnText(t.msgTextColor);
    const QColor highlight(t.tableSelBg);
    const QColor highlightedText(t.tableSelText);
    const QColor placeholder(t.statusColor);
    const QColor tooltip(t.assistantBubbleBg);
    const QColor tooltipText(t.msgTextColor);

    pal.setColor(QPalette::Window, window);
    pal.setColor(QPalette::Base, base);                  // table/list/combo viewport bg
    pal.setColor(QPalette::AlternateBase, t.tableAltBg); // alternating row bg
    pal.setColor(QPalette::Text, text);
    pal.setColor(QPalette::ButtonText, btnText);
    pal.setColor(QPalette::Button, btn);
    pal.setColor(QPalette::Highlight, highlight);
    pal.setColor(QPalette::HighlightedText, highlightedText);
    pal.setColor(QPalette::ToolTipBase, tooltip);
    pal.setColor(QPalette::ToolTipText, tooltipText);
    pal.setColor(QPalette::PlaceholderText, placeholder);
    pal.setColor(QPalette::BrightText, t.statusColor);
    pal.setColor(QPalette::WindowText, text);
    pal.setColor(QPalette::Disabled, QPalette::Text, t.disabledText);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, t.disabledText);
    pal.setColor(QPalette::Disabled, QPalette::Base, t.disabledBg);

    propagatePalette(pal, this);
}

void ChatWidget::applyStyleSheet(const Theme &t)
{
    // QSS 模板：与原 MainWindow 完全一致，只是把 QMainWindow 选择器换成
    // QWidget#chatWidgetRoot，避免影响宿主程序里其他 QMainWindow。
    QString qss = QStringLiteral(
        "QWidget#chatWidgetRoot, QWidget#chatContainer, QWidget#inputBar { background: ${windowBg}; }"
        "QFrame#appHeader { background: ${headerBg}; border-bottom: 1px solid ${headerBorder}; }"
        "QLabel#appTitle { color: ${titleColor}; font-size: 14px; font-weight: bold;"
        "  font-family: ${monoFont}; letter-spacing: 2px; }"
        "QLabel#statusLabel { color: ${statusColor}; font-size: 12px; font-family: ${monoFont}; }"
        "QScrollArea#chatScroll { border: none; background: ${windowBg}; }"
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 4px 2px; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: ${scrollHandle}; border: none; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: ${scrollHandleHover}; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"

        "QFrame#userBubble { background: ${userBubbleBg}; border: 1px solid ${userBubbleBorder};"
        "  border-left: 3px solid ${userBubbleAccent}; border-radius: 12px; }"
        "QFrame#assistantBubble { background: ${assistantBubbleBg}; border: 1px solid ${assistantBubbleBorder};"
        "  border-left: 3px solid ${assistantBubbleAccent}; border-radius: 12px; }"
        "QLabel#roleLabel { color: ${roleColor}; font-size: 11px; font-weight: bold; font-family: ${monoFont}; letter-spacing: 1px; }"
        "QLabel#msgText { color: ${msgTextColor}; font-size: 13px; }"

        "QWidget#codeHeader { background: ${codeHeaderBg}; border: 1px solid ${codeHeaderBorder};"
        "  border-bottom: none; border-top-left-radius: 8px; border-top-right-radius: 8px; }"
        "QLabel#codeLang { color: ${codeLangColor}; font-size: 11px; font-family: ${monoFont}; letter-spacing: 1px; }"
        "QPushButton#copyBtn { color: ${copyBtnColor}; font-size: 11px; padding: 2px 10px; font-family: ${monoFont};"
        "  border: 1px solid ${codeHeaderBorder}; border-radius: 10px; }"
        "QPushButton#copyBtn:hover { color: ${copyBtnHoverColor}; background: ${copyBtnHoverBg}; }"
        "QTextEdit#codeEdit { background: ${codeEditBg}; color: ${codeEditTextColor}; border: 1px solid ${codeEditBorder};"
        "  border-top: none; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px; }"

        // 选项按钮：扁平、统一 6px 圆角、无左侧条纹，按主流商业软件风格
        // default：幽灵/次级按钮
        "QPushButton#optionBtn_default { background: ${optionBtnBg}; color: ${optionBtnText};"
        "  border: 1px solid ${optionBtnBorder}; border-radius: 6px; padding: 9px 14px; text-align: left; font-size: 13px; }"
        "QPushButton#optionBtn_default:hover { background: ${optionBtnHoverBg}; border-color: ${optionBtnHoverAccent}; color: ${optionBtnHoverText}; }"
        "QPushButton#optionBtn_default:pressed { background: ${optionBtnPressedBg}; }"
        // primary：实心主操作按钮（如"应用此修改"）
        "QPushButton#optionBtn_primary { background: ${confirmBtnBg}; color: ${confirmBtnText};"
        "  border: 1px solid ${confirmBtnBorder}; border-radius: 6px; padding: 9px 14px; text-align: left; font-size: 13px; font-weight: 600; }"
        "QPushButton#optionBtn_primary:hover { background: ${confirmBtnHoverBg}; border-color: ${confirmBtnHoverBorder}; color: ${confirmBtnHoverText}; }"
        // danger：危险操作（如"取消"）
        "QPushButton#optionBtn_danger { background: ${rejectBtnBg}; color: ${rejectBtnText};"
        "  border: 1px solid ${rejectBtnBorder}; border-radius: 6px; padding: 9px 14px; text-align: left; font-size: 13px; }"
        "QPushButton#optionBtn_danger:hover { background: ${rejectBtnHoverBg}; border-color: ${rejectBtnHoverBorder}; }"
        // success：成功确认类
        "QPushButton#optionBtn_success { background: ${confirmBtnBg}; color: ${confirmBtnText};"
        "  border: 1px solid ${confirmBtnBorder}; border-radius: 6px; padding: 9px 14px; text-align: left; font-size: 13px; }"
        "QPushButton#optionBtn_success:hover { background: ${confirmBtnHoverBg}; border-color: ${confirmBtnHoverBorder}; color: ${confirmBtnHoverText}; }"
        // 兼容旧的 #optionBtn（无 style 后缀）
        "QPushButton#optionBtn { background: ${optionBtnBg}; color: ${optionBtnText};"
        "  border: 1px solid ${optionBtnBorder}; border-radius: 6px; padding: 9px 14px; text-align: left; font-size: 13px; }"
        "QPushButton#optionBtn:hover { background: ${optionBtnHoverBg}; border-color: ${optionBtnHoverAccent}; color: ${optionBtnHoverText}; }"
        "QPushButton#optionBtn:pressed { background: ${optionBtnPressedBg}; }"

        "QFrame#toolPanel { background: ${toolPanelBg}; border: 1px solid ${toolPanelBorder};"
        "  border-radius: 8px; }"
        "QLabel#toolTitle { color: ${toolTitleColor}; font-weight: 600; font-size: 13px; font-family: ${monoFont}; }"
        "QLabel#toolDesc { color: ${toolDescColor}; font-size: 12px; font-family: ${monoFont}; }"
        "QPushButton#approveBtn { background: ${approveBtnBg}; color: ${approveBtnText}; border: 1px solid ${approveBtnBorder};"
        "  border-radius: 6px; padding: 7px 14px; font-family: ${monoFont}; }"
        "QPushButton#approveBtn:hover { background: ${approveBtnHoverBg}; border-color: ${approveBtnHoverBorder}; }"
        "QPushButton#approveBtn:disabled { background: ${disabledBg}; color: ${disabledText}; border-color: ${disabledBorder}; }"
        "QPushButton#rejectBtn { background: ${rejectBtnBg}; color: ${rejectBtnText}; border: 1px solid ${rejectBtnBorder};"
        "  border-radius: 6px; padding: 7px 14px; font-family: ${monoFont}; }"
        "QPushButton#rejectBtn:hover { background: ${rejectBtnHoverBg}; border-color: ${rejectBtnHoverBorder}; }"
        "QPushButton#rejectBtn:disabled { background: ${disabledBg}; color: ${disabledText}; border-color: ${disabledBorder}; }"

        "QTableWidget#paramTable { background: ${tableBg}; alternate-background-color: ${tableAltBg};"
        "  color: ${tableText}; border: 1px solid ${tableBorder};"
        "  border-radius: 6px; selection-background-color: ${tableSelBg}; selection-color: ${tableSelText}; }"
        "QTableWidget#paramTable::item { padding: 6px 8px; border: none; }"
        "QTableWidget#paramTable::item:editable { background: ${tableEditBg}; }"
        "QTableWidget#paramTable::item:selected { background: ${tableSelBg}; color: ${tableSelText}; }"
        "QHeaderView::section { background: ${headerSectionBg}; color: ${headerSectionColor}; padding: 7px 8px;"
        "  border: none; border-right: 1px solid ${headerSectionBorder}; border-bottom: 1px solid ${headerSectionBorder};"
        "  font-weight: 600; font-family: ${monoFont}; }"
        "QTableCornerButton::section { background: ${headerSectionBg}; border: none; border-bottom: 1px solid ${headerSectionBorder}; }"
        "QPushButton#confirmParamsBtn { background: ${confirmBtnBg}; color: ${confirmBtnText}; border: 1px solid ${confirmBtnBorder};"
        "  border-radius: 6px; padding: 8px 18px; font-size: 12px; font-weight: 600; font-family: ${monoFont}; }"
        "QPushButton#confirmParamsBtn:hover { background: ${confirmBtnHoverBg}; border-color: ${confirmBtnHoverBorder}; color: ${confirmBtnHoverText}; }"
        "QPushButton#confirmParamsBtn:pressed { background: ${confirmBtnPressedBg}; color: ${confirmBtnPressedText}; }"
        "QPushButton#confirmParamsBtn:disabled { background: ${disabledBg}; color: ${disabledText}; border-color: ${disabledBorder}; }"

        "QTextEdit#inputEdit { background: ${inputBg}; color: ${inputText}; border: 1px solid ${inputBorder};"
        "  border-left: 3px solid ${inputAccent}; border-radius: 10px; padding: 8px; font-size: 13px; font-family: ${monoFont}; }"
        "QTextEdit#inputEdit:focus { border-left: 3px solid ${inputFocusAccent}; }"
        "QPushButton#sendBtn { background: ${sendBtnBg}; color: ${sendBtnText}; border: 1px solid ${sendBtnBorder};"
        "  border-radius: 10px; font-size: 13px; font-weight: bold; font-family: ${monoFont}; letter-spacing: 1px; }"
        "QPushButton#sendBtn:hover { background: ${sendBtnHoverBg}; border-color: ${sendBtnHoverBorder}; color: ${sendBtnHoverText}; }"
        "QPushButton#sendBtn:pressed { background: ${sendBtnPressedBg}; color: ${sendBtnPressedText}; }"

        "QComboBox#themeCombo, QComboBox#modeCombo { background: ${comboBg}; color: ${comboText}; border: 1px solid ${comboBorder};"
        "  border-radius: 8px; padding: 2px 10px; font-family: ${monoFont}; }"
        "QComboBox#themeCombo::drop-down, QComboBox#modeCombo::drop-down { border: none; width: 18px; border-left: 1px solid ${comboBorder};"
        "  border-top-right-radius: 8px; border-bottom-right-radius: 8px; }"
        "QComboBox#themeCombo::down-arrow, QComboBox#modeCombo::down-arrow { image: none;"
        "  border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 5px solid ${comboArrowColor}; }"
        "QComboBox QAbstractItemView { background: ${comboViewBg}; color: ${comboViewText};"
        "  selection-background-color: ${comboViewSelBg}; selection-color: ${comboViewSelText};"
        "  border: 1px solid ${comboViewBorder}; border-radius: 6px; padding: 4px; }"
    );

    struct Token { const char *key; const QString &val; };
    const Token tokens[] = {
        {"windowBg", t.windowBg}, {"headerBg", t.headerBg}, {"headerBorder", t.headerBorder},
        {"titleColor", t.titleColor}, {"monoFont", t.monoFont}, {"statusColor", t.statusColor},
        {"scrollBorder", t.scrollBorder}, {"scrollHandle", t.scrollHandle}, {"scrollHandleHover", t.scrollHandleHover},
        {"userBubbleBg", t.userBubbleBg}, {"userBubbleBorder", t.userBubbleBorder}, {"userBubbleAccent", t.userBubbleAccent},
        {"assistantBubbleBg", t.assistantBubbleBg}, {"assistantBubbleBorder", t.assistantBubbleBorder}, {"assistantBubbleAccent", t.assistantBubbleAccent},
        {"roleColor", t.roleColor}, {"msgTextColor", t.msgTextColor},
        {"codeHeaderBg", t.codeHeaderBg}, {"codeHeaderBorder", t.codeHeaderBorder}, {"codeLangColor", t.codeLangColor},
        {"copyBtnColor", t.copyBtnColor}, {"copyBtnHoverBg", t.copyBtnHoverBg}, {"copyBtnHoverColor", t.copyBtnHoverColor},
        {"codeEditBg", t.codeEditBg}, {"codeEditTextColor", t.codeEditTextColor}, {"codeEditBorder", t.codeEditBorder},
        {"optionBtnBg", t.optionBtnBg}, {"optionBtnText", t.optionBtnText}, {"optionBtnBorder", t.optionBtnBorder}, {"optionBtnAccent", t.optionBtnAccent},
        {"optionBtnHoverBg", t.optionBtnHoverBg}, {"optionBtnHoverAccent", t.optionBtnHoverAccent}, {"optionBtnHoverText", t.optionBtnHoverText},
        {"optionBtnPressedBg", t.optionBtnPressedBg}, {"optionBtnPressedAccent", t.optionBtnPressedAccent},
        {"toolPanelBg", t.toolPanelBg}, {"toolPanelBorder", t.toolPanelBorder}, {"toolPanelAccent", t.toolPanelAccent},
        {"toolTitleColor", t.toolTitleColor}, {"toolDescColor", t.toolDescColor},
        {"approveBtnBg", t.approveBtnBg}, {"approveBtnText", t.approveBtnText}, {"approveBtnBorder", t.approveBtnBorder},
        {"approveBtnHoverBg", t.approveBtnHoverBg}, {"approveBtnHoverBorder", t.approveBtnHoverBorder},
        {"rejectBtnBg", t.rejectBtnBg}, {"rejectBtnText", t.rejectBtnText}, {"rejectBtnBorder", t.rejectBtnBorder},
        {"rejectBtnHoverBg", t.rejectBtnHoverBg}, {"rejectBtnHoverBorder", t.rejectBtnHoverBorder},
        {"disabledBg", t.disabledBg}, {"disabledText", t.disabledText}, {"disabledBorder", t.disabledBorder},
        {"tableBg", t.tableBg}, {"tableAltBg", t.tableAltBg}, {"tableText", t.tableText}, {"tableBorder", t.tableBorder}, {"tableGrid", t.tableGrid},
        {"tableSelBg", t.tableSelBg}, {"tableSelText", t.tableSelText}, {"tableEditBg", t.tableEditBg},
        {"headerSectionBg", t.headerSectionBg}, {"headerSectionColor", t.headerSectionColor}, {"headerSectionBorder", t.headerSectionBorder},
        {"confirmBtnBg", t.confirmBtnBg}, {"confirmBtnText", t.confirmBtnText}, {"confirmBtnBorder", t.confirmBtnBorder},
        {"confirmBtnHoverBg", t.confirmBtnHoverBg}, {"confirmBtnHoverBorder", t.confirmBtnHoverBorder}, {"confirmBtnHoverText", t.confirmBtnHoverText},
        {"confirmBtnPressedBg", t.confirmBtnPressedBg}, {"confirmBtnPressedText", t.confirmBtnPressedText},
        {"inputBg", t.inputBg}, {"inputText", t.inputText}, {"inputBorder", t.inputBorder}, {"inputAccent", t.inputAccent}, {"inputFocusAccent", t.inputFocusAccent},
        {"sendBtnBg", t.sendBtnBg}, {"sendBtnText", t.sendBtnText}, {"sendBtnBorder", t.sendBtnBorder},
        {"sendBtnHoverBg", t.sendBtnHoverBg}, {"sendBtnHoverBorder", t.sendBtnHoverBorder}, {"sendBtnHoverText", t.sendBtnHoverText},
        {"sendBtnPressedBg", t.sendBtnPressedBg}, {"sendBtnPressedText", t.sendBtnPressedText},
        {"comboBg", t.comboBg}, {"comboText", t.comboText}, {"comboBorder", t.comboBorder}, {"comboArrowColor", t.comboArrowColor},
        {"comboViewBg", t.comboViewBg}, {"comboViewText", t.comboViewText}, {"comboViewSelBg", t.comboViewSelBg}, {"comboViewSelText", t.comboViewSelText}, {"comboViewBorder", t.comboViewBorder},
    };
    for (const Token &tk : tokens)
        qss.replace(QStringLiteral("${") + QString::fromLatin1(tk.key) + QStringLiteral("}"), tk.val);

    setStyleSheet(qss);
}
