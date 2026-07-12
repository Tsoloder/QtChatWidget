import asyncio
import json

from fastmcp import Client
from key_value.shared.code_gen.run import await_awaitable
from openai import OpenAI


class UserClient:
    def __init__(self, script="server.py", model="qwen3:8b"):
        self.model = model
        self.mcp_client = Client(script)
        # self.mcp_client = Client("https://mcpmarket.cn/mcp/a416d4820f562e5baddf5ce1")
        self.openai_client = OpenAI(
            base_url="http://localhost:11434/v1",
            api_key="ollama"
        )
        self.messages = [
            {
                "role": "system",
                "content": "你是一个AI助手，你需要通过工具，回答用户问题"
            }
        ]
        self.tools = []

    async def prepare_tools(self):
        tools = await self.mcp_client.list_tools()
        tools = [
            {
                "type": "function",
                "function": {
                    "name": tool.name,
                    "description": tool.description,
                    "input_schema": tool.inputSchema
                }
            }
            for tool in tools
        ]
        print(tools)
        return tools
    
    # {
    #     "type": "object",
    #     "properties": {
    #         "location": {
    #         "type": "string",
    #         "description": "要获取天气信息的城市，例如：旧金山、东京、北京"
    #         },
    #         "unit": {
    #         "type": "string",
    #         "enum": ["摄氏度", "华氏度"],
    #         "description": "温度单位",
    #         "default": "摄氏度"
    #         }
    #     },
    #     "required": ["location"]
    # }

    async def chat(self, messages: list[dict]):
        if not self.tools:
            self.tools = await self.prepare_tools()

        response = self.openai_client.chat.completions.create(
            model=self.model,
            messages=messages,
            tools=self.tools,
        )

        if response.choices:
            message = response.choices[0].message

            # 如果没有工具调用，直接返回结果
            if not message.tool_calls:
                return message

            # 处理工具调用
            for tool_call in message.tool_calls:
                # 执行工具调用
                tool_response = await self.mcp_client.call_tool(
                    tool_call.function.name,
                    json.loads(tool_call.function.arguments)
                )

                # 添加工具调用结果到消息历史
                self.messages.append({
                    "role": "tool",
                    "tool_call_id": tool_call.id,
                    "content": tool_response.content[0].text if tool_response.content else ""
                })

                
            print(f"self.messages:{self.messages}\n")
            # 再次调用API，让模型基于工具调用结果生成最终回复
            final_response = self.openai_client.chat.completions.create(
                model=self.model,
                messages=self.messages,
                tools=self.tools,
            )

            if final_response.choices:
                return final_response.choices[0].message

        return None

    async def loop(self):
        async with self.mcp_client:
            while True:
                question = input("User: ")
                if question.lower() in ["exit", "quit", "bye"]:
                    print("再见！")
                    break

                message = {
                    "role": "user",
                    "content": question
                }
                self.messages.append(message)

                response_message = await self.chat(self.messages)

                if response_message and response_message.content:
                    # 添加到消息历史
                    self.messages.append({
                        "role": "assistant",
                        "content": response_message.content
                    })
                    print("AI:", response_message.content)
                else:
                    print("AI: 抱歉，我没有收到回复。")


async def main():
    # 关键修正：创建UserClient的实例
    user_client = UserClient()  # 加上括号实例化

    # # 测试一次聊天
    # test_response = await user_client.chat([
    #     {"role": "user", "content": "绵阳今天的天气如何"}
    # ])
    # print("测试回复:", test_response)

    # 运行交互循环
    await user_client.loop()


if __name__ == '__main__':
    asyncio.run(main())