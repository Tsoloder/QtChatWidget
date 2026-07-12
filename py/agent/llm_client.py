import json
import logging
from dataclasses import dataclass
from typing import AsyncIterator, Literal

import httpx
from openai import AsyncOpenAI

logger = logging.getLogger(__name__)


@dataclass
class StreamDelta:
    text: str | None = None
    tool_call_delta: "ToolCallDelta | None" = None
    usage: dict | None = None


@dataclass
class ToolCallDelta:
    index: int
    id: str | None = None
    name: str | None = None
    args_fragment: str | None = None


def to_openai_tools(mcp_tools: list) -> list:
    return [
        {
            "type": "function",
            "function": {
                "name": tool.name,
                "description": tool.description or "",
                "parameters": tool.inputSchema or {"type": "object", "properties": {}},
            },
        }
        for tool in mcp_tools
    ]


def to_anthropic_tools(mcp_tools: list) -> list:
    return [
        {
            "name": tool.name,
            "description": tool.description or "",
            "input_schema": tool.inputSchema or {"type": "object", "properties": {}},
        }
        for tool in mcp_tools
    ]


def to_anthropic_messages(messages: list, system_prompt: str):
    """把 OpenAI 格式的 messages 转为 Anthropic 格式。
    返回 (system, messages) 元组。"""
    anthropic_messages = []
    for msg in messages:
        role = msg["role"]
        if role == "system":
            continue
        if role == "user":
            anthropic_messages.append(
                {"role": "user", "content": [{"type": "text", "text": msg["content"]}]}
            )
        elif role == "assistant":
            content = []
            if msg.get("content"):
                content.append({"type": "text", "text": msg["content"]})
            for tc in msg.get("tool_calls", []):
                try:
                    input_dict = json.loads(tc["function"]["arguments"])
                except (json.JSONDecodeError, KeyError):
                    input_dict = {}
                content.append(
                    {
                        "type": "tool_use",
                        "id": tc["id"],
                        "name": tc["function"]["name"],
                        "input": input_dict,
                    }
                )
            anthropic_messages.append({"role": "assistant", "content": content})
        elif role == "tool":
            anthropic_messages.append(
                {
                    "role": "user",
                    "content": [
                        {
                            "type": "tool_result",
                            "tool_use_id": msg["tool_call_id"],
                            "content": msg["content"],
                        }
                    ],
                }
            )
    system = system_prompt or None
    return system, anthropic_messages


async def stream_chat(
    messages: list,
    system_prompt: str,
    config,
    tools: list,
) -> AsyncIterator[dict]:
    """流式调用 LLM。yield 三种事件：
    {"type": "text_chunk", "delta": "..."}
    {"type": "tool_call", "id": "...", "name": "...", "args": {...}}
    {"type": "usage", "input": N, "output": N, "total": N}
    """
    if config.api_type == "openai":
        async for event in _stream_openai(messages, system_prompt, config, tools):
            yield event
    else:
        async for event in _stream_anthropic(messages, system_prompt, config, tools):
            yield event


async def _stream_openai(messages, system_prompt, config, tools) -> AsyncIterator[dict]:
    base_url = config.api_url.rstrip("/")
    if not base_url.endswith("/v1"):
        base_url += "/v1"
    client = AsyncOpenAI(base_url=base_url, api_key=config.api_key)

    full_messages = []
    if system_prompt:
        full_messages.append({"role": "system", "content": system_prompt})
    full_messages.extend(messages)

    openai_tools = to_openai_tools(tools) if tools else None

    response = await client.chat.completions.create(
        model=config.model_id,
        messages=full_messages,
        tools=openai_tools,
        stream=True,
        stream_options={"include_usage": True} if openai_tools else None,
    )

    tool_call_buffers = {}

    async for chunk in response:
        delta = _extract_openai_delta(chunk)
        if delta.text:
            yield {"type": "text_chunk", "delta": delta.text}
        if delta.tool_call_delta:
            idx = delta.tool_call_delta.index
            if idx not in tool_call_buffers:
                tool_call_buffers[idx] = {
                    "id": delta.tool_call_delta.id or "",
                    "name": delta.tool_call_delta.name or "",
                    "args_str": "",
                }
            if delta.tool_call_delta.id:
                tool_call_buffers[idx]["id"] = delta.tool_call_delta.id
            if delta.tool_call_delta.name:
                tool_call_buffers[idx]["name"] = delta.tool_call_delta.name
            if delta.tool_call_delta.args_fragment:
                tool_call_buffers[idx]["args_str"] += delta.tool_call_delta.args_fragment
        if delta.usage:
            yield delta.usage

    for idx in sorted(tool_call_buffers.keys()):
        tc = tool_call_buffers[idx]
        try:
            args = json.loads(tc["args_str"]) if tc["args_str"] else {}
        except json.JSONDecodeError:
            logger.warning(f"tool_call args parse failed: {tc['args_str']}")
            args = {}
        yield {"type": "tool_call", "id": tc["id"], "name": tc["name"], "args": args}


def _extract_openai_delta(chunk) -> StreamDelta:
    delta_obj = StreamDelta()
    if not chunk.choices:
        if hasattr(chunk, "usage") and chunk.usage:
            delta_obj.usage = {
                "type": "usage",
                "input": chunk.usage.prompt_tokens or 0,
                "output": chunk.usage.completion_tokens or 0,
                "total": chunk.usage.total_tokens or 0,
            }
        return delta_obj

    choice = chunk.choices[0]
    delta = choice.delta

    if delta and delta.content:
        delta_obj.text = delta.content

    if delta and delta.tool_calls:
        for tc_delta in delta.tool_calls:
            delta_obj.tool_call_delta = ToolCallDelta(
                index=tc_delta.index,
                id=tc_delta.id,
                name=tc_delta.function.name if tc_delta.function else None,
                args_fragment=tc_delta.function.arguments if tc_delta.function else None,
            )

    return delta_obj


async def _stream_anthropic(messages, system_prompt, config, tools) -> AsyncIterator[dict]:
    system, anthropic_messages = to_anthropic_messages(messages, system_prompt)
    anthropic_tools = to_anthropic_tools(tools) if tools else None

    url = config.api_url.rstrip("/")
    if not url.endswith("/v1/messages"):
        if url.endswith("/v1"):
            url = url + "/messages"
        else:
            url = url + "/v1/messages"

    headers = {
        "x-api-key": config.api_key,
        "anthropic-version": "2023-06-01",
        "content-type": "application/json",
    }
    body = {
        "model": config.model_id,
        "messages": anthropic_messages,
        "max_tokens": 4096,
        "stream": True,
    }
    if system:
        body["system"] = system
    if anthropic_tools:
        body["tools"] = anthropic_tools

    tool_call_buffers = {}
    input_tokens = 0
    output_tokens = 0

    async with httpx.AsyncClient() as client:
        async with client.stream("POST", url, headers=headers, json=body, timeout=120.0) as resp:
            if resp.status_code != 200:
                text = await resp.aread()
                logger.error(f"anthropic error {resp.status_code}: {text.decode()}")
                yield {
                    "type": "error",
                    "message": f"Anthropic API error {resp.status_code}",
                    "retryable": False,
                }
                return

            event_type = None
            data_lines = []
            async for line in resp.aiter_lines():
                line = line.strip()
                if not line:
                    if event_type and data_lines:
                        data_str = "\n".join(data_lines)
                        try:
                            data = json.loads(data_str)
                        except json.JSONDecodeError:
                            data = {}
                        delta = _extract_anthropic_delta(event_type, data, tool_call_buffers)
                        if delta:
                            if delta.text:
                                yield {"type": "text_chunk", "delta": delta.text}
                            if delta.tool_call_delta:
                                idx = delta.tool_call_delta.index
                                if idx not in tool_call_buffers:
                                    tool_call_buffers[idx] = {
                                        "id": delta.tool_call_delta.id or "",
                                        "name": delta.tool_call_delta.name or "",
                                        "args_str": "",
                                    }
                                if delta.tool_call_delta.id:
                                    tool_call_buffers[idx]["id"] = delta.tool_call_delta.id
                                if delta.tool_call_delta.name:
                                    tool_call_buffers[idx]["name"] = delta.tool_call_delta.name
                                if delta.tool_call_delta.args_fragment:
                                    tool_call_buffers[idx]["args_str"] += delta.tool_call_delta.args_fragment
                            if delta.usage:
                                if "input" in delta.usage:
                                    input_tokens = delta.usage["input"]
                                if "output" in delta.usage:
                                    output_tokens = delta.usage["output"]
                    event_type = None
                    data_lines = []
                    continue
                if line.startswith("event: "):
                    event_type = line[7:]
                elif line.startswith("data: "):
                    data_lines.append(line[6:])

    if input_tokens or output_tokens:
        yield {
            "type": "usage",
            "input": input_tokens,
            "output": output_tokens,
            "total": input_tokens + output_tokens,
        }

    for idx in sorted(tool_call_buffers.keys()):
        tc = tool_call_buffers[idx]
        try:
            args = json.loads(tc["args_str"]) if tc["args_str"] else {}
        except json.JSONDecodeError:
            logger.warning(f"anthropic tool_call args parse failed: {tc['args_str']}")
            args = {}
        yield {"type": "tool_call", "id": tc["id"], "name": tc["name"], "args": args}


def _extract_anthropic_delta(event_type: str, data: dict, buffers: dict) -> StreamDelta | None:
    delta_obj = StreamDelta()

    if event_type == "message_start":
        usage = data.get("message", {}).get("usage", {})
        if usage:
            delta_obj.usage = {"input": usage.get("input_tokens", 0)}

    elif event_type == "content_block_start":
        block = data.get("content_block", {})
        idx = data.get("index", 0)
        if block.get("type") == "tool_use":
            delta_obj.tool_call_delta = ToolCallDelta(
                index=idx,
                id=block.get("id"),
                name=block.get("name"),
            )

    elif event_type == "content_block_delta":
        delta_data = data.get("delta", {})
        idx = data.get("index", 0)
        if delta_data.get("type") == "text_delta":
            delta_obj.text = delta_data.get("text", "")
        elif delta_data.get("type") == "input_json_delta":
            delta_obj.tool_call_delta = ToolCallDelta(
                index=idx,
                args_fragment=delta_data.get("partial", ""),
            )

    elif event_type == "message_delta":
        usage = data.get("usage", {})
        if usage:
            delta_obj.usage = {"output": usage.get("output_tokens", 0)}

    return delta_obj if (delta_obj.text or delta_obj.tool_call_delta or delta_obj.usage) else None
