import hashlib
import logging
from pathlib import Path

from config import ApiConfig

logger = logging.getLogger(__name__)

MAX_TURNS = 20

MODEL_LIMITS = {
    "gpt-4": 8192,
    "gpt-4-turbo": 128000,
    "gpt-4o": 128000,
    "claude-3-5-sonnet": 200000,
    "claude-3-opus": 200000,
    "claude-3-haiku": 200000,
    "mimo": 32000,
}


def max_tokens_for(model_id: str) -> int:
    lower = model_id.lower()
    for k, v in MODEL_LIMITS.items():
        if k in lower:
            return v
    return 32000


class TokenCounter:
    def __init__(self):
        self._calibration = 4.0

    def estimate(self, text: str) -> int:
        if not text:
            return 0
        return max(1, len(text) // int(self._calibration))

    def calibrate(self, text: str, actual_tokens: int):
        if actual_tokens > 0 and len(text) > 0:
            ratio = len(text) / actual_tokens
            self._calibration = 0.7 * self._calibration + 0.3 * ratio


class ContextManager:
    STALE_TURNS = 6
    LARGE_RESULT_CHARS = 30000
    SHORT_MSG_CHARS = 100

    def __init__(self):
        self.counter = TokenCounter()
        self._sessions_dir = Path(__file__).parent / "sessions"

    async def compress(self, messages: list, session_id: str, config: ApiConfig) -> list:
        max_tokens = max_tokens_for(config.model_id)
        threshold_warn = int(max_tokens * 0.8)
        threshold_micro = int(max_tokens * 0.6)
        current = sum(self.counter.estimate(self._msg_text(m)) for m in messages)

        messages = self._layer1_budget_truncate(messages, max_tokens)
        messages = self._layer2_stale_snip(messages, self.STALE_TURNS)
        if current > threshold_micro:
            messages = self._layer3_microcompact(messages)
        if current > threshold_warn:
            messages = await self._layer4_auto_compact(messages, config)
        return messages

    def _layer1_budget_truncate(self, messages, max_tokens):
        while sum(self.counter.estimate(self._msg_text(m)) for m in messages) > max_tokens * 0.9:
            if len(messages) <= 2:
                break
            messages.pop(0)
        return messages

    def _layer2_stale_snip(self, messages, stale_turns):
        for i, m in enumerate(messages):
            if m.get("role") == "tool" and i < len(messages) - stale_turns * 2:
                content = m.get("content", "")
                if isinstance(content, str) and len(content) > 200:
                    m["content"] = f"[Tool result from turn {i}: {content[:200]}...]"
        return messages

    def _layer3_microcompact(self, messages):
        result = []
        i = 0
        while i < len(messages):
            if (
                messages[i].get("role") == "assistant"
                and not messages[i].get("tool_calls")
                and len(self._msg_text(messages[i])) < self.SHORT_MSG_CHARS
            ):
                group = [messages[i]]
                j = i + 1
                while (
                    j < len(messages)
                    and messages[j].get("role") == "assistant"
                    and not messages[j].get("tool_calls")
                    and len(self._msg_text(messages[j])) < self.SHORT_MSG_CHARS
                ):
                    group.append(messages[j])
                    j += 1
                if len(group) >= 3:
                    merged = " ".join(self._msg_text(m) for m in group)
                    result.append({"role": "assistant", "content": merged})
                    i = j
                    continue
            result.append(messages[i])
            i += 1
        return result

    async def _layer4_auto_compact(self, messages, config):
        try:
            summary = await self._summarize_via_llm(messages, config)
            return [
                {"role": "system", "content": f"[Previous conversation summary]: {summary}"}
            ]
        except Exception as e:
            logger.warning(f"auto-compact LLM call failed: {e}, fallback to truncation")
            return self._layer1_budget_truncate(
                messages, int(max_tokens_for(config.model_id) * 0.5)
            )

    async def _summarize_via_llm(self, messages, config) -> str:
        import llm_client

        conversation_text = "\n".join(
            f"{m['role']}: {self._msg_text(m)[:500]}" for m in messages[-20:]
        )
        summary_messages = [
            {
                "role": "user",
                "content": f"请用 200 字以内总结以下对话的要点：\n{conversation_text}",
            }
        ]
        result_text = ""
        async for event in llm_client.stream_chat(
            messages=summary_messages,
            system_prompt="你是对话总结助手。",
            config=config,
            tools=[],
        ):
            if event["type"] == "text_chunk":
                result_text += event["delta"]
        return result_text.strip()

    def _msg_text(self, msg) -> str:
        if isinstance(msg.get("content"), str):
            return msg["content"]
        if msg.get("tool_calls"):
            parts = []
            for tc in msg["tool_calls"]:
                parts.append(tc.get("function", {}).get("arguments", ""))
            return " ".join(parts)
        return ""

    def persist_large_result(self, result: str, session_id: str) -> str:
        if len(result) > self.LARGE_RESULT_CHARS:
            results_dir = self._sessions_dir / session_id / "results"
            path = results_dir / f"{hashlib.md5(result.encode()).hexdigest()}.txt"
            from session import atomic_write

            atomic_write(str(path), result)
            return f"[Result persisted to file: {path}, size: {len(result)} chars]"
        return result
