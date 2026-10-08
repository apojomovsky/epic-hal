"""SIM_VARIANTS must match the run_one gates, or the local replica skips them.

scripts/ci-local-emit.py emits sim build scripts for its SIM_VARIANTS list
while scripts/ci-target-sim.sh runs every run_one gate; a pair in one list
but not the other means `make target-ci` either builds a hex no gate uses
or runs a gate whose hex was never emitted (epic-hal#355)."""
import importlib.util
import pathlib
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]


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


if __name__ == "__main__":
    unittest.main()
