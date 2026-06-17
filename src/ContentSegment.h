#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// 一条消息里的一个内容段。一条消息（气泡）= 多个段从上到下排列。
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

    // Options：结构化选项，每个选项有显示文本、回传值、样式
    struct Option {
        QString label;   // 按钮上显示的文字
        QString value;   // 用户点击后回传给业务的值（可与 label 不同）
        QString style;   // primary | default | danger | success（控制按钮配色）
    };
    QVector<Option> options;

    // ToolApproval
    QString toolName;
    QString toolDescription;   // 例如文件路径 + diff/输入摘要

    // ToolParams：一行 = {名称, 描述, 值}。只有值可编辑。
    struct Param {
        QString name;
        QString description;
        QString value;
    };
    QVector<Param> params;
};

using ContentSegments = QVector<ContentSegment>;
