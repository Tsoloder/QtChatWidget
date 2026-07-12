import logging
from pathlib import Path

LOG_PATH = Path(__file__).parent / "agent.log"


def setup_logging():
    logging.basicConfig(
        filename=str(LOG_PATH),
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        encoding="utf-8",
    )
