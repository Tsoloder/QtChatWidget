#pragma once

#include <QString>

// All color/style tokens for one visual theme. The entire QSS is generated
// from these tokens, so adding a new theme = filling in one struct.
struct Theme
{
    QString name;

    // window / header
    QString windowBg;
    QString headerBg;
    QString headerBorder;
    QString titleColor;
    QString statusColor;

    // scroll bar
    QString scrollBg;
    QString scrollHandle;
    QString scrollHandleHover;
    QString scrollBorder;

    // chat bubbles
    QString userBubbleBg;
    QString userBubbleBorder;
    QString userBubbleAccent;
    QString assistantBubbleBg;
    QString assistantBubbleBorder;
    QString assistantBubbleAccent;
    QString roleColor;
    QString msgTextColor;

    // code block
    QString codeHeaderBg;
    QString codeHeaderBorder;
    QString codeLangColor;
    QString copyBtnColor;
    QString copyBtnHoverBg;
    QString copyBtnHoverColor;
    QString codeEditBg;
    QString codeEditTextColor;
    QString codeEditBorder;

    // option buttons
    QString optionBtnBg;
    QString optionBtnText;
    QString optionBtnBorder;
    QString optionBtnAccent;
    QString optionBtnHoverBg;
    QString optionBtnHoverAccent;
    QString optionBtnHoverText;
    QString optionBtnPressedBg;
    QString optionBtnPressedAccent;

    // tool approval panel
    QString toolPanelBg;
    QString toolPanelBorder;
    QString toolPanelAccent;
    QString toolTitleColor;
    QString toolDescColor;
    QString approveBtnBg;
    QString approveBtnText;
    QString approveBtnBorder;
    QString approveBtnHoverBg;
    QString approveBtnHoverBorder;
    QString rejectBtnBg;
    QString rejectBtnText;
    QString rejectBtnBorder;
    QString rejectBtnHoverBg;
    QString rejectBtnHoverBorder;
    QString disabledBg;
    QString disabledText;
    QString disabledBorder;

    // parameter table
    QString tableBg;
    QString tableAltBg;   // alternating row background (slightly lighter/darker than tableBg)
    QString tableText;
    QString tableBorder;
    QString tableGrid;
    QString tableSelBg;
    QString tableSelText;
    QString tableEditBg;  // background for the editable Value column
    QString headerSectionBg;
    QString headerSectionColor;
    QString headerSectionBorder;
    QString confirmBtnBg;
    QString confirmBtnText;
    QString confirmBtnBorder;
    QString confirmBtnHoverBg;
    QString confirmBtnHoverBorder;
    QString confirmBtnHoverText;
    QString confirmBtnPressedBg;
    QString confirmBtnPressedText;

    // input bar
    QString inputBg;
    QString inputText;
    QString inputBorder;
    QString inputAccent;
    QString inputFocusAccent;
    QString sendBtnBg;
    QString sendBtnText;
    QString sendBtnBorder;
    QString sendBtnHoverBg;
    QString sendBtnHoverBorder;
    QString sendBtnHoverText;
    QString sendBtnPressedBg;
    QString sendBtnPressedText;

    // mode combo
    QString comboBg;
    QString comboText;
    QString comboBorder;
    QString comboArrowColor;
    QString comboViewBg;
    QString comboViewText;
    QString comboViewSelBg;
    QString comboViewSelText;
    QString comboViewBorder;

    // monospace font family for technical labels
    QString monoFont;
};

enum class ThemeId
{
    MilitaryTech,    // 军工科技质感
    FutureTechBlue,  // 未来科技蓝
    WhiteMinimal,    // 白色简约
    OneDarkPro,      // VS Code One Dark Pro
    WeChatLight      // 微信风格白色主题
};

// Look up a fully-populated Theme by id.
Theme themeById(ThemeId id);

// 当前主题的全局访问器：供无法直接拿到主题的子部件（如 ToolParamsWidget、
// ChatBubble 内的图标）查询颜色用于 SVG 图标着色。由 ChatWidget::setTheme 设置。
Theme currentTheme();
void setCurrentTheme(ThemeId id);
