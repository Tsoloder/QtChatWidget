import logging
from typing import AsyncIterator

import llm_client
from config import ApiConfig
from context import ContextManager, MAX_TURNS
from mcp_bridge import McpBridge
from session import Session
from skill_runtime import (
    SkillError,
    SkillRegistry,
    render_selected_skill,
    tool_is_allowed,
)

logger = logging.getLogger(__name__)


async def run_agent_loop(
    session: Session,
    user_message: str,
    base_system_prompt: str,
    config: ApiConfig,
    mcp: McpBridge,
    ctx_mgr: ContextManager,
    skill_registry: SkillRegistry,
    selected_skills: list = None,
) -> AsyncIterator[dict]:
    selected_skills = selected_skills or []
    selected_ids = []
    selected_params = {}
    for item in selected_skills:
        if not isinstance(item, dict):
            continue
        skill_id = str(item.get("id", "")).strip().lower()
        try:
            skill_registry.get(skill_id)
        except SkillError:
            continue
        if skill_id not in selected_ids:
            selected_ids.append(skill_id)
        params = item.get("params", {})
        selected_params[skill_id] = params if isinstance(params, dict) else {}

    session.append_user(user_message, selected_ids)
    loaded_skills = set(selected_ids)
    selected_bodies = []
    for skill_id in selected_ids:
        selected_bodies.append(render_selected_skill(
            skill_registry.read_skill(skill_id), selected_params.get(skill_id, {})
        ))

    system_parts = []
    if base_system_prompt:
        system_parts.append(base_system_prompt)
    system_parts.append(skill_registry.catalog_prompt(selected_ids))
    if selected_bodies:
        system_parts.append("<selected_skill_instructions>\n%s\n</selected_skill_instructions>" %
                            "\n\n".join(selected_bodies))
    system_prompt = "\n\n".join(system_parts)
    turn = 0

    while True:
        turn += 1
        if turn > MAX_TURNS:
            yield {"type": "error", "message": "Max turns reached", "retryable": False}
            return

        session.messages = await ctx_mgr.compress(session.messages, session.id, config)

        text_acc = ""
        tool_calls = []
        usage = {}

        try:
            allowed_patterns = []
            for skill_id in loaded_skills:
                for pattern in skill_registry.get(skill_id).allowed_tools:
                    if pattern not in allowed_patterns:
                        allowed_patterns.append(pattern)

            external_tools = [
                tool for tool in mcp.available_tools()
                if tool_is_allowed(tool.name, allowed_patterns)
            ]
            runtime_tools = skill_registry.internal_tools()
            async for event in llm_client.stream_chat(
                messages=session.messages,
                system_prompt=system_prompt,
                config=config,
                tools=external_tools + runtime_tools,
            ):
                if event["type"] == "text_chunk":
                    text_acc += event["delta"]
                    yield event
                elif event["type"] == "tool_call":
                    tool_calls.append(event)
                elif event["type"] == "usage":
                    usage = event
                elif event["type"] == "error":
                    yield event
                    return
        except Exception as e:
            logger.exception("stream_chat failed")
            yield {"type": "error", "message": f"LLM stream failed: {e}", "retryable": False}
            return

        if usage:
            ctx_mgr.counter.calibrate(str(session.messages), usage.get("input", 0))

        if tool_calls:
            session.append_assistant_with_tool_calls(text_acc, tool_calls)
            for tc in tool_calls:
                yield {
                    "type": "tool_call",
                    "id": tc["id"],
                    "name": tc["name"],
                    "args": tc["args"],
                }
                try:
                    if tc["name"] == "read_skill":
                        skill_id = str(tc["args"].get("skill_id", "")).strip().lower()
                        result = skill_registry.read_skill(skill_id)
                        loaded_skills.add(skill_id)
                    elif tc["name"] == "read_skill_resource":
                        result = skill_registry.read_resource(
                            str(tc["args"].get("skill_id", "")).strip().lower(),
                            str(tc["args"].get("relative_path", "")),
                        )
                    elif tc["name"] == "create_skill":
                        result = skill_registry.create_skill(
                            str(tc["args"].get("skill_id", "")).strip().lower(),
                            tc["args"].get("files", {}),
                            bool(tc["args"].get("overwrite", False)),
                        )
                    else:
                        current_patterns = []
                        for loaded_id in loaded_skills:
                            for pattern in skill_registry.get(loaded_id).allowed_tools:
                                if pattern not in current_patterns:
                                    current_patterns.append(pattern)
                        if not tool_is_allowed(tc["name"], current_patterns):
                            result = "Tool blocked by active Skill policy: %s" % tc["name"]
                        else:
                            result = await mcp.call_tool(tc["name"], tc["args"])
                    result = ctx_mgr.persist_large_result(result, session.id)
                except Exception as e:
                    result = f"Tool error: {e}"
                    logger.warning(f"tool {tc['name']} failed: {e}")
                yield {
                    "type": "tool_result",
                    "call_id": tc["id"],
                    "name": tc["name"],
                    "result": result,
                }
                session.append_tool_result(tc["id"], result)
        else:
            session.append_assistant(text_acc)
            yield {
                "type": "done",
                "session_id": session.id,
                "tokens": usage.get("total", 0),
            }
            return
