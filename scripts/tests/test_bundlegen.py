"""Unit tests for scripts/bundlegen.py."""
import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import bundlegen  # noqa: E402
import epicmanifest  # noqa: E402

MANIFEST = """
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
fosc_hz  = 20000000
includes = ["pic16f87xa-hal/include/target", "pic16f87xa-hal/include",
            "epic-common/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c"]

[families.PIC18Fxx5x]
hal_dir  = "pic18fxx5x-hal"
variants = ["18F4550"]
dfp      = "Microchip.PIC18Fxxxx_DFP"
fosc_hz  = 48000000
includes = ["pic18fxx5x-hal/include/target", "epic-common/include"]
hal_sources = ["pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c"]

[families.PIC16F193X]
hal_dir  = "pic16f193x-hal"
variants = ["16F1937"]
dfp      = "Microchip.PIC12-16F1xxx_DFP"
fosc_hz  = 32000000
includes = ["pic16f193x-hal/include/target", "epic-common/include"]
hal_sources = ["pic16f193x-hal/src/peripherals/pic16f193x_gpio.c"]

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
config  = { FOSC = "HS" }

[modules.epic-tick.example.PIC18Fxx5x]
name    = "tick-blink"
sources = ["examples/example_tick.c"]

[modules.epic-serial]
dir        = "epic-serial"
sources    = ["src/epic_serial.c"]
includes   = ["include"]
depends_on = ["epic-tick"]

[modules.epic-serial.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-serial.excluded]
"16F873A" = "RAM: 32-byte g_rx_buf does not fit"

[modules.epic-serial.example.PIC16F87XA]
name    = "serial-echo"
sources = ["examples/example_serial.c"]

[modules.epic-usb]
dir        = "epic-usb"
sources    = ["src/epic_usb.c"]
includes   = ["include"]
depends_on = []

[modules.epic-usb.supported]
PIC18Fxx5x = ["18F4550"]

[modules.epic-usb.example.PIC18Fxx5x]
name    = "usb-cdc"
sources = ["examples/example_usb.c"]

# A module standing in for the family's own bare-HAL build, dir equal
# to the family's hal_dir, the same shape as epic-pic16f193x-firmware
# in the real manifest: must not appear in a bundle's module list, it
# is CI coverage plumbing, not a consumer-facing library.
[modules.epic-pic16f193x-firmware]
dir        = "pic16f193x-hal"
sources    = []
includes   = []
depends_on = []

[modules.epic-pic16f193x-firmware.supported]
PIC16F193X = ["16F1937"]

[modules.epic-pic16f193x-firmware.example.PIC16F193X]
name    = "firmware"
sources = ["tests/example_blink.c"]
"""


def load():
    tmp = tempfile.NamedTemporaryFile("w", suffix=".toml", delete=False)
    tmp.write(MANIFEST)
    tmp.close()
    return epicmanifest.load(pathlib.Path(tmp.name))


class TestModuleSelection(unittest.TestCase):
    def setUp(self):
        self.m = load()

    def test_includes_modules_supported_on_the_family(self):
        self.assertEqual(
            bundlegen.modules_for_family(self.m, "PIC16F87XA"),
            ["epic-serial", "epic-tick"],
        )

    def test_excludes_modules_with_no_supported_part(self):
        self.assertNotIn(
            "epic-usb", bundlegen.modules_for_family(self.m, "PIC16F87XA")
        )

    def test_other_family_gets_its_own_module_set(self):
        self.assertEqual(
            bundlegen.modules_for_family(self.m, "PIC18Fxx5x"),
            ["epic-tick", "epic-usb"],
        )

    def test_excludes_the_family_hal_wrapper_pseudo_module(self):
        # epic-pic16f193x-firmware's dir equals its family's hal_dir: it
        # is CI-coverage plumbing (plan 1's fallback for a family with no
        # real modules yet), not a library a bundle consumer would ask
        # for by name. PIC16F193X is correctly a HAL-only bundle.
        self.assertEqual(bundlegen.modules_for_family(self.m, "PIC16F193X"), [])

    def test_unknown_family_raises(self):
        with self.assertRaises(bundlegen.BundleError):
            bundlegen.modules_for_family(self.m, "PIC99XXXX")


class TestFileSelection(unittest.TestCase):
    def setUp(self):
        self.files = bundlegen.files_for_family(load(), "PIC16F87XA")

    def test_includes_family_hal_sources(self):
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", self.files)

    def test_includes_module_sources(self):
        self.assertIn("epic-serial/src/epic_serial.c", self.files)
        self.assertIn("epic-tick/src/epic_tick.c", self.files)

    def test_includes_example_sources(self):
        self.assertIn("epic-tick/examples/example_tick.c", self.files)

    def test_excludes_other_families_hal(self):
        self.assertFalse(any(f.startswith("pic18fxx5x-hal/") for f in self.files))

    def test_excludes_modules_not_supported_here(self):
        self.assertFalse(any(f.startswith("epic-usb/") for f in self.files))

    def test_is_sorted_and_deduplicated(self):
        self.assertEqual(self.files, sorted(set(self.files)))


class TestEpicurusMk(unittest.TestCase):
    def setUp(self):
        self.mk = bundlegen.emit_epicurus_mk(load(), "PIC16F87XA", "v0.1.0")

    def test_declares_the_family_and_version(self):
        self.assertIn("PIC16F87XA", self.mk)
        self.assertIn("v0.1.0", self.mk)

    def test_maps_short_module_names_to_full_ones(self):
        self.assertIn("EPICURUS_MODULE_serial := epic-serial", self.mk)
        self.assertIn("EPICURUS_MODULE_tick := epic-tick", self.mk)

    def test_flattens_dependencies_at_generation_time(self):
        self.assertIn(
            "EPICURUS_RESOLVED_epic-serial := epic-tick epic-serial", self.mk
        )

    def test_lists_supported_parts_per_module(self):
        self.assertIn(
            "EPICURUS_SUPPORTED_epic-serial := 16F877A", self.mk
        )
        self.assertIn(
            "EPICURUS_SUPPORTED_epic-tick := 16F873A 16F877A", self.mk
        )

    def test_carries_the_exclusion_reason(self):
        self.assertIn(
            "EPICURUS_WHYNOT_epic-serial_16F873A := "
            "RAM: 32-byte g_rx_buf does not fit",
            self.mk,
        )

    def test_hal_sources_are_prefixed_with_the_bundle_dir(self):
        self.assertIn(
            "$(EPICURUS_DIR)/pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c",
            self.mk,
        )

    def test_module_sources_are_prefixed_with_the_bundle_dir(self):
        self.assertIn("$(EPICURUS_DIR)/epic-serial/src/epic_serial.c", self.mk)

    def test_includes_are_prefixed_and_ordered(self):
        self.assertIn(
            "-I$(EPICURUS_DIR)/pic16f87xa-hal/include/target "
            "-I$(EPICURUS_DIR)/pic16f87xa-hal/include",
            self.mk,
        )

    def test_errors_on_an_unset_mcu(self):
        self.assertIn("EPICURUS_MCU is not set", self.mk)

    def test_errors_on_an_unsupported_pair(self):
        self.assertIn("$(error", self.mk)
        self.assertIn("is not supported on", self.mk)

    def test_defines_the_part_macro_and_dfp(self):
        self.assertIn("-DPIC$(EPICURUS_MCU)", self.mk)
        self.assertIn("Microchip.PIC16Fxxx_DFP", self.mk)

    def test_excludes_the_family_hal_wrapper_pseudo_module(self):
        # Same rule as modules_for_family: PIC16F193X's epicurus.mk must
        # not map a short name for epic-pic16f193x-firmware.
        mk193x = bundlegen.emit_epicurus_mk(load(), "PIC16F193X", "v0.1.0")
        self.assertNotIn("EPICURUS_MODULE_pic16f193x-firmware", mk193x)


class TestSourcesJson(unittest.TestCase):
    def setUp(self):
        self.doc = json.loads(
            bundlegen.emit_sources_json(load(), "PIC16F87XA", "v0.1.0")
        )

    def test_carries_version_family_and_dfp(self):
        self.assertEqual(self.doc["version"], "v0.1.0")
        self.assertEqual(self.doc["family"], "PIC16F87XA")
        self.assertEqual(self.doc["dfp"], "Microchip.PIC16Fxxx_DFP")

    def test_lists_hal_and_conditional_sources(self):
        self.assertIn(
            "pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c",
            self.doc["hal_sources"],
        )
        self.assertIsInstance(self.doc["conditional_sources"], list)

    def test_module_entry_is_complete(self):
        entry = self.doc["modules"]["epic-serial"]
        self.assertEqual(entry["resolved"], ["epic-tick", "epic-serial"])
        self.assertEqual(entry["sources"], ["epic-serial/src/epic_serial.c"])
        self.assertEqual(entry["includes"], ["epic-serial/include"])
        self.assertEqual(entry["supported"], ["16F877A"])
        self.assertEqual(
            entry["excluded"]["16F873A"], "RAM: 32-byte g_rx_buf does not fit"
        )

    def test_module_entry_carries_its_family_example(self):
        entry = self.doc["modules"]["epic-tick"]
        self.assertEqual(entry["example"]["name"], "tick-blink")
        self.assertEqual(
            entry["example"]["sources"], ["epic-tick/examples/example_tick.c"]
        )
        self.assertEqual(entry["example"]["config"], {"FOSC": "HS"})

    def test_omits_modules_from_other_families(self):
        self.assertNotIn("epic-usb", self.doc["modules"])

    def test_omits_the_family_hal_wrapper_pseudo_module(self):
        doc193x = json.loads(
            bundlegen.emit_sources_json(load(), "PIC16F193X", "v0.1.0")
        )
        self.assertNotIn("epic-pic16f193x-firmware", doc193x["modules"])

    def test_paths_are_bundle_relative(self):
        for path in self.doc["hal_sources"]:
            self.assertFalse(path.startswith("/"))
            self.assertNotIn("..", path)


class TestSupportMd(unittest.TestCase):
    def setUp(self):
        self.md = bundlegen.emit_support_md(load(), "PIC16F87XA", "v0.1.0")

    def test_has_a_row_per_module(self):
        self.assertIn("epic-serial", self.md)
        self.assertIn("epic-tick", self.md)

    def test_marks_supported_and_unsupported_parts(self):
        self.assertIn("yes", self.md)
        self.assertIn("no", self.md)

    def test_lists_every_exclusion_reason(self):
        self.assertIn("RAM: 32-byte g_rx_buf does not fit", self.md)

    def test_names_the_variants_as_columns(self):
        self.assertIn("16F873A", self.md)
        self.assertIn("16F877A", self.md)

    def test_has_no_em_dash(self):
        self.assertNotIn(chr(0x2014), self.md)  # em-dash, repo convention


class TestQuickstart(unittest.TestCase):
    def setUp(self):
        self.md = bundlegen.emit_quickstart_md(load(), "PIC16F87XA", "v0.1.0")

    def test_shows_a_complete_consumer_makefile(self):
        self.assertIn("EPICURUS_DIR :=", self.md)
        self.assertIn("EPICURUS_MCU :=", self.md)
        self.assertIn("EPICURUS_MODULES :=", self.md)
        self.assertIn("include $(EPICURUS_DIR)/epicurus.mk", self.md)

    def test_names_a_real_part_from_this_family(self):
        self.assertIn("16F877A", self.md)

    def test_names_a_real_module_from_this_family(self):
        self.assertIn("tick", self.md)

    def test_mentions_the_dfp_flag(self):
        self.assertIn("-mdfp", self.md)

    def test_has_no_em_dash(self):
        self.assertNotIn(chr(0x2014), self.md)  # em-dash, repo convention

    def test_raises_for_a_hal_only_family(self):
        # PIC16F193X has no consumer-facing module (its sole entry is the
        # excluded HAL-wrapper pseudo-module), so a quickstart cannot
        # name a working EPICURUS_MODULES value.
        with self.assertRaises(bundlegen.BundleError):
            bundlegen.emit_quickstart_md(load(), "PIC16F193X", "v0.1.0")


class TestMplabxMd(unittest.TestCase):
    def setUp(self):
        self.md = bundlegen.emit_mplabx_md(load(), "PIC16F87XA", "v0.1.0")

    def test_lists_the_source_folders_to_add(self):
        self.assertIn("pic16f87xa-hal/src", self.md)
        self.assertIn("epic-serial/src", self.md)

    def test_lists_the_include_paths_in_order(self):
        self.assertIn("pic16f87xa-hal/include/target", self.md)
        idx_target = self.md.index("pic16f87xa-hal/include/target")
        idx_plain = self.md.index("epic-common/include")
        self.assertLess(idx_target, idx_plain)

    def test_names_the_dfp_pack(self):
        self.assertIn("Microchip.PIC16Fxxx_DFP", self.md)

    def test_points_at_the_reference_project(self):
        self.assertIn("examples/epicurus-demo.X", self.md)

    def test_has_no_em_dash(self):
        self.assertNotIn(chr(0x2014), self.md)  # em-dash, repo convention


class TestReferenceProjectPath(unittest.TestCase):
    """The repo-side .X directory name for a family."""

    def test_maps_family_to_its_project_dir(self):
        self.assertEqual(
            bundlegen.reference_project_dir(load(), "PIC16F87XA"),
            "examples/epicurus-demo-pic16f87xa.X",
        )
        self.assertEqual(
            bundlegen.reference_project_dir(load(), "PIC18Fxx5x"),
            "examples/epicurus-demo-pic18fxx5x.X",
        )

    def test_unknown_family_raises(self):
        with self.assertRaises(bundlegen.BundleError):
            bundlegen.reference_project_dir(load(), "PIC99XXXX")


if __name__ == "__main__":
    unittest.main()
