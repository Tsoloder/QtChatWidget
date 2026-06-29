cmake_minimum_required(VERSION 3.10)
project(ClineLikeChat)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 5.12.2: point Qt5_DIR to your install, e.g.
#   D:/Qt/5.12.2/msvc2017_64/lib/cmake/Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core Svg Network)

# 强制源码按 UTF-8 编码编译（MSVC 默认按系统 ANSI 解析，会导致中文乱码）
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(${PROJECT_NAME} WIN32
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
    src/Skill.cpp
    src/SkillManager.cpp
    src/SkillMdParser.cpp
    src/SkillPicker.cpp
    src/SkillParamsDialog.cpp
    src/ToolRegistry.cpp
    src/LLMClient.cpp
    src/SettingsDialog.cpp
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg Qt5::Network)

# Temporary screenshot harness (offscreen render -> PNG). Not part of the app.
add_executable(ClineScreenshot
    screenshot_main.cpp
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
    src/Skill.cpp
    src/SkillManager.cpp
    src/SkillMdParser.cpp
    src/SkillPicker.cpp
    src/SkillParamsDialog.cpp
    src/ToolRegistry.cpp
    src/LLMClient.cpp
    src/SettingsDialog.cpp
    resources.qrc
)
target_include_directories(ClineScreenshot PRIVATE src)
target_link_libraries(ClineScreenshot PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg Qt5::Network)

add_executable(DockDemo
    dock_demo.cpp
    src/ChatWidget.cpp
    src/ChatBubble.cpp
    src/CodeEditor.cpp
    src/SyntaxHighlighter.cpp
    src/OptionsWidget.cpp
    src/InputBar.cpp
    src/ToolParamsWidget.cpp
    src/Theme.cpp
    src/ReplyParser.cpp
    src/Skill.cpp
    src/SkillManager.cpp
    src/SkillMdParser.cpp
    src/SkillPicker.cpp
    src/SkillParamsDialog.cpp
    src/ToolRegistry.cpp
    resources.qrc
)
target_include_directories(DockDemo PRIVATE src)
target_link_libraries(DockDemo PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg Qt5::Network)

add_executable(SkillScreenshot
    skill_screenshot_main.cpp
    src/ChatWidget.cpp
    src/ChatBubble.cpp
    src/CodeEditor.cpp
    src/SyntaxHighlighter.cpp
    src/OptionsWidget.cpp
    src/InputBar.cpp
    src/ToolParamsWidget.cpp
    src/Theme.cpp
    src/ReplyParser.cpp
    src/Skill.cpp
    src/SkillManager.cpp
    src/SkillMdParser.cpp
    src/SkillPicker.cpp
    src/SkillParamsDialog.cpp
    src/ToolRegistry.cpp
    resources.qrc
)
target_include_directories(SkillScreenshot PRIVATE src)
target_link_libraries(SkillScreenshot PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg Qt5::Network)

# Skill system test (console app, no GUI)
add_executable(SkillTest
    skill_test_main.cpp
    src/Skill.cpp
    src/SkillManager.cpp
    src/SkillMdParser.cpp
    src/ToolRegistry.cpp
    resources.qrc
)
target_include_directories(SkillTest PRIVATE src)
target_link_libraries(SkillTest PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Network)

# Skill E2E test (console app, calls real API)
add_executable(SkillE2E
    skill_e2e_test.cpp
    src/Skill.cpp
    src/SkillManager.cpp
    src/SkillMdParser.cpp
    src/ToolRegistry.cpp
    resources.qrc
)
target_include_directories(SkillE2E PRIVATE src)
target_link_libraries(SkillE2E PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Network)
