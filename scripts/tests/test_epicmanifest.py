"""Unit tests for scripts/epicmanifest.py."""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import epicmanifest  # noqa: E402

MINIMAL = """
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
fosc_hz  = 20000000
includes = ["pic16f87xa-hal/include", "epic-common/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", "epic-common/src/core/epic_harness_target.c"]
harness_src = "epic-common/src/core/epic_harness_target.c"
epiccc_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", "epic-common/src/core/epic_harness_target.c"]

[[families.PIC16F87XA.conditional_sources]]
path     = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
variants = ["16F877A"]

[families.PIC18Fxx5x]
hal_dir  = "pic18fxx5x-hal"
variants = ["18F4550"]
dfp      = "Microchip.PIC18Fxxxx_DFP"
fosc_hz  = 48000000
includes = ["pic18fxx5x-hal/include", "epic-common/include"]
hal_sources = ["pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c"]

[modules.epic-tick]
dir        = "epic-tick"
sources    = ["src/epic_tick.c"]
includes   = ["include"]
depends_on = []

[modules.epic-tick.supported]
PIC16F87XA = ["16F873A", "16F877A"]
PIC18Fxx5x = ["18F4550"]

[modules.epic-tick.example.PIC16F87XA]
name    = "tick-blink"
sources = ["examples/example_tick.c"]
config  = { FOSC = "HS", WDTE = "ON" }

[modules.epic-tick.example.PIC16F87XA.sim]
name        = "tick-blink-sim"
harness_src = "pic16f87xa-hal/src/mdb/pic16_harness_mdb.c"
config      = { FOSC = "HS", WDTE = "OFF" }

[modules.epic-tick.example.PIC18Fxx5x]
name    = "tick-blink"
sources = ["examples/example_tick.c"]
config  = { FOSC = "HS" }

[modules.epic-serial]
dir        = "epic-serial"
sources    = ["src/epic_serial.c"]
includes   = ["include"]
depends_on = ["epic-tick"]

[modules.epic-serial.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-serial.excluded]
"16F873A" = "RAM: 32-byte rx buffer does not fit"

[modules.epic-math]
dir        = "epic-math"
sources    = ["src/common/epic_math_sqrt.c", "src/common/epic_math_numeric.c"]
includes   = ["include"]
depends_on = []
needs_hal  = false

[modules.epic-math.sources_by_family]
PIC16F87XA = ["src/pic16/epic_math_mul.c", "src/pic16/epic_math_scratch.c"]
PIC18Fxx5x = ["src/pic18/epic_math_mul.c"]

[modules.epic-math.supported]
PIC16F87XA = ["16F873A", "16F877A"]
PIC18Fxx5x = ["18F4550"]

[modules.epic-math.example.PIC16F87XA]
name    = "math-smoke"
sources = ["tests/target_smoke16.c"]
hal     = true
config  = { FOSC = "HS" }
# The example logs over serial; the library does not, so the dep is
# scoped to the example (the per-MCU variant below drops it).
depends_on = ["epic-serial"]

[modules.epic-math.example.PIC16F87XA.variants.16F873A]
name    = "math-smoke-873"
sources = ["tests/target_smoke16.c"]
hal     = true

[modules.epic-math.example.PIC18Fxx5x]
name    = "math-selftest"
sources = ["tests/target_selftest.c"]
hal     = true
depends_on = ["epic-serial"]

[modules.epic-adcfilter]
dir        = "epic-adcfilter"
sources    = ["src/epic_adcfilter.c"]
includes   = ["include"]
depends_on = []
needs_hal  = false

[modules.epic-adcfilter.supported]
PIC16F87XA = ["16F873A", "16F877A"]

[modules.epic-adcfilter.example.PIC16F87XA]
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
        self.assertEqual(fam.hal_dir, "pic16f87xa-hal")
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
        mod = self.m.modules["epic-serial"]
        self.assertEqual(mod.name, "epic-serial")
        self.assertEqual(mod.dir, "epic-serial")
        self.assertEqual(mod.depends_on, ["epic-tick"])
        self.assertEqual(mod.supported["PIC16F87XA"], ["16F877A"])
        self.assertEqual(mod.excluded["16F873A"], "RAM: 32-byte rx buffer does not fit")

    def test_needs_hal_defaults_to_true(self):
        self.assertTrue(self.m.modules["epic-tick"].needs_hal)

    def test_needs_hal_false_parsed(self):
        self.assertFalse(self.m.modules["epic-math"].needs_hal)

    def test_sources_by_family_parsed(self):
        sbf = self.m.modules["epic-math"].sources_by_family
        self.assertEqual(sbf["PIC16F87XA"], ["src/pic16/epic_math_mul.c", "src/pic16/epic_math_scratch.c"])
        self.assertEqual(sbf["PIC18Fxx5x"], ["src/pic18/epic_math_mul.c"])

    def test_examples_are_keyed_by_family(self):
        ex = self.m.modules["epic-tick"].examples
        self.assertEqual(ex["PIC16F87XA"].name, "tick-blink")
        self.assertEqual(ex["PIC18Fxx5x"].name, "tick-blink")

    def test_example_config_is_family_scoped(self):
        ex = self.m.modules["epic-tick"].examples["PIC16F87XA"]
        self.assertEqual(ex.config, {"FOSC": "HS", "WDTE": "ON"})

    def test_example_hal_defaults_to_module_needs_hal(self):
        # epic-adcfilter: needs_hal=false, no hal override -> hal False
        self.assertFalse(self.m.modules["epic-adcfilter"].examples["PIC16F87XA"].hal)

    def test_example_hal_can_override_module_needs_hal(self):
        # epic-math: needs_hal=false, example hal=true -> hal True
        self.assertTrue(self.m.modules["epic-math"].examples["PIC16F87XA"].hal)

    def test_example_config_is_empty_when_absent(self):
        self.assertEqual(self.m.modules["epic-adcfilter"].examples["PIC16F87XA"].config, {})

    def test_example_depends_on_parsed(self):
        self.assertEqual(self.m.modules["epic-math"].examples["PIC16F87XA"].depends_on,
                         ["epic-serial"])
        # Variants carry their own depends_on (default empty when absent).
        variant = self.m.modules["epic-math"].examples["PIC16F87XA"].variants["16F873A"]
        self.assertEqual(variant.depends_on, [])

    def test_example_is_none_when_no_entry_for_that_family(self):
        # epic-serial has no example table at all
        self.assertIsNone(self.m.modules["epic-serial"].examples.get("PIC16F87XA"))

    def test_excluded_defaults_to_empty(self):
        self.assertEqual(self.m.modules["epic-tick"].excluded, {})


class TestValidation(unittest.TestCase):
    def test_unknown_dependency_is_rejected(self):
        bad = MINIMAL.replace('depends_on = ["epic-tick"]', 'depends_on = ["epic-nope"]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("epic-nope", str(cm.exception))

    def test_unknown_family_in_supported_is_rejected(self):
        bad = MINIMAL.replace("[modules.epic-tick.supported]\nPIC16F87XA",
                              "[modules.epic-tick.supported]\nPIC99XXXX")
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
        bad = MINIMAL.replace('[modules.epic-tick]\ndir        = "epic-tick"\nsources    = ["src/epic_tick.c"]\nincludes   = ["include"]\ndepends_on = []',
                              '[modules.epic-tick]\ndir        = "epic-tick"\nsources    = ["src/epic_tick.c"]\nincludes   = ["include"]\ndepends_on = ["epic-serial"]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("cycle", str(cm.exception).lower())

    def test_unknown_family_in_sources_by_family_is_rejected(self):
        bad = MINIMAL.replace('[modules.epic-math.sources_by_family]\nPIC16F87XA',
                              '[modules.epic-math.sources_by_family]\nPIC99XXXX')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("PIC99XXXX", str(cm.exception))

    def test_unknown_family_in_example_is_rejected(self):
        # Rename both the base example and its per-MCU variant table so
        # the family key is consistently unknown.
        bad = MINIMAL.replace('[modules.epic-math.example.PIC16F87XA.variants.16F873A]',
                              '[modules.epic-math.example.PIC99XXXX.variants.16F873A]')
        bad = bad.replace('[modules.epic-math.example.PIC16F87XA]',
                          '[modules.epic-math.example.PIC99XXXX]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("PIC99XXXX", str(cm.exception))

    def test_unknown_example_dependency_is_rejected(self):
        bad = MINIMAL.replace('depends_on = ["epic-serial"]\n\n[modules.epic-math.example.PIC18Fxx5x]',
                              'depends_on = ["epic-serial"]\n\n[modules.epic-math.example.PIC18Fxx5x]')
        bad = bad.replace('depends_on = ["epic-serial"]',
                          'depends_on = ["epic-nope"]', 1)
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("epic-nope", str(cm.exception))

    def test_missing_family_fosc_hz_is_rejected(self):
        bad = MINIMAL.replace('fosc_hz  = 20000000\n', '', 1)
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("fosc_hz", str(cm.exception))


class TestResolution(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(MINIMAL))

    def test_resolve_deps_puts_dependencies_first(self):
        self.assertEqual(self.m.resolve_deps("epic-serial"), ["epic-tick", "epic-serial"])

    def test_resolve_deps_of_a_leaf_is_just_itself(self):
        self.assertEqual(self.m.resolve_deps("epic-tick"), ["epic-tick"])

    def test_resolve_deps_rejects_unknown_module(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.resolve_deps("epic-nope")

    def test_family_of_maps_a_part_to_its_family(self):
        self.assertEqual(self.m.family_of("16F877A").name, "PIC16F87XA")
        self.assertEqual(self.m.family_of("18F4550").name, "PIC18Fxx5x")

    def test_family_of_rejects_unknown_part(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.family_of("16F999X")

    def test_is_supported(self):
        self.assertTrue(self.m.is_supported("epic-serial", "PIC16F87XA", "16F877A"))
        self.assertFalse(self.m.is_supported("epic-serial", "PIC16F87XA", "16F873A"))

    def test_exclusion_reason(self):
        self.assertEqual(
            self.m.exclusion_reason("epic-serial", "16F873A"),
            "RAM: 32-byte rx buffer does not fit",
        )
        self.assertIsNone(self.m.exclusion_reason("epic-serial", "16F877A"))

    def test_example_for_returns_the_family_example(self):
        self.assertEqual(self.m.example_for("epic-math", "PIC16F87XA").name, "math-smoke")
        self.assertEqual(self.m.example_for("epic-math", "PIC18Fxx5x").name, "math-selftest")

    def test_example_for_returns_none_when_absent(self):
        self.assertIsNone(self.m.example_for("epic-serial", "PIC16F87XA"))

    def test_uses_hal_falls_back_to_needs_hal_without_an_example(self):
        # epic-serial has no example; uses_hal falls back to needs_hal (true)
        self.assertTrue(self.m.uses_hal("epic-serial", "16F877A"))

    def test_uses_hal_follows_the_example_override_when_present(self):
        # epic-math: needs_hal=false but example hal=true
        self.assertTrue(self.m.uses_hal("epic-math", "16F877A"))
        # epic-adcfilter: needs_hal=false, no override
        self.assertFalse(self.m.uses_hal("epic-adcfilter", "16F877A"))

    def test_hal_true_build_includes_family_hal_sources(self):
        srcs = self.m.sources_for("epic-tick", "16F877A")
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", srcs)

    def test_needs_hal_false_with_no_override_omits_family_hal_sources(self):
        srcs = self.m.sources_for("epic-adcfilter", "16F877A")
        self.assertNotIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", srcs)

    def test_needs_hal_false_but_example_hal_true_includes_family_hal_sources(self):
        srcs = self.m.sources_for("epic-math", "16F877A")
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", srcs)

    def test_needs_hal_false_omits_family_includes(self):
        incs = self.m.includes_for("epic-adcfilter", "16F877A")
        self.assertNotIn("pic16f87xa-hal/include", incs)
        self.assertNotIn("epic-common/include", incs)
        self.assertIn("epic-adcfilter/include", incs)

    def test_conditional_source_included_only_on_matching_variant(self):
        psp = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
        self.assertIn(psp, self.m.sources_for("epic-tick", "16F877A"))
        self.assertNotIn(psp, self.m.sources_for("epic-tick", "16F873A"))

    def test_conditional_source_omitted_when_hal_not_used(self):
        psp = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
        self.assertNotIn(psp, self.m.sources_for("epic-adcfilter", "16F877A"))

    def test_sources_include_module_and_example(self):
        srcs = self.m.sources_for("epic-tick", "16F877A")
        self.assertIn("epic-tick/src/epic_tick.c", srcs)
        self.assertIn("epic-tick/examples/example_tick.c", srcs)

    def test_sources_pull_in_dependency_sources(self):
        srcs = self.m.sources_for("epic-serial", "16F877A")
        self.assertIn("epic-tick/src/epic_tick.c", srcs)
        self.assertIn("epic-serial/src/epic_serial.c", srcs)

    def test_sources_pull_in_example_dependency_sources(self):
        # epic-math's example depends_on epic-serial (the library does
        # not), so the serial library joins the math example's build.
        srcs = self.m.sources_for("epic-math", "16F877A")
        self.assertIn("epic-serial/src/epic_serial.c", srcs)

    def test_includes_pull_in_example_dependency_includes(self):
        incs = self.m.includes_for("epic-math", "16F877A")
        self.assertIn("epic-serial/include", incs)

    def test_example_dependency_respects_per_mcu_variant_override(self):
        # The 16F873A variant of epic-math's example drops the serial
        # dep (it is excluded on that part for RAM), so the small-part
        # probe build must not compile epic-serial.
        srcs = self.m.sources_for("epic-math", "16F873A")
        self.assertNotIn("epic-serial/src/epic_serial.c", srcs)
        self.assertIn("epic-math/tests/target_smoke16.c", srcs)

    def test_example_dependency_sources_are_deduplicated(self):
        srcs = self.m.sources_for("epic-math", "16F877A")
        self.assertEqual(len(srcs), len(set(srcs)))

    def test_sources_only_include_the_requested_modules_example(self):
        srcs = self.m.sources_for("epic-serial", "16F877A")
        self.assertNotIn("epic-tick/examples/example_tick.c", srcs)

    def test_sources_by_family_contributes_only_the_matching_family(self):
        srcs16 = self.m.sources_for("epic-math", "16F877A")
        self.assertIn("epic-math/src/pic16/epic_math_mul.c", srcs16)
        self.assertIn("epic-math/src/pic16/epic_math_scratch.c", srcs16)
        self.assertNotIn("epic-math/src/pic18/epic_math_mul.c", srcs16)
        self.assertIn("epic-math/src/common/epic_math_sqrt.c", srcs16)
        srcs18 = self.m.sources_for("epic-math", "18F4550")
        self.assertIn("epic-math/src/pic18/epic_math_mul.c", srcs18)
        self.assertNotIn("epic-math/src/pic16/epic_math_mul.c", srcs18)
        self.assertIn("epic-math/src/common/epic_math_sqrt.c", srcs18)

    def test_the_right_example_is_used_per_family(self):
        self.assertIn("epic-math/tests/target_smoke16.c",
                      self.m.sources_for("epic-math", "16F877A"))
        self.assertIn("epic-math/tests/target_selftest.c",
                      self.m.sources_for("epic-math", "18F4550"))
        self.assertNotIn("epic-math/tests/target_selftest.c",
                         self.m.sources_for("epic-math", "16F877A"))

    def test_sources_have_no_duplicates(self):
        srcs = self.m.sources_for("epic-math", "16F877A")
        self.assertEqual(len(srcs), len(set(srcs)))

    def test_includes_preserve_family_order_then_modules(self):
        incs = self.m.includes_for("epic-serial", "16F877A")
        self.assertEqual(incs[0], "pic16f87xa-hal/include")
        self.assertEqual(incs[1], "epic-common/include")
        self.assertIn("epic-tick/include", incs)
        self.assertIn("epic-serial/include", incs)

    def test_sim_variant_for_returns_the_sim_data(self):
        sim = self.m.sim_variant_for("epic-tick", "PIC16F87XA")
        self.assertEqual(sim.name, "tick-blink-sim")
        self.assertEqual(sim.harness_src,
                         "pic16f87xa-hal/src/mdb/pic16_harness_mdb.c")

    def test_sim_variant_for_returns_none_without_one(self):
        self.assertIsNone(self.m.sim_variant_for("epic-tick", "PIC18Fxx5x"))

    def test_sources_for_sim_variant_swaps_the_harness_source(self):
        target = self.m.sources_for("epic-tick", "16F877A", variant="target")
        sim = self.m.sources_for("epic-tick", "16F877A", variant="sim")
        self.assertIn("epic-common/src/core/epic_harness_target.c", target)
        self.assertNotIn("pic16f87xa-hal/src/mdb/pic16_harness_mdb.c", target)
        self.assertIn("pic16f87xa-hal/src/mdb/pic16_harness_mdb.c", sim)
        self.assertNotIn("epic-common/src/core/epic_harness_target.c", sim)

    def test_sources_for_sim_variant_keeps_the_harness_source_position(self):
        sim = self.m.sources_for("epic-tick", "16F877A", variant="sim")
        target = self.m.sources_for("epic-tick", "16F877A", variant="target")
        target_pos = target.index("epic-common/src/core/epic_harness_target.c")
        sim_pos = sim.index("pic16f87xa-hal/src/mdb/pic16_harness_mdb.c")
        self.assertEqual(target_pos, sim_pos)

    def test_sources_for_sim_variant_reuses_example_sources_when_no_override(self):
        sim = self.m.sources_for("epic-tick", "16F877A", variant="sim")
        self.assertIn("epic-tick/examples/example_tick.c", sim)

    def test_sources_for_sim_variant_raises_without_one(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.sources_for("epic-tick", "18F4550", variant="sim")

    def test_epiccc_sources_used_for_epic_cc_toolchain(self):
        srcs = self.m.sources_for("epic-tick", "16F877A", toolchain="epic-cc")
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", srcs)
        self.assertNotIn("pic16f87xa-hal/src/epiccc/pic16f87xa_gpio_epiccc.c", srcs)

    def test_epiccc_sources_do_not_splice_conditional_sources(self):
        # Conditional sources are XC8 psect-order machinery; the epic-cc
        # path is a single whole-program invocation with no link order.
        psp = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
        srcs = self.m.sources_for("epic-tick", "16F877A", toolchain="epic-cc")
        self.assertNotIn(psp, srcs)

    def test_epiccc_sources_default_to_empty(self):
        fam = self.m.families["PIC18Fxx5x"]
        self.assertEqual(fam.epiccc_sources, [])

    def test_epic_cc_without_epiccc_sources_raises(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.sources_for("epic-tick", "18F4550", toolchain="epic-cc")

    def test_epiccc_sources_keep_the_example(self):
        srcs = self.m.sources_for("epic-tick", "16F877A", toolchain="epic-cc")
        self.assertIn("epic-tick/examples/example_tick.c", srcs)

    def test_xc8_path_is_unchanged_with_epiccc_sources_present(self):
        # The epiccc_sources key must not leak into the XC8 resolution.
        srcs = self.m.sources_for("epic-tick", "16F877A")
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", srcs)
        self.assertNotIn("pic16f87xa-hal/src/epiccc/pic16f87xa_gpio_epiccc.c", srcs)


if __name__ == "__main__":
    unittest.main()
