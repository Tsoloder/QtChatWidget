import json
import logging
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Literal

logger = logging.getLogger(__name__)

CONFIG_PATH = Path(__file__).parent / "config.json"


@dataclass
class ApiConfig:
    api_type: Literal["openai", "anthropic"]
    api_url: str
    api_key: str
    model_id: str


def load_config() -> ApiConfig | None:
    if CONFIG_PATH.exists():
        try:
            data = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
            return ApiConfig(**data)
        except Exception as e:
            logger.warning(f"load_config failed: {e}")
    return None


def save_config(cfg: ApiConfig):
    from session import atomic_write
    atomic_write(str(CONFIG_PATH), json.dumps(asdict(cfg), ensure_ascii=False, indent=2))
    logger.info("config saved")
