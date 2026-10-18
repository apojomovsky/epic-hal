"""SIM_VARIANTS must match the run_one gates, or the local replica skips them.

scripts/ci/ci-local-emit.py emits sim build scripts for its SIM_VARIANTS list,
the flake-hunt job in .github/workflows/ci.yml and the emit step in
.github/workflows/family-check.yml each embed a copy of that list inline,
and scripts/ci/ci-target-sim.sh runs every run_one gate. A pair
missing from any of them means a gate with no emitted hex, or a hex no gate
uses (epic-hal#355, epic-hal#365)."""
import importlib.util
import pathlib
import re
import sys
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
WORKFLOW = SCRIPTS.parent / ".github" / "workflows" / "ci.yml"
FAMILY_CHECK = SCRIPTS.parent / ".github" / "workflows" / "family-check.yml"
FLAKE_HUNT_START = "- name: Emit sim build scripts"
FLAKE_HUNT_END = "- name: REPEAT=5 sim gate hunt"
FAMILY_CHECK_START = "for module, mcu in ["
FAMILY_CHECK_END = "]:"


def load_sim_variants():
    spec = importlib.util.spec_from_file_location(
        "ci_local_emit", SCRIPTS / "ci" / "ci-local-emit.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.SIM_VARIANTS


def load_run_one_pairs():
    pairs = set()
    for line in (SCRIPTS / "ci" / "ci-target-sim.sh").read_text().splitlines():
        parts = line.split()
        if len(parts) < 5 or parts[0] != "run_one":
            continue
        pairs.add((parts[4], parts[2]))
    return pairs

def load_run_one_triples():
    """(module, mcu, gate) per run_one line; gate is None for the
    default gate. The gate is always the last field when present:
    eeprom, TOGGLE_REG and TOGGLE_STEPI keep their positions."""
    triples = set()
    for line in (SCRIPTS / "ci" / "ci-target-sim.sh").read_text().splitlines():
        parts = line.split()
        if len(parts) < 5 or parts[0] != "run_one":
            continue
        gate = parts[10] if len(parts) > 10 else None
        triples.add((parts[4], parts[2], gate))
    return triples


def load_manifest():
    sys.path.insert(0, str(SCRIPTS / "build"))
    import epicmanifest
    return epicmanifest.load(epicmanifest.default_path())


def load_flake_hunt_pairs():
    text = WORKFLOW.read_text()
    start = text.index(FLAKE_HUNT_START)
    end = text.index(FLAKE_HUNT_END, start)
    return set(re.findall(r'\("([^"]+)", "([^"]+)"\)', text[start:end]))


def load_family_check_pairs():
    text = FAMILY_CHECK.read_text()
    start = text.index(FAMILY_CHECK_START)
    end = text.index(FAMILY_CHECK_END, start)
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

    def test_family_check_covers_every_sim_gate(self):
        """The family-check emit list builds a hex for every run_one gate."""
        missing = load_run_one_pairs() - load_family_check_pairs()
        self.assertEqual(missing, set())

    def test_family_check_has_no_pair_without_a_sim_gate(self):
        """Every family-check emit pair is exercised by a run_one gate."""
        extra = load_family_check_pairs() - load_run_one_pairs()
        self.assertEqual(extra, set())

    def manifest_gates(self):
        """(module, mcu, gate) triples the manifest resolves, keyed to
        the run_one pairs: the default gate as None, extras by name."""
        manifest = load_manifest()
        triples = set()
        for module, mcu in load_run_one_pairs():
            fam = manifest.family_of(mcu)
            default = manifest.sim_variant_for(module, fam.name, mcu)
            for gate in manifest.sim_gates_for(module, fam.name, mcu):
                triples.add((module, mcu, None
                             if default is not None
                             and gate.name == default.name else gate.name))
        return triples

    def test_every_extra_gate_has_a_run_one_line(self):
        """A manifest extra gate with no run_one line never runs."""
        missing = {t for t in self.manifest_gates() if t[2] is not None} \
            - load_run_one_triples()
        self.assertEqual(missing, set())

    def test_no_run_one_line_without_a_manifest_gate(self):
        """A named run_one gate must exist in the manifest as an extra;
        the default gate is always addressed bare, never by name."""
        extra = load_run_one_triples() - self.manifest_gates()
        self.assertEqual(extra, set())

    def test_run_one_gate_lines_are_unique(self):
        """A duplicated gate line would run the same firmware twice."""
        lines = [
            line.split() for line in
            (SCRIPTS / "ci" / "ci-target-sim.sh").read_text().splitlines()
            if line.split()[:1] == ["run_one"]
        ]
        triples = [(p[4], p[2], p[10] if len(p) > 10 else None)
                   for p in lines if len(p) >= 5]
        self.assertEqual(len(triples), len(set(triples)))


if __name__ == "__main__":
    unittest.main()
