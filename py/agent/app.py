import argparse
import asyncio
import json
import logging
from contextlib import asynccontextmanager
from datetime import datetime
from pathlib import Path

import uvicorn
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse, StreamingResponse

from agent_loop import run_agent_loop
from config import ApiConfig, load_config, save_config
from context import ContextManager
from logging_setup import setup_logging
from mcp_bridge import McpBridge
from session import (
    Session,
    clear_session,
    create_session,
    delete_session,
    list_sessions,
    load_session,
    save_session,
)
from skill_runtime import SkillRegistry

setup_logging()
logger = logging.getLogger(__name__)

current_config: ApiConfig | None = load_config()
ctx_mgr = ContextManager()
skill_registry = SkillRegistry(SkillRegistry.default_roots())

_mcp: McpBridge | None = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    global _mcp
    _mcp = McpBridge(str(Path(__file__).parent.parent / "server.py"))
    try:
        await _mcp.connect()
    except Exception as e:
        logger.error(f"mcp connect failed: {e}")
    yield
    if _mcp:
        await _mcp.disconnect()


app = FastAPI(lifespan=lifespan)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:*", "http://127.0.0.1:*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/health")
async def health():
    return {"status": "ok", "config_loaded": current_config is not None}


@app.post("/config")
async def update_config(cfg: dict):
    global current_config
    try:
        current_config = ApiConfig(**cfg)
        save_config(current_config)
        return {"ok": True}
    except Exception as e:
        return JSONResponse({"error": str(e)}, status_code=400)


@app.post("/chat/stream")
async def chat_stream(body: dict):
    if current_config is None:
        return JSONResponse(
            {"error": "no config, POST /config first"}, status_code=400
        )
    if _mcp is None:
        return JSONResponse({"error": "mcp not ready"}, status_code=503)

    session_id = body.get("session_id", "")
    message = body.get("message", "")
    system_prompt = body.get("system_prompt", "")
    selected_skills = body.get("selected_skills", [])
    skill_roots = body.get("skill_roots", [])
    writable_skill_root = body.get("writable_skill_root", "")
    if skill_roots:
        # Qt sends authoritative local roots so deployed and development layouts work alike.
        skill_registry.set_roots(
            [str(path) for path in SkillRegistry.default_roots()] +
            [str(path) for path in skill_roots],
            writable_skill_root,
        )
    else:
        skill_registry.reload()

    if not isinstance(selected_skills, list):
        return JSONResponse({"error": "selected_skills must be an array"}, status_code=400)

    if not session_id or not message:
        return JSONResponse({"error": "missing session_id or message"}, status_code=400)

    save_events = {"tool_call", "tool_result", "done"}

    async def gen():
        session = load_session(session_id)
        if session is None:
            yield f"event: error\ndata: {json.dumps({'message': 'session not found', 'retryable': False}, ensure_ascii=False)}\n\n"
            return
        try:
            async for event in run_agent_loop(
                session, message, system_prompt, current_config, _mcp, ctx_mgr,
                skill_registry, selected_skills
            ):
                if event["type"] in save_events:
                    save_session(session)
                yield f"event: {event['type']}\ndata: {json.dumps(event, ensure_ascii=False)}\n\n"
        except asyncio.CancelledError:
            save_session(session)
            logger.info("client disconnected, session saved")
            raise
        except Exception as e:
            logger.exception("agent loop failed")
            save_session(session)
            yield f"event: error\ndata: {json.dumps({'message': str(e), 'retryable': False}, ensure_ascii=False)}\n\n"

    return StreamingResponse(gen(), media_type="text/event-stream")


@app.get("/sessions")
async def get_sessions():
    return {"sessions": list_sessions()}


@app.post("/sessions")
async def new_session(body: dict = None):
    body = body or {}
    title = body.get("title", "New Session")
    s = create_session(title)
    return {"id": s.id, "title": s.title, "created_at": s.created_at}


@app.get("/sessions/{session_id}")
async def get_session(session_id: str):
    s = load_session(session_id)
    if s is None:
        return JSONResponse({"error": "not found"}, status_code=404)
    return {
        "meta": {
            "id": s.id,
            "title": s.title,
            "created_at": s.created_at,
            "updated_at": s.updated_at,
        },
        "messages": s.messages,
    }


@app.put("/sessions/{session_id}/rename")
async def rename_session(session_id: str, body: dict = None):
    body = body or {}
    new_title = body.get("title", "").strip()
    if not new_title:
        return JSONResponse({"error": "title is required"}, status_code=400)
    s = load_session(session_id)
    if s is None:
        return JSONResponse({"error": "not found"}, status_code=404)
    old_title = s.title
    s.title = new_title
    s.updated_at = datetime.now().isoformat()
    save_session(s)
    update_index(s)
    logger.info(f"session renamed: {session_id} | {old_title} -> {new_title}")
    return {"ok": True, "id": s.id, "title": s.title}


@app.delete("/sessions/{session_id}")
async def remove_session(session_id: str):
    delete_session(session_id)
    return {"ok": True}


@app.post("/sessions/{session_id}/clear")
async def clear_session_endpoint(session_id: str):
    clear_session(session_id)
    return {"ok": True}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--host", default="127.0.0.1")
    args = parser.parse_args()
    uvicorn.run(app, host=args.host, port=args.port, log_level="info")
