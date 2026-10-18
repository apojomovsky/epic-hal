"""Unit tests for scripts/build/epicmanifest.py."""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "build"))

import epicmanifest  # noqa: E402

MINIMAL = """
[toolchain]
min_epic_cc = "0.3.0"

[families.PIC16F87XA]
hal_dir  = "hal/pic14/16f87xa"
variants = ["16F873A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
fosc_hz  = 20000000
includes = ["hal/pic14/16f87xa/include", "common/include"]
hal_sources = ["hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c", "common/src/core/epic_harness_target.c"]
harness_src = "common/src/core/epic_harness_target.c"
epiccc_sources = ["hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c", "common/src/core/epic_harness_target.c"]

[[families.PIC16F87XA.conditional_sources]]
path     = "hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c"
variants = ["16F877A"]

[families.PIC18Fxx5x]
hal_dir  = "hal/pic18/18fxx5x"
variants = ["18F4550"]
dfp      = "Microchip.PIC18Fxxxx_DFP"
fosc_hz  = 48000000
includes = ["hal/pic18/18fxx5x/include", "common/include"]
hal_sources = ["hal/pic18/18fxx5x/src/peripherals/pic18fxx5x_gpio.c"]

[modules."lib/tick"]
summary    = "Tooling-test fixture."
sources    = ["src/epic_tick.c"]
includes   = ["include"]
depends_on = []

[modules."lib/tick".supported]
PIC16F87XA = ["16F873A", "16F877A"]
PIC18Fxx5x = ["18F4550"]

[modules."lib/tick".example.PIC16F87XA]
name    = "tick-blink"
sources = ["examples/example_tick.c"]
config  = { FOSC = "HS", WDTE = "ON" }

[modules."lib/tick".example.PIC16F87XA.sim]
name        = "tick-blink-sim"
harness_src = "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c"
config      = { FOSC = "HS", WDTE = "OFF" }

[modules."lib/tick".example.PIC18Fxx5x]
name    = "tick-blink"
sources = ["examples/example_tick.c"]
config  = { FOSC = "HS" }

[modules."lib/serial"]
summary    = "Tooling-test fixture."
sources    = ["src/epic_serial.c"]
includes   = ["include"]
depends_on = ["lib/tick"]

[modules."lib/serial".supported]
PIC16F87XA = ["16F877A"]

[modules."lib/serial".excluded]
"16F873A" = "RAM: 32-byte rx buffer does not fit"

[modules."lib/math"]
summary    = "Tooling-test fixture."
sources    = ["src/common/epic_math_sqrt.c", "src/common/epic_math_numeric.c"]
includes   = ["include"]
depends_on = []
needs_hal  = false

[modules."lib/math".sources_by_family]
PIC16F87XA = ["src/pic16/epic_math_mul.c", "src/pic16/epic_math_scratch.c"]
PIC18Fxx5x = ["src/pic18/epic_math_mul.c"]

[modules."lib/math".supported]
PIC16F87XA = ["16F873A", "16F877A"]
PIC18Fxx5x = ["18F4550"]

[modules."lib/math".example.PIC16F87XA]
name    = "math-smoke"
sources = ["tests/target_smoke16.c"]
hal     = true
config  = { FOSC = "HS" }
# The example logs over serial; the library does not, so the dep is
# scoped to the example (the per-MCU variant below drops it).
depends_on = ["lib/serial"]

[modules."lib/math".example.PIC16F87XA.variants.16F873A]
name    = "math-smoke-873"
sources = ["tests/target_smoke16.c"]
hal     = true

[modules."lib/math".example.PIC18Fxx5x]
name    = "math-selftest"
sources = ["tests/target_selftest.c"]
hal     = true
depends_on = ["lib/serial"]

[modules."lib/adcfilter"]
summary    = "Tooling-test fixture."
sources    = ["src/epic_adcfilter.c"]
includes   = ["include"]
depends_on = []
needs_hal  = false

[modules."lib/adcfilter".supported]
PIC16F87XA = ["16F873A", "16F877A"]

[modules."lib/adcfilter".example.PIC16F87XA]
name    = "adcfilter-sizecheck"
sources = ["mcu/target_sizecheck.c"]
"""


def write(text):
    tmp = tempfile.NamedTemporaryFile("w", suffix=".toml", delete=False)
    tmp.write(text)
    tmp.close()
    return pathlib.Path(tmp.name)


class TestLoad(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(MINIMAL))

    def test_families_parsed(self):
        fam = self.m.families["PIC16F87XA"]
        self.assertEqual(fam.hal_dir, "hal/pic14/16f87xa")
        self.assertEqual(fam.variants, ["16F873A", "16F877A"])
        self.assertEqual(fam.dfp, "Microchip.PIC16Fxxx_DFP")
        self.assertEqual(fam.fosc_hz, 20000000)

    def test_family_name_is_populated_from_the_table_key(self):
        self.assertEqual(self.m.families["PIC16F87XA"].name, "PIC16F87XA")

    def test_conditional_sources_parsed(self):
        cond = self.m.families["PIC16F87XA"].conditional_sources
        self.assertEqual(len(cond), 1)
        self.assertEqual(cond[0].variants, ["16F877A"])

    def test_modules_parsed(self):
        mod = self.m.modules["lib/serial"]
        self.assertEqual(mod.name, "lib/serial")
        self.assertEqual(mod.dir, "lib/serial")
        self.assertEqual(mod.depends_on, ["lib/tick"])
        self.assertEqual(mod.supported["PIC16F87XA"], ["16F877A"])
        self.assertEqual(mod.excluded["16F873A"], "RAM: 32-byte rx buffer does not fit")

    def test_needs_hal_defaults_to_true(self):
        self.assertTrue(self.m.modules["lib/tick"].needs_hal)

    def test_needs_hal_false_parsed(self):
        self.assertFalse(self.m.modules["lib/math"].needs_hal)

    def test_sources_by_family_parsed(self):
        sbf = self.m.modules["lib/math"].sources_by_family
        self.assertEqual(sbf["PIC16F87XA"], ["src/pic16/epic_math_mul.c", "src/pic16/epic_math_scratch.c"])
        self.assertEqual(sbf["PIC18Fxx5x"], ["src/pic18/epic_math_mul.c"])

    def test_examples_are_keyed_by_family(self):
        ex = self.m.modules["lib/tick"].examples
        self.assertEqual(ex["PIC16F87XA"].name, "tick-blink")
        self.assertEqual(ex["PIC18Fxx5x"].name, "tick-blink")

    def test_example_config_is_family_scoped(self):
        ex = self.m.modules["lib/tick"].examples["PIC16F87XA"]
        self.assertEqual(ex.config, {"FOSC": "HS", "WDTE": "ON"})

    def test_example_hal_defaults_to_module_needs_hal(self):
        # lib/adcfilter: needs_hal=false, no hal override -> hal False
        self.assertFalse(self.m.modules["lib/adcfilter"].examples["PIC16F87XA"].hal)

    def test_example_hal_can_override_module_needs_hal(self):
        # lib/math: needs_hal=false, example hal=true -> hal True
        self.assertTrue(self.m.modules["lib/math"].examples["PIC16F87XA"].hal)

    def test_example_config_is_empty_when_absent(self):
        self.assertEqual(self.m.modules["lib/adcfilter"].examples["PIC16F87XA"].config, {})

    def test_example_depends_on_parsed(self):
        self.assertEqual(self.m.modules["lib/math"].examples["PIC16F87XA"].depends_on,
                         ["lib/serial"])
        # Variants carry their own depends_on (default empty when absent).
        variant = self.m.modules["lib/math"].examples["PIC16F87XA"].variants["16F873A"]
        self.assertEqual(variant.depends_on, [])

    def test_example_is_none_when_no_entry_for_that_family(self):
        # lib/serial has no example table at all
        self.assertIsNone(self.m.modules["lib/serial"].examples.get("PIC16F87XA"))

    def test_excluded_defaults_to_empty(self):
        self.assertEqual(self.m.modules["lib/tick"].excluded, {})


class TestValidation(unittest.TestCase):
    def test_unknown_dependency_is_rejected(self):
        bad = MINIMAL.replace('depends_on = ["lib/tick"]', 'depends_on = ["epic-nope"]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("epic-nope", str(cm.exception))

    def test_unknown_family_in_supported_is_rejected(self):
        bad = MINIMAL.replace('[modules."lib/tick".supported]\nPIC16F87XA',
                              '[modules."lib/tick".supported]\nPIC99XXXX')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("PIC99XXXX", str(cm.exception))

    def test_supported_variant_not_in_family_is_rejected(self):
        bad = MINIMAL.replace('PIC16F87XA = ["16F873A", "16F877A"]',
                              'PIC16F87XA = ["16F873A", "16F999X"]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("16F999X", str(cm.exception))

    def test_variant_both_supported_and_excluded_is_rejected(self):
        bad = MINIMAL.replace('"16F873A" = "RAM: 32-byte rx buffer does not fit"',
                              '"16F877A" = "contradiction"')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("16F877A", str(cm.exception))

    def test_dependency_cycle_is_rejected(self):
        bad = MINIMAL.replace('[modules."lib/tick"]\nsummary    = "Tooling-test fixture."\nsources    = ["src/epic_tick.c"]\nincludes   = ["include"]\ndepends_on = []',
                              '[modules."lib/tick"]\nsummary    = "Tooling-test fixture."\nsources    = ["src/epic_tick.c"]\nincludes   = ["include"]\ndepends_on = ["lib/serial"]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("cycle", str(cm.exception).lower())

    def test_unknown_family_in_sources_by_family_is_rejected(self):
        bad = MINIMAL.replace('[modules."lib/math".sources_by_family]\nPIC16F87XA',
                              '[modules."lib/math".sources_by_family]\nPIC99XXXX')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("PIC99XXXX", str(cm.exception))

    def test_unknown_family_in_example_is_rejected(self):
        # Rename both the base example and its per-MCU variant table so
        # the family key is consistently unknown.
        bad = MINIMAL.replace('[modules."lib/math".example.PIC16F87XA.variants.16F873A]',
                              '[modules."lib/math".example.PIC99XXXX.variants.16F873A]')
        bad = bad.replace('[modules."lib/math".example.PIC16F87XA]',
                          '[modules."lib/math".example.PIC99XXXX]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("PIC99XXXX", str(cm.exception))

    def test_unknown_example_dependency_is_rejected(self):
        bad = MINIMAL.replace('depends_on = ["lib/serial"]\n\n[modules."lib/math".example.PIC18Fxx5x]',
                              'depends_on = ["lib/serial"]\n\n[modules."lib/math".example.PIC18Fxx5x]')
        bad = bad.replace('depends_on = ["lib/serial"]',
                          'depends_on = ["epic-nope"]', 1)
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("epic-nope", str(cm.exception))

    def test_missing_family_fosc_hz_is_rejected(self):
        bad = MINIMAL.replace('fosc_hz  = 20000000\n', '', 1)
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("fosc_hz", str(cm.exception))

    def test_unknown_key_in_module_table_is_rejected(self):
        # The issue's case: deleting the [modules."x".supported] header
        # leaves bare family keys in the module table, silently
        # accepted, and the family vanishes from the matrix.
        bad = MINIMAL.replace('[modules."lib/tick".supported]\n', "")
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("PIC16F87XA", str(cm.exception))

    def test_unknown_key_in_family_table_is_rejected(self):
        bad = MINIMAL.replace('[families.PIC18Fxx5x]\nhal_dir',
                              '[families.PIC18Fxx5x]\nhal_dri')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("hal_dri", str(cm.exception))

    def test_min_epic_cc_is_parsed(self):
        self.assertEqual(epicmanifest.load(write(MINIMAL)).min_epic_cc, "0.3.0")

    def test_missing_min_epic_cc_is_rejected(self):
        bad = MINIMAL.replace('[toolchain]\nmin_epic_cc = "0.3.0"\n', '')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("min_epic_cc", str(cm.exception))

    def test_malformed_min_epic_cc_is_rejected(self):
        bad = MINIMAL.replace('min_epic_cc = "0.3.0"', 'min_epic_cc = "0.3"')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("X.Y.Z", str(cm.exception))

    def test_unknown_toolchain_key_is_rejected(self):
        bad = MINIMAL.replace('[toolchain]\n', '[toolchain]\nmax_epic_cc = "0.4.0"\n')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("max_epic_cc", str(cm.exception))


class TestResolution(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(MINIMAL))

    def test_resolve_deps_puts_dependencies_first(self):
        self.assertEqual(self.m.resolve_deps("lib/serial"), ["lib/tick", "lib/serial"])

    def test_resolve_deps_of_a_leaf_is_just_itself(self):
        self.assertEqual(self.m.resolve_deps("lib/tick"), ["lib/tick"])

    def test_resolve_deps_rejects_unknown_module(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.resolve_deps("epic-nope")
    def test_resolve_module_accepts_an_id(self):
        mod = self.m.resolve_module("lib/tick")
        self.assertEqual((mod.name, mod.dir), ("lib/tick", "lib/tick"))

    def test_resolve_module_accepts_a_dir(self):
        mod = self.m.resolve_module("lib/tick")
        self.assertEqual(mod.name, "lib/tick")

    def test_resolve_module_ignores_a_trailing_slash(self):
        self.assertEqual(self.m.resolve_module("lib/tick/").name, "lib/tick")

    def test_resolve_module_ignores_a_trailing_slash_on_an_id(self):
        self.assertEqual(self.m.resolve_module("lib/tick/").name, "lib/tick")


    def test_resolve_module_rejects_unknown_values_naming_valid_ids(self):
        with self.assertRaisesRegex(
            epicmanifest.ManifestError, "unknown module 'epic-nope'.*lib/tick"
        ):
            self.m.resolve_module("epic-nope")


    def test_family_of_maps_a_part_to_its_family(self):
        self.assertEqual(self.m.family_of("16F877A").name, "PIC16F87XA")
        self.assertEqual(self.m.family_of("18F4550").name, "PIC18Fxx5x")

    def test_family_of_rejects_unknown_part(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.family_of("16F999X")

    def test_is_supported(self):
        self.assertTrue(self.m.is_supported("lib/serial", "PIC16F87XA", "16F877A"))
        self.assertFalse(self.m.is_supported("lib/serial", "PIC16F87XA", "16F873A"))

    def test_exclusion_reason(self):
        self.assertEqual(
            self.m.exclusion_reason("lib/serial", "16F873A"),
            "RAM: 32-byte rx buffer does not fit",
        )
        self.assertIsNone(self.m.exclusion_reason("lib/serial", "16F877A"))

    def test_example_for_returns_the_family_example(self):
        self.assertEqual(self.m.example_for("lib/math", "PIC16F87XA").name, "math-smoke")
        self.assertEqual(self.m.example_for("lib/math", "PIC18Fxx5x").name, "math-selftest")

    def test_example_for_returns_none_when_absent(self):
        self.assertIsNone(self.m.example_for("lib/serial", "PIC16F87XA"))

    def test_uses_hal_falls_back_to_needs_hal_without_an_example(self):
        # lib/serial has no example; uses_hal falls back to needs_hal (true)
        self.assertTrue(self.m.uses_hal("lib/serial", "16F877A"))

    def test_uses_hal_follows_the_example_override_when_present(self):
        # lib/math: needs_hal=false but example hal=true
        self.assertTrue(self.m.uses_hal("lib/math", "16F877A"))
        # lib/adcfilter: needs_hal=false, no override
        self.assertFalse(self.m.uses_hal("lib/adcfilter", "16F877A"))

    def test_hal_true_build_includes_family_hal_sources(self):
        srcs = self.m.sources_for("lib/tick", "16F877A")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c", srcs)

    def test_needs_hal_false_with_no_override_omits_family_hal_sources(self):
        srcs = self.m.sources_for("lib/adcfilter", "16F877A")
        self.assertNotIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c", srcs)

    def test_needs_hal_false_but_example_hal_true_includes_family_hal_sources(self):
        srcs = self.m.sources_for("lib/math", "16F877A")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c", srcs)

    def test_needs_hal_false_omits_family_includes(self):
        incs = self.m.includes_for("lib/adcfilter", "16F877A")
        self.assertNotIn("hal/pic14/16f87xa/include", incs)
        self.assertNotIn("common/include", incs)
        self.assertIn("lib/adcfilter/include", incs)

    def test_conditional_source_included_only_on_matching_variant(self):
        psp = "hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c"
        self.assertIn(psp, self.m.sources_for("lib/tick", "16F877A"))
        self.assertNotIn(psp, self.m.sources_for("lib/tick", "16F873A"))

    def test_conditional_source_omitted_when_hal_not_used(self):
        psp = "hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c"
        self.assertNotIn(psp, self.m.sources_for("lib/adcfilter", "16F877A"))

    def test_sources_include_module_and_example(self):
        srcs = self.m.sources_for("lib/tick", "16F877A")
        self.assertIn("lib/tick/src/epic_tick.c", srcs)
        self.assertIn("lib/tick/examples/example_tick.c", srcs)

    def test_sources_pull_in_dependency_sources(self):
        srcs = self.m.sources_for("lib/serial", "16F877A")
        self.assertIn("lib/tick/src/epic_tick.c", srcs)
        self.assertIn("lib/serial/src/epic_serial.c", srcs)

    def test_sources_pull_in_example_dependency_sources(self):
        # lib/math's example depends_on lib/serial (the library does
        # not), so the serial library joins the math example's build.
        srcs = self.m.sources_for("lib/math", "16F877A")
        self.assertIn("lib/serial/src/epic_serial.c", srcs)

    def test_includes_pull_in_example_dependency_includes(self):
        incs = self.m.includes_for("lib/math", "16F877A")
        self.assertIn("lib/serial/include", incs)

    def test_example_dependency_respects_per_mcu_variant_override(self):
        # The 16F873A variant of lib/math's example drops the serial
        # dep (it is excluded on that part for RAM), so the small-part
        # probe build must not compile epic-serial.
        srcs = self.m.sources_for("lib/math", "16F873A")
        self.assertNotIn("lib/serial/src/epic_serial.c", srcs)
        self.assertIn("lib/math/tests/target_smoke16.c", srcs)

    def test_example_dependency_sources_are_deduplicated(self):
        srcs = self.m.sources_for("lib/math", "16F877A")
        self.assertEqual(len(srcs), len(set(srcs)))

    def test_sources_only_include_the_requested_modules_example(self):
        srcs = self.m.sources_for("lib/serial", "16F877A")
        self.assertNotIn("lib/tick/examples/example_tick.c", srcs)

    def test_sources_by_family_contributes_only_the_matching_family(self):
        srcs16 = self.m.sources_for("lib/math", "16F877A")
        self.assertIn("lib/math/src/pic16/epic_math_mul.c", srcs16)
        self.assertIn("lib/math/src/pic16/epic_math_scratch.c", srcs16)
        self.assertNotIn("lib/math/src/pic18/epic_math_mul.c", srcs16)
        self.assertIn("lib/math/src/common/epic_math_sqrt.c", srcs16)
        srcs18 = self.m.sources_for("lib/math", "18F4550")
        self.assertIn("lib/math/src/pic18/epic_math_mul.c", srcs18)
        self.assertNotIn("lib/math/src/pic16/epic_math_mul.c", srcs18)
        self.assertIn("lib/math/src/common/epic_math_sqrt.c", srcs18)

    def test_the_right_example_is_used_per_family(self):
        self.assertIn("lib/math/tests/target_smoke16.c",
                      self.m.sources_for("lib/math", "16F877A"))
        self.assertIn("lib/math/tests/target_selftest.c",
                      self.m.sources_for("lib/math", "18F4550"))
        self.assertNotIn("lib/math/tests/target_selftest.c",
                         self.m.sources_for("lib/math", "16F877A"))

    def test_sources_have_no_duplicates(self):
        srcs = self.m.sources_for("lib/math", "16F877A")
        self.assertEqual(len(srcs), len(set(srcs)))

    def test_includes_preserve_family_order_then_modules(self):
        incs = self.m.includes_for("lib/serial", "16F877A")
        self.assertEqual(incs[0], "hal/pic14/16f87xa/include")
        self.assertEqual(incs[1], "common/include")
        self.assertIn("lib/tick/include", incs)
        self.assertIn("lib/serial/include", incs)

    def test_sim_variant_for_returns_the_sim_data(self):
        sim = self.m.sim_variant_for("lib/tick", "PIC16F87XA")
        self.assertEqual(sim.name, "tick-blink-sim")
        self.assertEqual(sim.harness_src,
                         "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c")

    def test_sim_variant_for_returns_none_without_one(self):
        self.assertIsNone(self.m.sim_variant_for("lib/tick", "PIC18Fxx5x"))

    def test_sources_for_sim_variant_swaps_the_harness_source(self):
        target = self.m.sources_for("lib/tick", "16F877A", variant="target")
        sim = self.m.sources_for("lib/tick", "16F877A", variant="sim")
        self.assertIn("common/src/core/epic_harness_target.c", target)
        self.assertNotIn("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c", target)
        self.assertIn("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c", sim)
        self.assertNotIn("common/src/core/epic_harness_target.c", sim)

    def test_sources_for_sim_variant_keeps_the_harness_source_position(self):
        sim = self.m.sources_for("lib/tick", "16F877A", variant="sim")
        target = self.m.sources_for("lib/tick", "16F877A", variant="target")
        target_pos = target.index("common/src/core/epic_harness_target.c")
        sim_pos = sim.index("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c")
        self.assertEqual(target_pos, sim_pos)

    def test_epic_cc_sim_variant_swaps_the_harness_source(self):
        # The mdb gate under epic-cc (lib/taskmgr's, epic-hal#236) needs
        # the sim variant's harness too, not the family slice's no-op
        # target harness.
        srcs = self.m.sources_for("lib/tick", "16F877A",
                                  variant="sim", toolchain="epic-cc")
        self.assertIn("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c", srcs)
        self.assertNotIn("common/src/core/epic_harness_target.c", srcs)

    def test_epic_cc_sim_variant_keeps_the_harness_position(self):
        sim = self.m.sources_for("lib/tick", "16F877A",
                                 variant="sim", toolchain="epic-cc")
        target = self.m.sources_for("lib/tick", "16F877A",
                                    toolchain="epic-cc")
        self.assertEqual(target.index("common/src/core/epic_harness_target.c"),
                         sim.index("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c"))

    def test_epic_cc_target_variant_keeps_the_family_harness(self):
        srcs = self.m.sources_for("lib/tick", "16F877A",
                                  variant="target", toolchain="epic-cc")
        self.assertIn("common/src/core/epic_harness_target.c", srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c", srcs)

    def test_sources_for_sim_variant_reuses_example_sources_when_no_override(self):
        sim = self.m.sources_for("lib/tick", "16F877A", variant="sim")
        self.assertIn("lib/tick/examples/example_tick.c", sim)

    def test_sources_for_sim_variant_raises_without_one(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.sources_for("lib/tick", "18F4550", variant="sim")

    def test_epiccc_sources_used_for_epic_cc_toolchain(self):
        srcs = self.m.sources_for("lib/tick", "16F877A", toolchain="epic-cc")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c", srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/epiccc/pic16f87xa_gpio_epiccc.c", srcs)

    def test_epiccc_sources_do_not_splice_conditional_sources(self):
        # Conditional sources are XC8 psect-order machinery; the epic-cc
        # path is a single whole-program invocation with no link order.
        psp = "hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c"
        srcs = self.m.sources_for("lib/tick", "16F877A", toolchain="epic-cc")
        self.assertNotIn(psp, srcs)

    def test_epiccc_sources_default_to_empty(self):
        fam = self.m.families["PIC18Fxx5x"]
        self.assertEqual(fam.epiccc_sources, [])

    def test_epic_cc_without_epiccc_sources_raises(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.sources_for("lib/tick", "18F4550", toolchain="epic-cc")

    def test_epiccc_sources_keep_the_example(self):
        srcs = self.m.sources_for("lib/tick", "16F877A", toolchain="epic-cc")
        self.assertIn("lib/tick/examples/example_tick.c", srcs)

    def test_xc8_path_is_unchanged_with_epiccc_sources_present(self):
        # The epiccc_sources key must not leak into the XC8 resolution.
        srcs = self.m.sources_for("lib/tick", "16F877A")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c", srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/epiccc/pic16f87xa_gpio_epiccc.c", srcs)



HALT = MINIMAL.replace(
    '[modules."lib/serial".supported]',
    'epiccc_hal_sources_by_family.PIC16F87XA = ['
    '"hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c", '
    '"common/src/core/epic_harness_target.c"]\n\n'
    '[modules."lib/serial".supported]'
).replace(
    '[modules."lib/serial".excluded]',
    '[modules."lib/serial".example.PIC16F87XA]\n'
    'name    = "serial"\n'
    'sources = ["examples/example_serial.c"]\n\n'
    '[modules."lib/serial".excluded]'
)


class TestEpicccHalSources(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(HALT))

    def test_override_replaces_the_family_slice_verbatim(self):
        srcs = self.m.sources_for("lib/serial", "16F877A",
                                  toolchain="epic-cc")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c",
                      srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c",
                         srcs)

    def test_override_keeps_deps_and_the_example(self):
        srcs = self.m.sources_for("lib/serial", "16F877A",
                                  toolchain="epic-cc")
        self.assertIn("lib/tick/src/epic_tick.c", srcs)
        self.assertIn("lib/serial/examples/example_serial.c", srcs)

    def test_xc8_path_unaffected_by_the_override(self):
        srcs = self.m.sources_for("lib/serial", "16F877A")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c",
                      srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c",
                         srcs)

    def test_modules_without_the_key_keep_the_family_slice(self):
        srcs = self.m.sources_for("lib/tick", "16F877A",
                                  toolchain="epic-cc")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c",
                      srcs)

    def test_empty_override_rejected(self):
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(HALT.replace(
                'epiccc_hal_sources_by_family.PIC16F87XA = [' +
                '"hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c", '
                '"common/src/core/epic_harness_target.c"]',
                'epiccc_hal_sources_by_family.PIC16F87XA = []')))

    def test_outside_hal_dir_rejected(self):
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(HALT.replace(
                '"hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c"',
                '"../escape.c"')))


EEXAMPLE = MINIMAL.replace(
    '[modules."lib/tick".example.PIC18Fxx5x]',
    '[modules."lib/tick".example.PIC16F87XA.epiccc]\n'
    'name    = "tick-blink-epiccc"\n'
    'sources = ["examples/example_tick_epiccc.c"]\n'
    'config  = { FOSC = "HS", WDTE = "OFF" }\n\n'
    '[modules."lib/tick".example.PIC18Fxx5x]',
)


class TestEpicccExample(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(EEXAMPLE))

    def test_epiccc_example_parsed(self):
        ex = self.m.example_for("lib/tick", "PIC16F87XA")
        self.assertEqual(ex.epiccc.name, "tick-blink-epiccc")
        self.assertEqual(ex.epiccc.sources,
                         ["examples/example_tick_epiccc.c"])
        self.assertEqual(ex.epiccc.config, {"FOSC": "HS", "WDTE": "OFF"})

    def test_epiccc_example_absent_is_none(self):
        ex = self.m.example_for("lib/tick", "PIC18Fxx5x")
        self.assertIsNone(ex.epiccc)

    def test_epic_cc_path_swaps_the_example_sources(self):
        srcs = self.m.sources_for("lib/tick", "16F877A", toolchain="epic-cc")
        self.assertIn("lib/tick/examples/example_tick_epiccc.c", srcs)
        self.assertNotIn("lib/tick/examples/example_tick.c", srcs)

    def test_xc8_path_keeps_the_target_example(self):
        srcs = self.m.sources_for("lib/tick", "16F877A")
        self.assertIn("lib/tick/examples/example_tick.c", srcs)
        self.assertNotIn("lib/tick/examples/example_tick_epiccc.c", srcs)

    def test_epiccc_example_requires_sources(self):
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(EEXAMPLE.replace(
                'sources = ["examples/example_tick_epiccc.c"]\n', "")))


SIMHALT = MINIMAL + """
[modules."lib/tick".epiccc_hal_sources_by_family]
PIC16F87XA = ["common/src/core/epic_harness_target.c"]

[modules."lib/tick".epiccc_sim_hal_sources_by_family]
PIC16F87XA = [
  "hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c",
  "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c",
]

[modules."lib/probe"]
summary    = "Tooling-test fixture."
sources    = ["src/probe.c"]
includes   = ["include"]
depends_on = []

[modules."lib/probe".supported]
PIC16F87XA = ["16F877A"]

[modules."lib/probe".epiccc_hal_sources_by_family]
PIC16F87XA = ["common/src/core/epic_harness_target.c"]

[modules."lib/probe".example.PIC16F87XA]
name    = "probe"
sources = ["examples/example_probe.c"]
config  = { FOSC = "HS", WDTE = "ON" }

[modules."lib/probe".example.PIC16F87XA.sim]
name        = "probe-sim"
harness_src = "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c"
config      = { FOSC = "HS", WDTE = "OFF" }
"""


class TestEpicccSimHalSources(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(SIMHALT))

    def test_sim_epiccc_uses_the_sim_slice_verbatim(self):
        srcs = self.m.sources_for("lib/tick", "16F877A", variant="sim",
                                  toolchain="epic-cc")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c",
                      srcs)
        self.assertIn("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c", srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c",
                         srcs)
        self.assertNotIn("common/src/core/epic_harness_target.c", srcs)

    def test_target_epiccc_ignores_the_sim_slice(self):
        srcs = self.m.sources_for("lib/tick", "16F877A",
                                  toolchain="epic-cc")
        self.assertIn("common/src/core/epic_harness_target.c", srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c", srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c",
                         srcs)

    def test_sim_epiccc_without_a_sim_slice_keeps_the_target_override(self):
        # Modules with only the target override (the combo tiers) keep
        # resolving it verbatim on sim builds: no swap runs under an
        # override, so the target harness stays linked there.
        srcs = self.m.sources_for("lib/probe", "16F877A", variant="sim",
                                  toolchain="epic-cc")
        self.assertIn("common/src/core/epic_harness_target.c", srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c", srcs)

    def test_xc8_sim_ignores_the_sim_slice(self):
        srcs = self.m.sources_for("lib/tick", "16F877A", variant="sim")
        self.assertIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_gpio.c",
                      srcs)
        self.assertIn("hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c", srcs)
        self.assertNotIn("hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c",
                         srcs)

    def test_unknown_family_rejected(self):
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(SIMHALT.replace(
                "epiccc_sim_hal_sources_by_family]\nPIC16F87XA",
                "epiccc_sim_hal_sources_by_family]\nPIC16F99Z")))

    def test_empty_sim_slice_rejected(self):
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(SIMHALT.replace(
                "PIC16F87XA = [\n"
                '  "hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c",\n'
                '  "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c",\n'
                "]",
                "PIC16F87XA = []", 1)))

    def test_outside_hal_dir_rejected(self):
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(SIMHALT.replace(
                '"hal/pic14/16f87xa/src/peripherals/pic16f87xa_usart.c"',
                '"../escape.c"')))

    def test_sim_slice_without_a_sim_variant_rejected(self):
        nosim = SIMHALT.replace(
            '[modules."lib/tick".example.PIC16F87XA.sim]', '[modules."lib/tick".example.PIC16F87XA.nosim]')
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(nosim))

    def test_sim_slice_missing_the_harness_rejected(self):
        noharness = SIMHALT.replace(
            '  "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c",\n', "")
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(noharness))

SIMGATES = MINIMAL + """
[modules."lib/tick".example.PIC16F87XA.sim_gates.second]
name        = "tick-second"
harness_src = "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c"
sources     = ["examples/example_second.c"]
config      = { FOSC = "HS", WDTE = "OFF" }
depends_on  = ["lib/serial"]

[modules."lib/tick".example.PIC16F87XA.sim_gates.second.variants.16F873A]
name        = "tick-second-873"
harness_src = "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c"
sources     = ["examples/example_second.c"]
config      = { FOSC = "HS", WDTE = "OFF" }
"""


class TestSimGates(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(SIMGATES))

    def test_gates_list_default_first_then_extras(self):
        gates = self.m.sim_gates_for("lib/tick", "PIC16F87XA", "16F877A")
        self.assertEqual([g.name for g in gates],
                         ["tick-blink-sim", "tick-second"])

    def test_extra_applies_its_own_mcu_variant(self):
        gates = self.m.sim_gates_for("lib/tick", "PIC16F87XA", "16F873A")
        self.assertEqual([g.name for g in gates],
                         ["tick-blink-sim", "tick-second-873"])

    def test_no_sim_means_no_gates(self):
        self.assertEqual(
            self.m.sim_gates_for("lib/tick", "PIC18Fxx5x", "18F4550"), [])

    def test_unknown_gate_raises(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.sim_gate_for("lib/tick", "PIC16F87XA", "16F877A",
                                "tick-third")

    def test_sources_follow_the_selected_gate(self):
        sim = self.m.sources_for("lib/tick", "16F877A", variant="sim")
        second = self.m.sources_for("lib/tick", "16F877A", variant="sim",
                                    sim_gate="tick-second")
        self.assertIn("lib/tick/examples/example_tick.c", sim)
        self.assertIn("lib/tick/examples/example_second.c", second)
        self.assertNotIn("lib/tick/examples/example_second.c", sim)

    def test_deps_follow_the_selected_gate(self):
        # Only the extra gate declares the serial dep, so only its
        # build links the serial library.
        sim = self.m.sources_for("lib/tick", "16F877A", variant="sim")
        second = self.m.sources_for("lib/tick", "16F877A", variant="sim",
                                    sim_gate="tick-second")
        self.assertNotIn("lib/serial/src/epic_serial.c", sim)
        self.assertIn("lib/serial/src/epic_serial.c", second)

    def test_base_mcu_override_replaces_the_whole_gate_set(self):
        override = SIMGATES + """
[modules."lib/tick".example.PIC16F87XA.sim.variants.16F873A]
name        = "tick-873"
harness_src = "hal/pic14/16f87xa/src/mdb/pic16_harness_mdb.c"
config      = { FOSC = "HS", WDTE = "OFF" }
"""
        m = epicmanifest.load(write(override))
        gates = m.sim_gates_for("lib/tick", "PIC16F87XA", "16F873A")
        self.assertEqual([g.name for g in gates], ["tick-873"])
        gates = m.sim_gates_for("lib/tick", "PIC16F87XA", "16F877A")
        self.assertEqual([g.name for g in gates],
                         ["tick-blink-sim", "tick-second"])

    def test_duplicate_gate_names_rejected(self):
        dup = SIMGATES.replace('name        = "tick-second"',
                               'name        = "tick-blink-sim"', 1)
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(dup))

    def test_unknown_sim_key_rejected(self):
        bad = SIMGATES.replace(
            '[modules."lib/tick".example.PIC16F87XA.sim_gates.second]',
            '[modules."lib/tick".example.PIC16F87XA.sim_gates.second]\nmode = "gpio"',
            1)
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(bad))

    def test_unknown_example_key_rejected(self):
        bad = MINIMAL.replace(
            '[modules."lib/tick".example.PIC16F87XA]\nname    = "tick-blink"',
            '[modules."lib/tick".example.PIC16F87XA]\nname    = "tick-blink"\nsim_typo  = {}',
            1)
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(bad))


class TestRootAnchoredPaths(unittest.TestCase):
    """Cross-module manifest paths hang off the repo root (epic-hal#313):
    moving a module directory renames its id with it."""

    def test_join_ignores_module_depth(self):
        for mod_dir in ("demos/menu", "a/b/c/menu"):
            self.assertEqual(
                epicmanifest.join_module_path(
                    mod_dir, "lib/lcd/src/epic_lcd_gpio4.c"),
                "lib/lcd/src/epic_lcd_gpio4.c")
            self.assertEqual(
                epicmanifest.join_module_path(mod_dir, "src/menu_demo_core.c"),
                f"{mod_dir}/src/menu_demo_core.c")

    def test_dotdot_source_rejected(self):
        bad = MINIMAL.replace(
            'sources    = ["src/epic_tick.c"]',
            'sources    = ["src/epic_tick.c", "../../lib/lcd/src/epic_lcd_gpio4.c"]')
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(bad))

    def test_dotdot_example_source_rejected(self):
        bad = MINIMAL.replace(
            'sources = ["examples/example_tick.c"]',
            'sources = ["../shared/example_tick.c"]', 1)
        with self.assertRaises(epicmanifest.ManifestError):
            epicmanifest.load(write(bad))

    def test_real_menu_entries_are_root_anchored(self):
        m = epicmanifest.load(epicmanifest.default_path())
        for module in ("demos/menu", "tests/epic-combo-lcd-tick"):
            srcs = m.sources_for(module, "18F4550")
            self.assertIn("lib/lcd/src/epic_lcd_gpio4.c", srcs)
            self.assertFalse(any(".." in s for s in srcs))

if __name__ == "__main__":
    unittest.main()
