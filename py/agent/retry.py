import asyncio
import functools
import logging
import random

from openai import RateLimitError, APIConnectionError, APITimeoutError

logger = logging.getLogger(__name__)


def with_retry(max_retries=3, base_delay=1.0):
    def decorator(func):
        @functools.wraps(func)
        async def wrapper(*args, **kwargs):
            last_exc = None
            for attempt in range(max_retries):
                try:
                    return await func(*args, **kwargs)
                except (RateLimitError, APIConnectionError, APITimeoutError, asyncio.TimeoutError) as e:
                    last_exc = e
                    if attempt < max_retries - 1:
                        delay = base_delay * (2 ** attempt) + random.uniform(0, 0.25 * base_delay)
                        logger.warning(f"retry {attempt + 1}/{max_retries} after {delay:.2f}s: {e}")
                        await asyncio.sleep(delay)
                    else:
                        logger.error(f"max retries reached: {e}")
            raise last_exc

        return wrapper

    return decorator
