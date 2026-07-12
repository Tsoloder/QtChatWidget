import asyncio
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from agent_loop import run_agent_loop
from session import Session
from skill_runtime import RuntimeTool, SkillRegistry


class DummyConfig:
    api_type = "openai"
    api_url = "http://unused"
    api_key = "unused"
    model_id = "mimo-test"


class DummyCounter:
    def calibrate(self, _text, _tokens):
        pass


class DummyContext:
    counter = DummyCounter()

    async def compress(self, messages, _session_id, _config):
        return messages

    def persist_large_result(self, result, _session_id):
        return result


class DummyMcp:
    def __init__(self):
        self.calls = []
        self._tools = [
            RuntimeTool("read_file", "read", {"type": "object", "properties": {}}),
            RuntimeTool("run_command", "run", {"type": "object", "properties": {}}),
        ]

    def available_tools(self):
        return self._tools

    async def call_tool(self, name, args):
        self.calls.append((name, args))
        return "external result"


class SkillIntegrationTests(unittest.TestCase):
    def _registry(self, tmp):
        skill_dir = Path(tmp) / "safe-review"
        skill_dir.mkdir()
        (skill_dir / "SKILL.md").write_text(
            "---\n"
            "name: safe-review\n"
            "description: Review safely.\n"
            "allowed-tools: [read_file]\n"
            "---\n"
            "# Safe review\nUse read_file only.\n",
            encoding="utf-8",
        )
        return SkillRegistry([Path(tmp)])

    def _session(self):
        return Session("s1", "New Session", "now", "now")

    def test_manual_selection_injects_body_and_filters_tools(self):
        async def scenario():
            with tempfile.TemporaryDirectory() as tmp:
                registry = self._registry(tmp)
                mcp = DummyMcp()
                captured = {}

                async def fake_stream(messages, system_prompt, config, tools):
                    captured["prompt"] = system_prompt
                    captured["tools"] = [tool.name for tool in tools]
                    yield {"type": "text_chunk", "delta": "ok"}
                    yield {"type": "usage", "input": 1, "output": 1, "total": 2}

                with patch("agent_loop.llm_client.stream_chat", fake_stream):
                    events = [event async for event in run_agent_loop(
                        self._session(), "review", "", DummyConfig(), mcp,
                        DummyContext(), registry,
                        [{"id": "safe-review", "params": {"scope": "src"}}],
                    )]
                self.assertIn("# Safe review", captured["prompt"])
                self.assertIn("<available_skills>", captured["prompt"])
                self.assertIn("read_file", captured["tools"])
                self.assertNotIn("run_command", captured["tools"])
                self.assertIn("read_skill", captured["tools"])
                self.assertEqual("done", events[-1]["type"])

        asyncio.run(scenario())

    def test_create_skill_tool_installs_package(self):
        async def scenario():
            with tempfile.TemporaryDirectory() as tmp:
                builtin = Path(tmp) / "builtin"
                user = Path(tmp) / "user"
                builtin.mkdir()
                registry = SkillRegistry([builtin, user])
                registry.set_roots([str(builtin), str(user)], str(user))
                mcp = DummyMcp()
                calls = {"round": 0}

                async def fake_stream(messages, system_prompt, config, tools):
                    calls["round"] += 1
                    if calls["round"] == 1:
                        self.assertIn("create_skill", [tool.name for tool in tools])
                        yield {
                            "type": "tool_call", "id": "create1", "name": "create_skill",
                            "args": {
                                "skill_id": "chat-created",
                                "files": {
                                    "SKILL.md": "---\nname: chat-created\ndescription: Created through chat.\n---\n# Chat Created\nDo the task.\n"
                                },
                            },
                        }
                    else:
                        yield {"type": "text_chunk", "delta": "created"}

                with patch("agent_loop.llm_client.stream_chat", fake_stream):
                    events = [event async for event in run_agent_loop(
                        self._session(), "create a skill", "", DummyConfig(), mcp,
                        DummyContext(), registry, [],
                    )]
                results = [event["result"] for event in events if event["type"] == "tool_result"]
                self.assertTrue(any("Created Skill 'chat-created'" in result for result in results))
                self.assertTrue((user / "chat-created" / "SKILL.md").is_file())

        asyncio.run(scenario())

    def test_model_can_read_skill_then_policy_blocks_disallowed_tool(self):
        async def scenario():
            with tempfile.TemporaryDirectory() as tmp:
                registry = self._registry(tmp)
                mcp = DummyMcp()
                calls = {"round": 0}

                async def fake_stream(messages, system_prompt, config, tools):
                    calls["round"] += 1
                    if calls["round"] == 1:
                        yield {
                            "type": "tool_call", "id": "t1", "name": "read_skill",
                            "args": {"skill_id": "safe-review"},
                        }
                    elif calls["round"] == 2:
                        # Simulate a provider returning a stale/forged call even though
                        # run_command was filtered from the advertised schemas.
                        yield {
                            "type": "tool_call", "id": "t2", "name": "run_command",
                            "args": {},
                        }
                    else:
                        yield {"type": "text_chunk", "delta": "finished"}

                with patch("agent_loop.llm_client.stream_chat", fake_stream):
                    events = [event async for event in run_agent_loop(
                        self._session(), "please review", "", DummyConfig(), mcp,
                        DummyContext(), registry, [],
                    )]
                results = [event["result"] for event in events if event["type"] == "tool_result"]
                self.assertTrue(any("Skill: safe-review" in result for result in results))
                self.assertTrue(any("Tool blocked" in result for result in results))
                self.assertEqual([], mcp.calls)

        asyncio.run(scenario())


if __name__ == "__main__":
    unittest.main()
