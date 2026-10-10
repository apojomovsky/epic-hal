"""SIM_VARIANTS must match the run_one gates, or the local replica skips them.

scripts/ci-local-emit.py emits sim build scripts for its SIM_VARIANTS list,
the flake-hunt job in .github/workflows/ci.yml embeds a third copy of that
list inline, and scripts/ci-target-sim.sh runs every run_one gate. A pair
missing from any of them means a gate with no emitted hex, or a hex no gate
uses (epic-hal#355, epic-hal#365)."""
import importlib.util
import pathlib
import re
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
WORKFLOW = SCRIPTS.parent / ".github" / "workflows" / "ci.yml"
FLAKE_HUNT_START = "- name: Emit sim build scripts"
FLAKE_HUNT_END = "- name: REPEAT=5 sim gate hunt"


def load_sim_variants():
    spec = importlib.util.spec_from_file_location(
        "ci_local_emit", SCRIPTS / "ci-local-emit.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.SIM_VARIANTS


def load_run_one_pairs():
    pairs = set()
    for line in (SCRIPTS / "ci-target-sim.sh").read_text().splitlines():
        parts = line.split()
        if len(parts) < 5 or parts[0] != "run_one":
            continue
        pairs.add((parts[4], parts[2]))
    return pairs


def load_flake_hunt_pairs():
    text = WORKFLOW.read_text()
    start = text.index(FLAKE_HUNT_START)
    end = text.index(FLAKE_HUNT_END, start)
    return set(re.findall(r'\("([^"]+)", "([^"]+)"\)', text[start:end]))


class TestSimVariantSync(unittest.TestCase):
    def test_emit_covers_every_sim_gate(self):
        """Each run_one (module, mcu) has an emitted sim build script."""
        missing = load_run_one_pairs() - set(load_sim_variants())
        self.assertEqual(missing, set())

    def test_no_emit_pair_without_a_sim_gate(self):
        """Each emitted sim pair is exercised by a run_one gate."""
        extra = set(load_sim_variants()) - load_run_one_pairs()
        self.assertEqual(extra, set())

    def test_emit_pairs_are_unique(self):
        """A duplicated pair would emit the same build script twice."""
        pairs = load_sim_variants()
        self.assertEqual(len(pairs), len(set(pairs)))

    def test_flake_hunt_covers_every_sim_gate(self):
        """The flake-hunt inline list builds a hex for every run_one gate."""
        missing = load_run_one_pairs() - load_flake_hunt_pairs()
        self.assertEqual(missing, set())

    def test_flake_hunt_has_no_pair_without_a_sim_gate(self):
        """Every flake-hunt pair is exercised by a run_one gate."""
        extra = load_flake_hunt_pairs() - load_run_one_pairs()
        self.assertEqual(extra, set())


if __name__ == "__main__":
    unittest.main()
