"""Unit tests for scripts/config-key-audit.py."""
import importlib.util
import pathlib
import sys
import unittest
from unittest import mock

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))


def _load():
    """Import config-key-audit.py, whose filename is not a valid module name."""
    path = SCRIPTS / "config-key-audit.py"
    spec = importlib.util.spec_from_file_location("config_key_audit", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


audit = _load()


class TestLinkConfigTuIsThreadSafe(unittest.TestCase):
    """The shared main TU must not be written from the worker threads.

    link_config_tu runs on a thread pool while other workers' xc8-cc
    containers read the same path. Rewriting it there truncates the file
    under them, which CI saw as a run of "null character ignored" warnings
    followed by error (1091) main function "_main" not defined.
    """

    def test_link_config_tu_does_not_write_the_main_tu(self):
        """The worker only reads the main TU path; it never rewrites it."""
        with mock.patch.object(audit.subprocess, "run") as run, \
                mock.patch.object(pathlib.Path, "write_text") as write:
            run.return_value = mock.Mock(stdout="", stderr="")
            audit.link_config_tu("16F877A", "Microchip.PIC16Fxxx_DFP",
                                 "build-sim/audit-config/x.c")
        write.assert_not_called()

    def test_link_config_tu_still_names_the_main_tu(self):
        """The link command references the shared main TU by its path."""
        with mock.patch.object(audit.subprocess, "run") as run:
            run.return_value = mock.Mock(stdout="", stderr="")
            audit.link_config_tu("16F877A", "Microchip.PIC16Fxxx_DFP",
                                 "build-sim/audit-config/x.c")
        cmd = " ".join(run.call_args[0][0])
        self.assertIn(audit.MAIN_REL, cmd)
        self.assertIn("build-sim/audit-config/x.c", cmd)
        self.assertIn("-mcpu=16f877a", cmd)

    def test_main_tu_path_is_a_module_constant(self):
        """The path is shared state, so it lives in one place."""
        self.assertEqual(
            audit.MAIN_REL, "build-sim/audit-config/_audit_main.c")

    def test_main_tu_defines_main(self):
        """The trivial TU must actually define main, or every link fails."""
        self.assertIn("void main(void)", audit.MAIN_TU)


if __name__ == "__main__":
    unittest.main(verbosity=0)
