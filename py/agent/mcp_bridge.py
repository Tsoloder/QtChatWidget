import asyncio
import logging

from fastmcp import Client

logger = logging.getLogger(__name__)


class McpBridge:
    def __init__(self, script: str):
        self._script = script
        self._client = Client(script)
        self._lock = asyncio.Lock()
        self._entered = False
        self._cached_tools = []

    async def connect(self):
        await self._client.__aenter__()
        self._entered = True
        tools = await self.list_tools()
        self._cached_tools = tools
        logger.info(f"mcp connected, {len(tools)} tools available")

    async def disconnect(self):
        if self._entered:
            try:
                await self._client.__aexit__(None, None, None)
            except Exception as e:
                logger.warning(f"mcp disconnect error: {e}")
            self._entered = False

    async def list_tools(self):
        async with self._lock:
            return await self._client.list_tools()

    async def call_tool(self, name: str, args: dict) -> str:
        async with self._lock:
            result = await self._client.call_tool(name, args)
            return result.content[0].text if result.content else ""

    def available_tools(self) -> list:
        return self._cached_tools
