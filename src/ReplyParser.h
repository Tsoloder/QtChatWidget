#pragma once

#include <QString>
#include "ContentSegment.h"

// 把大模型的原始回复文本解析成 ContentSegments。
//
// 约定的回复格式（通过提示词约束模型）：
//   1. 前面是自然语言 / Markdown，可以包含普通代码块（```cpp ... ``` 等）
//   2. 回复的【最后】是一个 ```json 代码块，里面是结构化数据：
//
//        ```json
//        {
//          "tool_params": {              // 可选：需要用户确认/编辑的工具参数表
//            "tool": "edit_file",
//            "params": [
//              {"name": "path", "description": "目标文件路径", "value": "src/main.cpp"}
//            ]
//          },
//          "options": [                  // 可选：让用户选择的按钮，位于表格之后
//            {"label": "应用此修改", "value": "apply", "style": "primary"},
//            {"label": "取消", "value": "cancel", "style": "danger"}
//          ]
//        }
//        ```
//
// 解析后返回的段顺序：
//   Text / Code 段在前  →  ToolParams 段（表格）  →  Options 段（按钮）最后
//
// 这样在气泡里从上到下就是：文字说明 → 代码 → 参数表 → 操作按钮。
ContentSegments parseAssistantReply(const QString &rawReply);
