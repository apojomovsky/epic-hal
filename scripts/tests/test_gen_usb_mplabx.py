"""Unit tests for scripts/build/gen_usb_mplabx.py."""
import importlib.util
import pathlib
import sys
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
REPO = SCRIPTS.parent
sys.path.insert(0, str(SCRIPTS / "build"))


def _load():
    """Import the generator by path."""
    path = SCRIPTS / "build" / "gen_usb_mplabx.py"
    spec = importlib.util.spec_from_file_location("gen_usb_mplabx", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


gen = _load()


class GeneratorCase(unittest.TestCase):
    def test_committed_project_is_current(self):
        """The committed tree matches what the manifest produces."""
        self.assertEqual(gen.main.__doc__ is None, True)
        stale = [rel for rel, content in gen.generate().items()
                 if not (REPO / rel).is_file()
                 or (REPO / rel).read_text() != content]
        self.assertEqual(stale, [])

    def test_sources_come_from_the_manifest(self):
        sources, includes, defines, config = gen.project_contents()
        joined = " ".join(sources)
        self.assertIn("lib/usb/src/epic_usb.c", joined)
        self.assertIn("lib/usb/examples/example_usb.c", joined)
        # The config TU is the project's own, not a manifest source.
        self.assertIn("config.c", sources)
        self.assertIn("include", " ".join(includes))
        self.assertIn("third_party/m-stack/usb/include", " ".join(includes))
        self.assertIn("PIC18F4550", defines)
        self.assertIn("FOSC_HZ=48000000", defines)
        # The configuration words are the manifest example's, verbatim.
        self.assertIn("#pragma config BOR = OFF", config)
        self.assertIn("#pragma config USBDIV = 2", config)

    def test_paths_climb_back_to_the_repository_root(self):
        sources, includes, _, _ = gen.project_contents()
        depth = len(gen.PROJECT.relative_to(REPO).parts)
        for path in sources + includes:
            if path == "config.c":
                # The project's own config TU lives in the project.
                self.assertTrue((gen.PROJECT / path).is_file())
                continue
            self.assertTrue(path.startswith("/".join([".."] * depth)), path)
            # The rest of the path is a real repository path.
            resolved = (gen.PROJECT / path).resolve()
            self.assertTrue(resolved.exists(), path)

    def test_check_mode_passes_on_the_committed_tree(self):
        argv = sys.argv
        sys.argv = ["gen_usb_mplabx.py", "--check"]
        try:
            self.assertEqual(gen.main(), 0)
        finally:
            sys.argv = argv


if __name__ == "__main__":
    unittest.main()
