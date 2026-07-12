import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from fastapi.testclient import TestClient

import app as agent_app
import session as session_module


class SessionApiTests(unittest.TestCase):
    def test_rename_updates_meta_and_index(self):
        with tempfile.TemporaryDirectory() as tmp:
            sessions_dir = Path(tmp)
            with patch.object(session_module, "SESSIONS_DIR", sessions_dir):
                created = session_module.create_session("Before")
                with TestClient(agent_app.app) as client:
                    response = client.put(
                        "/sessions/%s/rename" % created.id,
                        json={"title": "After"},
                    )
                self.assertEqual(200, response.status_code, response.text)
                self.assertEqual("After", response.json()["title"])
                loaded = session_module.load_session(created.id)
                self.assertIsNotNone(loaded)
                self.assertEqual("After", loaded.title)
                index = session_module.list_sessions()
                self.assertEqual("After", index[0]["title"])

    def test_rename_rejects_empty_title(self):
        with TestClient(agent_app.app) as client:
            response = client.put(
                "/sessions/nonexistent/rename", json={"title": "   "}
            )
        self.assertEqual(400, response.status_code)


if __name__ == "__main__":
    unittest.main()
