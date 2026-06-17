#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
调用 Qwen3.6-35B-A3B (讯飞 MaaS) 获取模型原始回复，
保存到 model_reply.txt，供 Qt 部件 (screenshot_main) 渲染测试。

用法:
    python3 test_model.py            # 默认提问
    python3 test_model.py "你的问题"  # 自定义提问
"""

import sys
import json
import subprocess
import os
import time
import requests

# ---- 接口配置 ----
API_BASE = "https://maas-api.cn-huabei-1.xf-yun.com/v2"
MODEL_ID = "xopqwen36v35b"
API_KEY  = "337efebd7d1d1efbc08e032b59b3df5d:MjE0ZjFkNWIzZmZiODY2YWJiN2NjOTRh"
CHAT_URL = API_BASE + "/chat/completions"

REPLY_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "model_reply.txt")

# ---- 约束模型输出格式（与 ReplyParser 约定一致）----
SYSTEM_PROMPT = """你是一个编程助手。回答用户问题时，必须严格遵守下面的输出格式：

1. 先用自然语言/Markdown 说明你的方案，可以包含普通代码块（如 ```cpp ... ```）。
2. 回复的【最后】必须是一个 ```json 代码块，里面是结构化数据，包含两个字段：
   - "tool_params": 需要用户确认的工具参数表，结构为
       {"tool": "<工具名>", "params": [{"name":"...","description":"...","value":"..."}, ...]}
   - "options": 让用户选择的按钮，结构为
       [{"label":"...","value":"...","style":"primary|default|danger"}, ...]

重要约束：
- JSON 块里的 value 字段不要塞入完整代码或大段文本，只放简短值（如文件路径、布尔值、数字）。
  完整代码请放在前面的 ```代码块里展示，JSON 里只用 "<见上方代码块>" 之类的简短描述。
- JSON 块必须放在回复的最末尾，且只能出现一次。
- JSON 必须完整、可解析，不要被截断。

示例格式：
我先帮你分析了一下，建议这样修改：

```cpp
int main() { return 0; }
```

请确认下面的参数并选择操作：

```json
{
  "tool_params": {
    "tool": "edit_file",
    "params": [
      {"name": "path", "description": "目标文件路径", "value": "src/main.cpp"},
      {"name": "backup", "description": "是否备份", "value": "true"}
    ]
  },
  "options": [
    {"label": "应用此修改", "value": "apply", "style": "primary"},
    {"label": "取消", "value": "cancel", "style": "danger"}
  ]
}
```"""


def call_model(user_question: str) -> str:
    """调用大模型，返回原始回复文本。"""
    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json",
    }
    payload = {
        "model": MODEL_ID,
        "messages": [
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user",   "content": user_question},
        ],
        "temperature": 0.3,
        "max_tokens": 8192,
    }

    print(f"[INFO] 请求接口: {CHAT_URL}")
    print(f"[INFO] 模型ID : {MODEL_ID}")
    print(f"[INFO] 问题   : {user_question}")
    print("-" * 60)

    last_err = None
    for attempt in range(1, 5):
        try:
            resp = requests.post(CHAT_URL, headers=headers, json=payload, timeout=120)
        except requests.RequestException as e:
            last_err = e
            print(f"[WARN] 网络异常 (第{attempt}次): {e}，3s 后重试...")
            time.sleep(3)
            continue

        if resp.status_code == 200:
            break

        last_err = f"HTTP {resp.status_code}: {resp.text}"
        print(f"[WARN] 接口返回 {resp.status_code} (第{attempt}次)，3s 后重试...")
        print(resp.text[:300])
        time.sleep(3)
    else:
        print(f"[ERROR] 重试 4 次仍失败: {last_err}")
        sys.exit(1)

    data = resp.json()

    # 兼容 OpenAI 风格 / 讯飞风格
    try:
        content = data["choices"][0]["message"]["content"]
    except (KeyError, IndexError):
        print("[ERROR] 无法解析返回结构，原始内容如下：")
        print(json.dumps(data, ensure_ascii=False, indent=2))
        sys.exit(1)

    # 截断检测：finish_reason 为 length 表示被 max_tokens 截断
    try:
        finish = data["choices"][0].get("finish_reason", "")
        if finish == "length":
            print(f"[WARN] 回复被 max_tokens 截断 (finish_reason=length)，长度={len(content)}")
            print("[WARN] 末尾可能没有完整的 ```json 闭合块，渲染会失败。")
    except Exception:
        pass

    return content


def main():
    args = [a for a in sys.argv[1:] if a != "--no-render"]
    question = args[0] if args else \
        "我想用 C++ 写一个计算斐波那契数列前 20 项的程序，请给出代码并让我确认是否写入 src/fib.cpp。"

    reply = call_model(question)

    print("[模型原始回复] >>>")
    print(reply)
    print("<<< [模型原始回复结束]")

    with open(REPLY_FILE, "w", encoding="utf-8") as f:
        f.write(reply)
    print(f"\n[OK] 已保存原始回复到: {REPLY_FILE}")

    # 询问是否构建并运行 Qt 渲染
    if "--no-render" in sys.argv:
        return

    print("\n[INFO] 开始构建并运行 Qt 渲染程序 (ClineScreenshot) ...")
    build_and_render()


def build_and_render():
    workspace = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(workspace, "build")

    os.makedirs(build_dir, exist_ok=True)

    # 1. cmake 配置
    subprocess.run(
        ["cmake", "-S", workspace, "-B", build_dir,
         "-DCMAKE_BUILD_TYPE=Release"],
        check=True,
    )
    # 2. 编译 ClineScreenshot 目标
    subprocess.run(
        ["cmake", "--build", build_dir, "--target", "ClineScreenshot",
         "-j", "--config", "Release"],
        check=True,
    )
    # 3. 运行（offscreen 渲染，无需显示器）
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    exe = os.path.join(build_dir, "ClineScreenshot")
    subprocess.run([exe], cwd=workspace, env=env, check=True)
    print("[OK] 渲染完成，截图: /workspace/screenshot.png")


if __name__ == "__main__":
    main()
