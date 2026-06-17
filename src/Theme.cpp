#include "Theme.h"

namespace {

// 字体回退链：Windows 优先 Consolas；Linux/macOS 直接用含中文的等宽字体。
// 注意：Qt QSS 的 font-family fallback 不可靠（不会按字符回退），
// 所以在非 Windows 平台直接首选含 CJK 字形的字体，避免中文显示成方框。
#ifdef Q_OS_WIN
const QString MONO = QStringLiteral("Consolas, \"Courier New\", \"Microsoft YaHei\", monospace");
#else
const QString MONO = QStringLiteral("\"Noto Sans Mono CJK SC\", \"WenQuanYi Zen Hei\", monospace");
#endif

Theme militaryTech()
{
    Theme t;
    t.name = QStringLiteral("Military Tech");

    t.windowBg = "#0a0e0a";
    t.headerBg = "#0d130d";
    t.headerBorder = "#2a3a2a";
    t.titleColor = "#8fbf5a";
    t.statusColor = "#6a7a64";

    t.scrollBg = "#0a0e0a";
    t.scrollHandle = "#2a3a2a";
    t.scrollHandleHover = "#4a6a3a";
    t.scrollBorder = "#1a221a";

    t.userBubbleBg = "#121a12";
    t.userBubbleBorder = "#2a3a2a";
    t.userBubbleAccent = "#8fbf5a";
    t.assistantBubbleBg = "#0d130d";
    t.assistantBubbleBorder = "#1f2a1f";
    t.assistantBubbleAccent = "#d4a017";
    t.roleColor = "#8fbf5a";
    t.msgTextColor = "#c8d4c0";

    t.codeHeaderBg = "#0d130d";
    t.codeHeaderBorder = "#2a3a2a";
    t.codeLangColor = "#8fbf5a";
    t.copyBtnColor = "#8fbf5a";
    t.copyBtnHoverBg = "#8fbf5a";
    t.copyBtnHoverColor = "#0a0e0a";
    t.codeEditBg = "#060906";
    t.codeEditTextColor = "#c8d4c0";
    t.codeEditBorder = "#2a3a2a";

    t.optionBtnBg = "#0d130d";
    t.optionBtnText = "#c8d4c0";
    t.optionBtnBorder = "#2a3a2a";
    t.optionBtnAccent = "#4a6a3a";
    t.optionBtnHoverBg = "#1a2a1a";
    t.optionBtnHoverAccent = "#8fbf5a";
    t.optionBtnHoverText = "#8fbf5a";
    t.optionBtnPressedBg = "#2a3a2a";
    t.optionBtnPressedAccent = "#d4a017";

    t.toolPanelBg = "#0d130d";
    t.toolPanelBorder = "#3a3a1a";
    t.toolPanelAccent = "#d4a017";
    t.toolTitleColor = "#d4a017";
    t.toolDescColor = "#c8d4c0";
    t.approveBtnBg = "#1a2a14";
    t.approveBtnText = "#8fbf5a";
    t.approveBtnBorder = "#3a5a2a";
    t.approveBtnHoverBg = "#2a4a1a";
    t.approveBtnHoverBorder = "#8fbf5a";
    t.rejectBtnBg = "#2a1414";
    t.rejectBtnText = "#c87070";
    t.rejectBtnBorder = "#5a2a2a";
    t.rejectBtnHoverBg = "#4a1a1a";
    t.rejectBtnHoverBorder = "#a83030";
    t.disabledBg = "#0d130d";
    t.disabledText = "#3a4a3a";
    t.disabledBorder = "#1a221a";

    t.tableBg = "#060906";
    t.tableAltBg = "#0a100a";
    t.tableText = "#c8d4c0";
    t.tableBorder = "#2a3a2a";
    t.tableGrid = "#1a221a";
    t.tableSelBg = "#2a3a1a";
    t.tableSelText = "#8fbf5a";
    t.tableEditBg = "#0d130d";
    t.headerSectionBg = "#0d130d";
    t.headerSectionColor = "#8fbf5a";
    t.headerSectionBorder = "#2a3a2a";
    t.confirmBtnBg = "#1a2a14";
    t.confirmBtnText = "#8fbf5a";
    t.confirmBtnBorder = "#4a6a3a";
    t.confirmBtnHoverBg = "#2a4a1a";
    t.confirmBtnHoverBorder = "#8fbf5a";
    t.confirmBtnHoverText = "#c8d4c0";
    t.confirmBtnPressedBg = "#4a6a3a";
    t.confirmBtnPressedText = "#0a0e0a";

    t.inputBg = "#060906";
    t.inputText = "#c8d4c0";
    t.inputBorder = "#2a3a2a";
    t.inputAccent = "#4a6a3a";
    t.inputFocusAccent = "#8fbf5a";
    t.sendBtnBg = "#1a2a14";
    t.sendBtnText = "#8fbf5a";
    t.sendBtnBorder = "#4a6a3a";
    t.sendBtnHoverBg = "#2a4a1a";
    t.sendBtnHoverBorder = "#8fbf5a";
    t.sendBtnHoverText = "#c8d4c0";
    t.sendBtnPressedBg = "#4a6a3a";
    t.sendBtnPressedText = "#0a0e0a";

    t.comboBg = "#0d130d";
    t.comboText = "#8fbf5a";
    t.comboBorder = "#2a3a2a";
    t.comboArrowColor = "#8fbf5a";
    t.comboViewBg = "#0d130d";
    t.comboViewText = "#c8d4c0";
    t.comboViewSelBg = "#2a3a1a";
    t.comboViewSelText = "#8fbf5a";
    t.comboViewBorder = "#2a3a2a";

    t.monoFont = MONO;
    return t;
}

Theme futureTechBlue()
{
    Theme t;
    t.name = QStringLiteral("Future Tech Blue");

    t.windowBg = "#050a18";
    t.headerBg = "#0a1428";
    t.headerBorder = "#1a3a6a";
    t.titleColor = "#00d4ff";
    t.statusColor = "#4a7aaa";

    t.scrollBg = "#050a18";
    t.scrollHandle = "#1a3a6a";
    t.scrollHandleHover = "#0078ff";
    t.scrollBorder = "#0a1f3a";

    t.userBubbleBg = "#0a1830";
    t.userBubbleBorder = "#1a3a6a";
    t.userBubbleAccent = "#00d4ff";
    t.assistantBubbleBg = "#080f1f";
    t.assistantBubbleBorder = "#142544";
    t.assistantBubbleAccent = "#0078ff";
    t.roleColor = "#00d4ff";
    t.msgTextColor = "#c8e0ff";

    t.codeHeaderBg = "#0a1428";
    t.codeHeaderBorder = "#1a3a6a";
    t.codeLangColor = "#00d4ff";
    t.copyBtnColor = "#00d4ff";
    t.copyBtnHoverBg = "#00d4ff";
    t.copyBtnHoverColor = "#050a18";
    t.codeEditBg = "#03060f";
    t.codeEditTextColor = "#c8e0ff";
    t.codeEditBorder = "#1a3a6a";

    t.optionBtnBg = "#0a1428";
    t.optionBtnText = "#c8e0ff";
    t.optionBtnBorder = "#1a3a6a";
    t.optionBtnAccent = "#0078ff";
    t.optionBtnHoverBg = "#0a2040";
    t.optionBtnHoverAccent = "#00d4ff";
    t.optionBtnHoverText = "#00d4ff";
    t.optionBtnPressedBg = "#1a3a6a";
    t.optionBtnPressedAccent = "#00d4ff";

    t.toolPanelBg = "#0a1428";
    t.toolPanelBorder = "#2a4a8a";
    t.toolPanelAccent = "#00d4ff";
    t.toolTitleColor = "#00d4ff";
    t.toolDescColor = "#c8e0ff";
    t.approveBtnBg = "#0a2040";
    t.approveBtnText = "#00d4ff";
    t.approveBtnBorder = "#0078ff";
    t.approveBtnHoverBg = "#0a3060";
    t.approveBtnHoverBorder = "#00d4ff";
    t.rejectBtnBg = "#2a1430";
    t.rejectBtnText = "#c870d0";
    t.rejectBtnBorder = "#5a2a6a";
    t.rejectBtnHoverBg = "#4a1a50";
    t.rejectBtnHoverBorder = "#a830a8";
    t.disabledBg = "#0a1428";
    t.disabledText = "#2a4a6a";
    t.disabledBorder = "#0a1f3a";

    t.tableBg = "#03060f";
    t.tableAltBg = "#070d1c";
    t.tableText = "#c8e0ff";
    t.tableBorder = "#1a3a6a";
    t.tableGrid = "#0a1f3a";
    t.tableSelBg = "#0a3060";
    t.tableSelText = "#00d4ff";
    t.tableEditBg = "#0a1428";
    t.headerSectionBg = "#0a1428";
    t.headerSectionColor = "#00d4ff";
    t.headerSectionBorder = "#1a3a6a";
    t.confirmBtnBg = "#0a2040";
    t.confirmBtnText = "#00d4ff";
    t.confirmBtnBorder = "#0078ff";
    t.confirmBtnHoverBg = "#0a3060";
    t.confirmBtnHoverBorder = "#00d4ff";
    t.confirmBtnHoverText = "#c8e0ff";
    t.confirmBtnPressedBg = "#0078ff";
    t.confirmBtnPressedText = "#050a18";

    t.inputBg = "#03060f";
    t.inputText = "#c8e0ff";
    t.inputBorder = "#1a3a6a";
    t.inputAccent = "#0078ff";
    t.inputFocusAccent = "#00d4ff";
    t.sendBtnBg = "#0a2040";
    t.sendBtnText = "#00d4ff";
    t.sendBtnBorder = "#0078ff";
    t.sendBtnHoverBg = "#0a3060";
    t.sendBtnHoverBorder = "#00d4ff";
    t.sendBtnHoverText = "#c8e0ff";
    t.sendBtnPressedBg = "#0078ff";
    t.sendBtnPressedText = "#050a18";

    t.comboBg = "#0a1428";
    t.comboText = "#00d4ff";
    t.comboBorder = "#1a3a6a";
    t.comboArrowColor = "#00d4ff";
    t.comboViewBg = "#0a1428";
    t.comboViewText = "#c8e0ff";
    t.comboViewSelBg = "#0a3060";
    t.comboViewSelText = "#00d4ff";
    t.comboViewBorder = "#1a3a6a";

    t.monoFont = MONO;
    return t;
}

Theme whiteMinimal()
{
    Theme t;
    t.name = QStringLiteral("White Minimal");

    t.windowBg = "#ffffff";
    t.headerBg = "#fafafa";
    t.headerBorder = "#e0e0e0";
    t.titleColor = "#222222";
    t.statusColor = "#999999";

    t.scrollBg = "#ffffff";
    t.scrollHandle = "#d0d0d0";
    t.scrollHandleHover = "#999999";
    t.scrollBorder = "#eeeeee";

    t.userBubbleBg = "#f0f6ff";
    t.userBubbleBorder = "#d6e4f5";
    t.userBubbleAccent = "#0066cc";
    t.assistantBubbleBg = "#fafafa";
    t.assistantBubbleBorder = "#ececec";
    t.assistantBubbleAccent = "#666666";
    t.roleColor = "#0066cc";
    t.msgTextColor = "#333333";

    t.codeHeaderBg = "#f5f5f5";
    t.codeHeaderBorder = "#e0e0e0";
    t.codeLangColor = "#0066cc";
    t.copyBtnColor = "#0066cc";
    t.copyBtnHoverBg = "#0066cc";
    t.copyBtnHoverColor = "#ffffff";
    t.codeEditBg = "#fbfbfb";
    t.codeEditTextColor = "#333333";
    t.codeEditBorder = "#e0e0e0";

    t.optionBtnBg = "#ffffff";
    t.optionBtnText = "#333333";
    t.optionBtnBorder = "#d0d0d0";
    t.optionBtnAccent = "#cccccc";
    t.optionBtnHoverBg = "#f0f6ff";
    t.optionBtnHoverAccent = "#0066cc";
    t.optionBtnHoverText = "#0066cc";
    t.optionBtnPressedBg = "#e0e0e0";
    t.optionBtnPressedAccent = "#0066cc";

    t.toolPanelBg = "#fafafa";
    t.toolPanelBorder = "#d0d0d0";
    t.toolPanelAccent = "#cc8800";
    t.toolTitleColor = "#cc8800";
    t.toolDescColor = "#555555";
    t.approveBtnBg = "#f0f9f0";
    t.approveBtnText = "#2a8a2a";
    t.approveBtnBorder = "#9ad09a";
    t.approveBtnHoverBg = "#dff0df";
    t.approveBtnHoverBorder = "#2a8a2a";
    t.rejectBtnBg = "#fdf0f0";
    t.rejectBtnText = "#cc4444";
    t.rejectBtnBorder = "#e8a8a8";
    t.rejectBtnHoverBg = "#f0d8d8";
    t.rejectBtnHoverBorder = "#cc4444";
    t.disabledBg = "#fafafa";
    t.disabledText = "#bbbbbb";
    t.disabledBorder = "#eeeeee";

    t.tableBg = "#fbfbfb";
    t.tableAltBg = "#f5f5f5";
    t.tableText = "#333333";
    t.tableBorder = "#e0e0e0";
    t.tableGrid = "#eeeeee";
    t.tableSelBg = "#f0f6ff";
    t.tableSelText = "#0066cc";
    t.tableEditBg = "#ffffff";
    t.headerSectionBg = "#f5f5f5";
    t.headerSectionColor = "#0066cc";
    t.headerSectionBorder = "#e0e0e0";
    t.confirmBtnBg = "#0066cc";
    t.confirmBtnText = "#ffffff";
    t.confirmBtnBorder = "#0066cc";
    t.confirmBtnHoverBg = "#0055aa";
    t.confirmBtnHoverBorder = "#0055aa";
    t.confirmBtnHoverText = "#ffffff";
    t.confirmBtnPressedBg = "#004488";
    t.confirmBtnPressedText = "#ffffff";

    t.inputBg = "#ffffff";
    t.inputText = "#333333";
    t.inputBorder = "#d0d0d0";
    t.inputAccent = "#cccccc";
    t.inputFocusAccent = "#0066cc";
    t.sendBtnBg = "#0066cc";
    t.sendBtnText = "#ffffff";
    t.sendBtnBorder = "#0066cc";
    t.sendBtnHoverBg = "#0055aa";
    t.sendBtnHoverBorder = "#0055aa";
    t.sendBtnHoverText = "#ffffff";
    t.sendBtnPressedBg = "#004488";
    t.sendBtnPressedText = "#ffffff";

    t.comboBg = "#ffffff";
    t.comboText = "#0066cc";
    t.comboBorder = "#d0d0d0";
    t.comboArrowColor = "#0066cc";
    t.comboViewBg = "#ffffff";
    t.comboViewText = "#333333";
    t.comboViewSelBg = "#f0f6ff";
    t.comboViewSelText = "#0066cc";
    t.comboViewBorder = "#d0d0d0";

    t.monoFont = MONO;
    return t;
}

Theme oneDarkPro()
{
    Theme t;
    t.name = QStringLiteral("One Dark Pro");

    t.windowBg = "#282c34";
    t.headerBg = "#21252b";
    t.headerBorder = "#181a1f";
    t.titleColor = "#abb2bf";
    t.statusColor = "#5c6370";

    t.scrollBg = "#282c34";
    t.scrollHandle = "#4b5672";
    t.scrollHandleHover = "#5c6370";
    t.scrollBorder = "#21252b";

    t.userBubbleBg = "#2c313a";
    t.userBubbleBorder = "#3a3f4b";
    t.userBubbleAccent = "#61afef";
    t.assistantBubbleBg = "#2a2e36";
    t.assistantBubbleBorder = "#3a3f4b";
    t.assistantBubbleAccent = "#c678dd";
    t.roleColor = "#61afef";
    t.msgTextColor = "#abb2bf";

    t.codeHeaderBg = "#21252b";
    t.codeHeaderBorder = "#181a1f";
    t.codeLangColor = "#e5c07b";
    t.copyBtnColor = "#61afef";
    t.copyBtnHoverBg = "#61afef";
    t.copyBtnHoverColor = "#282c34";
    t.codeEditBg = "#282c34";
    t.codeEditTextColor = "#abb2bf";
    t.codeEditBorder = "#181a1f";

    t.optionBtnBg = "#2c313a";
    t.optionBtnText = "#abb2bf";
    t.optionBtnBorder = "#3a3f4b";
    t.optionBtnAccent = "#3e4451";
    t.optionBtnHoverBg = "#3a3f4b";
    t.optionBtnHoverAccent = "#61afef";
    t.optionBtnHoverText = "#61afef";
    t.optionBtnPressedBg = "#3e4451";
    t.optionBtnPressedAccent = "#c678dd";

    t.toolPanelBg = "#2c313a";
    t.toolPanelBorder = "#3a3f4b";
    t.toolPanelAccent = "#e5c07b";
    t.toolTitleColor = "#e5c07b";
    t.toolDescColor = "#abb2bf";
    t.approveBtnBg = "#2c3a2c";
    t.approveBtnText = "#98c379";
    t.approveBtnBorder = "#3a5a3a";
    t.approveBtnHoverBg = "#3a4f3a";
    t.approveBtnHoverBorder = "#98c379";
    t.rejectBtnBg = "#3a2c2c";
    t.rejectBtnText = "#e06c75";
    t.rejectBtnBorder = "#5a3a3a";
    t.rejectBtnHoverBg = "#4f3a3a";
    t.rejectBtnHoverBorder = "#e06c75";
    t.disabledBg = "#2c313a";
    t.disabledText = "#4b5672";
    t.disabledBorder = "#3a3f4b";

    t.tableBg = "#282c34";
    t.tableAltBg = "#2c313a";
    t.tableText = "#abb2bf";
    t.tableBorder = "#3a3f4b";
    t.tableGrid = "#2c313a";
    t.tableSelBg = "#3a3f4b";
    t.tableSelText = "#61afef";
    t.tableEditBg = "#1e2127";
    t.headerSectionBg = "#21252b";
    t.headerSectionColor = "#e5c07b";
    t.headerSectionBorder = "#3a3f4b";
    t.confirmBtnBg = "#61afef";
    t.confirmBtnText = "#282c34";
    t.confirmBtnBorder = "#61afef";
    t.confirmBtnHoverBg = "#7cc0f5";
    t.confirmBtnHoverBorder = "#7cc0f5";
    t.confirmBtnHoverText = "#282c34";
    t.confirmBtnPressedBg = "#4a9fde";
    t.confirmBtnPressedText = "#282c34";

    t.inputBg = "#21252b";
    t.inputText = "#abb2bf";
    t.inputBorder = "#3a3f4b";
    t.inputAccent = "#3e4451";
    t.inputFocusAccent = "#61afef";
    t.sendBtnBg = "#61afef";
    t.sendBtnText = "#282c34";
    t.sendBtnBorder = "#61afef";
    t.sendBtnHoverBg = "#7cc0f5";
    t.sendBtnHoverBorder = "#7cc0f5";
    t.sendBtnHoverText = "#282c34";
    t.sendBtnPressedBg = "#4a9fde";
    t.sendBtnPressedText = "#282c34";

    t.comboBg = "#21252b";
    t.comboText = "#61afef";
    t.comboBorder = "#3a3f4b";
    t.comboArrowColor = "#61afef";
    t.comboViewBg = "#21252b";
    t.comboViewText = "#abb2bf";
    t.comboViewSelBg = "#3a3f4b";
    t.comboViewSelText = "#61afef";
    t.comboViewBorder = "#3a3f4b";

    t.monoFont = MONO;
    return t;
}

Theme weChatLight()
{
    // WeChat-style light theme: pure white background, signature WeChat green
    // (#95EC69) user bubbles, white assistant bubbles, green primary buttons
    // (#07C160). Soft rounded feel via light borders; no harsh accent bars.
    Theme t;
    t.name = QStringLiteral("WeChat Light");

    t.windowBg = "#EDEDED";          // WeChat chat background grey
    t.headerBg = "#F7F7F7";
    t.headerBorder = "#DADADA";
    t.titleColor = "#191919";
    t.statusColor = "#888888";

    t.scrollBg = "#EDEDED";
    t.scrollHandle = "#C8C8C8";
    t.scrollHandleHover = "#07C160";
    t.scrollBorder = "#DADADA";

    t.userBubbleBg = "#95EC69";      // signature WeChat green bubble
    t.userBubbleBorder = "#7AD94F";
    t.userBubbleAccent = "#7AD94F";
    t.assistantBubbleBg = "#FFFFFF"; // white bubble
    t.assistantBubbleBorder = "#E5E5E5";
    t.assistantBubbleAccent = "#E5E5E5";
    t.roleColor = "#07C160";
    t.msgTextColor = "#191919";

    t.codeHeaderBg = "#F7F7F7";
    t.codeHeaderBorder = "#E5E5E5";
    t.codeLangColor = "#07C160";
    t.copyBtnColor = "#07C160";
    t.copyBtnHoverBg = "#07C160";
    t.copyBtnHoverColor = "#FFFFFF";
    t.codeEditBg = "#FAFAFA";
    t.codeEditTextColor = "#333333";
    t.codeEditBorder = "#E5E5E5";

    t.optionBtnBg = "#FFFFFF";
    t.optionBtnText = "#191919";
    t.optionBtnBorder = "#E5E5E5";
    t.optionBtnAccent = "#E5E5E5";
    t.optionBtnHoverBg = "#F0F0F0";
    t.optionBtnHoverAccent = "#07C160";
    t.optionBtnHoverText = "#07C160";
    t.optionBtnPressedBg = "#E5E5E5";
    t.optionBtnPressedAccent = "#07C160";

    t.toolPanelBg = "#FFFFFF";
    t.toolPanelBorder = "#E5E5E5";
    t.toolPanelAccent = "#FA9D3B";   // WeChat orange for tool warnings
    t.toolTitleColor = "#FA9D3B";
    t.toolDescColor = "#333333";
    t.approveBtnBg = "#07C160";      // WeChat green primary
    t.approveBtnText = "#FFFFFF";
    t.approveBtnBorder = "#07C160";
    t.approveBtnHoverBg = "#06AD56";
    t.approveBtnHoverBorder = "#06AD56";
    t.rejectBtnBg = "#FFFFFF";
    t.rejectBtnText = "#FA5151";     // WeChat red
    t.rejectBtnBorder = "#FA5151";
    t.rejectBtnHoverBg = "#FFF0F0";
    t.rejectBtnHoverBorder = "#FA5151";
    t.disabledBg = "#F7F7F7";
    t.disabledText = "#B2B2B2";
    t.disabledBorder = "#E5E5E5";

    t.tableBg = "#FFFFFF";
    t.tableAltBg = "#F7F7F7";
    t.tableText = "#191919";
    t.tableBorder = "#E5E5E5";
    t.tableGrid = "#F0F0F0";
    t.tableSelBg = "#E8F8EE";        // light green selection
    t.tableSelText = "#07C160";
    t.tableEditBg = "#FAFAFA";
    t.headerSectionBg = "#F7F7F7";
    t.headerSectionColor = "#07C160";
    t.headerSectionBorder = "#E5E5E5";
    t.confirmBtnBg = "#07C160";
    t.confirmBtnText = "#FFFFFF";
    t.confirmBtnBorder = "#07C160";
    t.confirmBtnHoverBg = "#06AD56";
    t.confirmBtnHoverBorder = "#06AD56";
    t.confirmBtnHoverText = "#FFFFFF";
    t.confirmBtnPressedBg = "#069248";
    t.confirmBtnPressedText = "#FFFFFF";

    t.inputBg = "#FFFFFF";
    t.inputText = "#191919";
    t.inputBorder = "#E5E5E5";
    t.inputAccent = "#E5E5E5";
    t.inputFocusAccent = "#07C160";
    t.sendBtnBg = "#07C160";
    t.sendBtnText = "#FFFFFF";
    t.sendBtnBorder = "#07C160";
    t.sendBtnHoverBg = "#06AD56";
    t.sendBtnHoverBorder = "#06AD56";
    t.sendBtnHoverText = "#FFFFFF";
    t.sendBtnPressedBg = "#069248";
    t.sendBtnPressedText = "#FFFFFF";

    t.comboBg = "#FFFFFF";
    t.comboText = "#07C160";
    t.comboBorder = "#E5E5E5";
    t.comboArrowColor = "#07C160";
    t.comboViewBg = "#FFFFFF";
    t.comboViewText = "#191919";
    t.comboViewSelBg = "#E8F8EE";
    t.comboViewSelText = "#07C160";
    t.comboViewBorder = "#E5E5E5";

    t.monoFont = MONO;
    return t;
}

} // namespace

Theme themeById(ThemeId id)
{
    switch (id) {
    case ThemeId::MilitaryTech:    return militaryTech();
    case ThemeId::FutureTechBlue:  return futureTechBlue();
    case ThemeId::WhiteMinimal:    return whiteMinimal();
    case ThemeId::OneDarkPro:      return oneDarkPro();
    case ThemeId::WeChatLight:     return weChatLight();
    }
    return militaryTech();
}

// ---- 当前主题全局访问器 ----
namespace {
ThemeId g_currentThemeId = ThemeId::OneDarkPro;
}

Theme currentTheme()
{
    return themeById(g_currentThemeId);
}

void setCurrentTheme(ThemeId id)
{
    g_currentThemeId = id;
}
