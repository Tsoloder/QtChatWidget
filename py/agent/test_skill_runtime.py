import tempfile
import unittest
from pathlib import Path

from skill_runtime import SkillError, SkillRegistry, parse_frontmatter, tool_is_allowed


class SkillRuntimeTests(unittest.TestCase):
    def _make_skill(self, root, name="code-review", extra=""):
        skill_dir = Path(root) / name
        skill_dir.mkdir(parents=True)
        (skill_dir / "SKILL.md").write_text(
            "---\n"
            "name: %s\n"
            "description: >\n"
            "  Review code for bugs and security problems.\n"
            "allowed-tools: [read_file, search_*]\n"
            "version: 1.0.0\n"
            "---\n"
            "# Review\n\nRead references/rules.md.\n%s" % (name, extra),
            encoding="utf-8",
        )
        return skill_dir

    def test_multiline_description_and_catalog(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._make_skill(tmp)
            registry = SkillRegistry([Path(tmp)])
            skill = registry.get("code-review")
            self.assertIn("security problems", skill.description)
            self.assertIn("<name>code-review</name>", registry.catalog_prompt())

    def test_read_resource_cannot_escape(self):
        with tempfile.TemporaryDirectory() as tmp:
            skill_dir = self._make_skill(tmp)
            refs = skill_dir / "references"
            refs.mkdir()
            (refs / "rules.md").write_text("safe", encoding="utf-8")
            registry = SkillRegistry([Path(tmp)])
            self.assertEqual("safe", registry.read_resource("code-review", "references/rules.md"))
            with self.assertRaises(SkillError):
                registry.read_resource("code-review", "../secret.txt")

    def test_invalid_skill_id_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._make_skill(tmp, "folder", extra="")
            path = Path(tmp) / "folder" / "SKILL.md"
            path.write_text("---\nname: ../bad\ndescription: bad\n---\nbody", encoding="utf-8")
            registry = SkillRegistry([Path(tmp)])
            self.assertEqual([], registry.all())

    def test_tool_policy(self):
        allowed = ["read_file", "search_*"]
        self.assertTrue(tool_is_allowed("read_file", allowed))
        self.assertTrue(tool_is_allowed("search_code", allowed))
        self.assertFalse(tool_is_allowed("run_command", allowed))

    def test_frontmatter_requires_line_delimiters(self):
        with self.assertRaises(SkillError):
            parse_frontmatter("--- name: bad ---")

    def test_create_skill_package(self):
        with tempfile.TemporaryDirectory() as tmp:
            builtin = Path(tmp) / "builtin"
            user = Path(tmp) / "user"
            builtin.mkdir()
            registry = SkillRegistry([builtin, user])
            registry.set_roots([str(builtin), str(user)], str(user))
            result = registry.create_skill(
                "hello-skill",
                {
                    "SKILL.md": "---\nname: hello-skill\ndescription: Say hello when asked.\n---\n# Hello\nRespond warmly.\n",
                    "references/rules.md": "# Rules\nBe concise.\n",
                },
            )
            self.assertIn("Created Skill 'hello-skill'", result)
            self.assertTrue((user / "hello-skill" / "SKILL.md").is_file())
            self.assertEqual("# Rules\nBe concise.\n", registry.read_resource(
                "hello-skill", "references/rules.md"
            ))
            with self.assertRaises(SkillError):
                registry.create_skill("hello-skill", {
                    "SKILL.md": "---\nname: hello-skill\ndescription: duplicate\n---\nbody"
                })


if __name__ == "__main__":
    unittest.main()
