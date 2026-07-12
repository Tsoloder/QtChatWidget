import json
import os
import tempfile
import uuid
import logging
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path

logger = logging.getLogger(__name__)

SESSIONS_DIR = Path(__file__).parent / "sessions"


def atomic_write(path: str, data: str):
    dir_ = os.path.dirname(path)
    os.makedirs(dir_, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=dir_, suffix=".tmp")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(data)
        os.replace(tmp, path)
    except Exception:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise


@dataclass
class Session:
    id: str
    title: str
    created_at: str
    updated_at: str
    messages: list = field(default_factory=list)

    def append_user(self, content: str, active_skills=None):
        message = {"role": "user", "content": content}
        if active_skills:
            message["active_skills"] = list(active_skills)
        self.messages.append(message)
        if len(self.messages) == 1 and (self.title == "New Session" or not self.title):
            self.title = content[:30] + ("..." if len(content) > 30 else "")
        self.updated_at = datetime.now().isoformat()

    def append_assistant(self, content: str):
        self.messages.append({"role": "assistant", "content": content})
        self.updated_at = datetime.now().isoformat()

    def append_assistant_with_tool_calls(self, text: str, tool_call_events: list):
        tool_calls_stored = [
            {
                "id": tc["id"],
                "type": "function",
                "function": {
                    "name": tc["name"],
                    "arguments": json.dumps(tc["args"], ensure_ascii=False),
                },
            }
            for tc in tool_call_events
        ]
        self.messages.append(
            {
                "role": "assistant",
                "content": text or None,
                "tool_calls": tool_calls_stored,
            }
        )
        self.updated_at = datetime.now().isoformat()

    def append_tool_result(self, tool_call_id: str, result: str):
        self.messages.append(
            {
                "role": "tool",
                "tool_call_id": tool_call_id,
                "content": result,
            }
        )
        self.updated_at = datetime.now().isoformat()


def create_session(title: str = "New Session") -> Session:
    sid = str(uuid.uuid4())
    now = datetime.now().isoformat()
    s = Session(id=sid, title=title, created_at=now, updated_at=now)
    (SESSIONS_DIR / sid / "results").mkdir(parents=True, exist_ok=True)
    save_session(s)
    update_index(s)
    logger.info(f"session created: {sid}")
    return s


def save_session(s: Session):
    meta = {
        "id": s.id,
        "title": s.title,
        "created_at": s.created_at,
        "updated_at": s.updated_at,
    }
    atomic_write(
        str(SESSIONS_DIR / s.id / "meta.json"),
        json.dumps(meta, ensure_ascii=False, indent=2),
    )
    atomic_write(
        str(SESSIONS_DIR / s.id / "messages.json"),
        json.dumps(s.messages, ensure_ascii=False),
    )


def load_session(sid: str) -> Session | None:
    meta_path = SESSIONS_DIR / sid / "meta.json"
    if not meta_path.exists():
        return None
    try:
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        msgs_path = SESSIONS_DIR / sid / "messages.json"
        msgs = json.loads(msgs_path.read_text(encoding="utf-8")) if msgs_path.exists() else []
        return Session(
            id=meta["id"],
            title=meta["title"],
            created_at=meta["created_at"],
            updated_at=meta["updated_at"],
            messages=msgs,
        )
    except Exception as e:
        logger.error(f"load_session {sid} failed: {e}")
        return None


def list_sessions() -> list:
    index_path = SESSIONS_DIR / "index.json"
    if index_path.exists():
        try:
            return json.loads(index_path.read_text(encoding="utf-8"))
        except Exception:
            return []
    return []


def update_index(s: Session):
    index_path = SESSIONS_DIR / "index.json"
    index = []
    if index_path.exists():
        try:
            index = json.loads(index_path.read_text(encoding="utf-8"))
        except Exception:
            index = []
    index = [item for item in index if item.get("id") != s.id]
    index.insert(0, {"id": s.id, "title": s.title, "updated_at": s.updated_at})
    atomic_write(str(index_path), json.dumps(index, ensure_ascii=False, indent=2))


def delete_session(sid: str):
    session_dir = SESSIONS_DIR / sid
    if not session_dir.exists():
        return
    trash_dir = SESSIONS_DIR / ".trash"
    trash_dir.mkdir(parents=True, exist_ok=True)
    import shutil

    dest = trash_dir / sid
    if dest.exists():
        shutil.rmtree(dest)
    shutil.move(str(session_dir), str(dest))
    index_path = SESSIONS_DIR / "index.json"
    if index_path.exists():
        index = json.loads(index_path.read_text(encoding="utf-8"))
        index = [item for item in index if item.get("id") != sid]
        atomic_write(str(index_path), json.dumps(index, ensure_ascii=False, indent=2))
    logger.info(f"session deleted: {sid}")


def clear_session(sid: str):
    s = load_session(sid)
    if s is None:
        return
    s.messages = []
    save_session(s)
    update_index(s)
    logger.info(f"session cleared: {sid}")
