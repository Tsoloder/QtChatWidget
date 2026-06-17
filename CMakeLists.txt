cmake_minimum_required(VERSION 3.10)
project(ClineLikeChat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 5.12.2: point Qt5_DIR to your install, e.g.
#   D:/Qt/5.12.2/msvc2017_64/lib/cmake/Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core Svg)

# 强制源码按 UTF-8 编码编译（MSVC 默认按系统 ANSI 解析，会导致中文乱码）
if(MSVC)
    add_compile_options(/utf-8)
    # 同时让生成的可执行文件以 UTF-8 作为运行时字符集，避免 QString 中文乱码
    add_compile_options(/execution-charset:utf-8)
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
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg)

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
    resources.qrc
)
target_include_directories(ClineScreenshot PRIVATE src)
target_link_libraries(ClineScreenshot PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg)

# DockWidget 集成演示（offscreen render -> PNG）。不是正式产物。
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
    resources.qrc
)
target_include_directories(DockDemo PRIVATE src)
target_link_libraries(DockDemo PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core Qt5::Svg)
