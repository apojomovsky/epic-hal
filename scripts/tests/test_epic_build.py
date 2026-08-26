"""Unit tests for scripts/epic_build.py."""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import epic_build  # noqa: E402
import epicmanifest  # noqa: E402

MANIFEST = """
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
fosc_hz  = 20000000
includes = ["pic16f87xa-hal/include/target", "pic16f87xa-hal/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", "epic-common/src/core/epic_harness_target.c"]
harness_src = "epic-common/src/core/epic_harness_target.c"
epiccc_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", "epic-common/src/core/epic_harness_target.c"]

[[families.PIC16F87XA.conditional_sources]]
path     = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
variants = ["16F877A"]

[modules.epic-tick]
dir        = "epic-tick"
sources    = ["src/epic_tick.c"]
includes   = ["include"]
depends_on = []

[modules.epic-tick.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-tick.excluded]
"16F873A" = "RAM: does not fit"

[modules.epic-tick.example.PIC16F87XA]
name    = "tick-blink"
sources = ["examples/example_tick.c"]
config  = { FOSC = "HS", WDTE = "ON" }

[modules.epic-tick.example.PIC16F87XA.sim]
name        = "tick-blink-sim"
harness_src = "pic16f87xa-hal/src/mdb/pic16_harness_mdb.c"
config      = { FOSC = "HS", WDTE = "OFF" }

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
# Example-level dep: epic-tick is excluded on 16F873A (see the tick
# fixture's excluded), so a build of this example there must fail the
# support check rather than compile excluded code.
depends_on = ["epic-tick"]

[modules.epic-math]
dir        = "epic-math"
sources    = ["src/common/epic_math_numeric.c"]
includes   = ["include"]
depends_on = []
needs_hal  = false

[modules.epic-math.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-math.sources_by_family]
PIC16F87XA = [
  "src/pic16/epic_math_addsub.c",
  "src/pic16/epic_math_mul.c",
]

[modules.epic-pid]
dir        = "epic-pid"
sources    = ["src/pid.c"]
includes   = ["include"]
depends_on = ["epic-math"]
needs_hal  = false

[modules.epic-pid.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-pid.example.PIC16F87XA]
name    = "pid-sizecheck"
sources = ["mcu/target_sizecheck.c"]

[modules.epic-encoder]
dir        = "epic-encoder"
sources    = ["src/encoder.c"]
includes   = ["include"]
depends_on = ["epic-tick"]
needs_hal  = false

[modules.epic-encoder.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-encoder.example.PIC16F87XA]
name    = "encoder-sizecheck"
sources = ["mcu/target_sizecheck.c"]
# Example-level dep: the real example logs over epic-serial; the
# epic-cc driver path must drop it.
depends_on = ["epic-serial"]

[modules.epic-serial]
dir        = "epic-serial"
sources    = ["src/epic_serial.c"]
includes   = ["include"]
depends_on = []

[modules.epic-serial.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-serial.example.PIC16F87XA]
name    = "serial-sizecheck"
sources = ["mcu/target_sizecheck.c"]
"""


def load():
    tmp = tempfile.NamedTemporaryFile("w", suffix=".toml", delete=False)
    tmp.write(MANIFEST)
    tmp.close()
    return epicmanifest.load(pathlib.Path(tmp.name))


class TestConfigSource(unittest.TestCase):
    def test_emits_one_pragma_per_entry(self):
        out = epic_build.emit_config_source(load(), "epic-tick", "16F877A")
        self.assertIn("#include <xc.h>", out)
        self.assertIn("#pragma config FOSC = HS", out)
        self.assertIn("#pragma config WDTE = ON", out)

    def test_marks_the_file_generated(self):
        out = epic_build.emit_config_source(load(), "epic-tick", "16F877A")
        self.assertIn("Auto-generated", out)

    def test_returns_none_when_the_example_has_no_config_table(self):
        # epic-adcfilter's example carries no config table, matching its
        # Makefile, which never compiled a config translation unit.
        out = epic_build.emit_config_source(load(), "epic-adcfilter", "16F877A")
        self.assertIsNone(out)

    def test_sim_variant_uses_its_own_config_override(self):
        out = epic_build.emit_config_source(load(), "epic-tick", "16F877A", variant="sim")
        self.assertIn("#pragma config WDTE = OFF", out)
        self.assertNotIn("#pragma config WDTE = ON", out)


class TestDisplayPath(unittest.TestCase):
    """`--build-dir` may point outside the checkout, so the "wrote ..." line
    cannot always be repo-relative."""

    def test_path_inside_the_repo_is_repo_relative(self):
        p = epic_build.REPO / "build" / "16F877A" / "build.sh"
        self.assertEqual(
            str(epic_build._display_path(p)), "build/16F877A/build.sh"
        )

    def test_path_outside_the_repo_stays_absolute(self):
        with tempfile.TemporaryDirectory() as td:
            p = pathlib.Path(td).resolve() / "16F877A" / "build.sh"
            self.assertEqual(epic_build._display_path(p), p)

    def test_sibling_of_the_repo_does_not_raise(self):
        p = (epic_build.REPO / ".." / "elsewhere" / "build.sh").resolve()
        self.assertEqual(epic_build._display_path(p), p)


class TestBuildScript(unittest.TestCase):
    def script(self, module="epic-tick", mcu="16F877A", dfp_dir="/opt/dfp", fosc_hz=None,
              variant="target"):
        return epic_build.emit_build_script(
            load(), module, mcu,
            build_dir="build", dfp_dir=dfp_dir, fosc_hz=fosc_hz, variant=variant,
        )

    def test_starts_with_a_posix_shebang_and_errexit(self):
        lines = self.script().splitlines()
        self.assertEqual(lines[0], "#!/bin/sh")
        self.assertIn("set -e", lines)

    def test_compiles_every_source_to_p1(self):
        s = self.script()
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", s)
        self.assertIn("epic-tick/src/epic_tick.c", s)
        self.assertIn("epic-tick/examples/example_tick.c", s)
        self.assertIn("build/16F877A/epic_tick.p1", s)

    def test_example_dependency_sources_are_in_the_build(self):
        # epic-adcfilter's example depends_on epic-tick, so the tick
        # library joins the example's build even though the module has
        # no module-level dep.
        s = epic_build.emit_build_script(
            load(), "epic-adcfilter", "16F877A",
            build_dir="build", dfp_dir="/opt/dfp")
        self.assertIn("epic-tick/src/epic_tick.c", s)
        self.assertIn("epic-adcfilter/mcu/target_sizecheck.c", s)

    def test_example_dep_excluded_on_mcu_raises(self):
        # epic-tick is excluded on 16F873A; epic-adcfilter's example
        # depends on it, so that build must fail loudly instead of
        # compiling excluded code.
        with self.assertRaises(epic_build.UnsupportedError) as cm:
            epic_build.emit_build_script(
                load(), "epic-adcfilter", "16F873A",
                build_dir="build", dfp_dir="/opt/dfp")
        self.assertIn("epic-tick", str(cm.exception))
        self.assertIn("16F873A", str(cm.exception))

    def test_includes_conditional_source_only_on_matching_variant(self):
        self.assertIn("pic16f87xa_psp.c", self.script())

    def test_flag_order_matches_the_makefiles(self):
        s = self.script()
        # Mirrors epic_build.emit_build_script's flag list exactly: the
        # triaged XC8 -Wno suppressions (set by ed2ac5d) sit between
        # -Wextra and -DPIC<part>, before the -I includes.
        self.assertIn(
            "-mdfp=/opt/dfp -mcpu=16f877a -O2 -std=c99 -Wall -Wextra "
            "-Wno-520 -Wno-2053 -Wno-759 -Wno-1516 -Wno-1311 -Wno-1262 "
            "-Wno-1510 -Wno-2098 -Wno-1498 -Wno-unused-function "
            "-Wno-unused-variable -Wno-unused-parameter "
            "-Wno-sign-conversion -Wno-implicit-int-conversion "
            "-DPIC16F877A",
            s,
        )

    def test_omits_dfp_flag_when_dfp_dir_is_empty(self):
        s = self.script(dfp_dir="")
        self.assertNotIn("-mdfp=", s)
        self.assertIn("-mcpu=16f877a", s)

    def test_include_flags_preserve_manifest_order(self):
        s = self.script()
        self.assertIn(
            "-Ipic16f87xa-hal/include/target -Ipic16f87xa-hal/include -Iepic-tick/include",
            s,
        )

    def test_fosc_hz_defaults_to_the_family_value_when_omitted(self):
        self.assertIn("-DFOSC_HZ=20000000", self.script(fosc_hz=None))

    def test_fosc_hz_argument_overrides_the_family_default(self):
        self.assertIn("-DFOSC_HZ=4000000", self.script(fosc_hz=4000000))
        self.assertNotIn("-DFOSC_HZ=20000000", self.script(fosc_hz=4000000))

    def test_links_to_the_example_named_hex_with_ginhx32(self):
        s = self.script()
        self.assertIn("build/16F877A-tick-blink.hex", s)
        self.assertIn("-ginhx32", s)

    def test_unsupported_pair_raises_with_the_reason(self):
        with self.assertRaises(epic_build.UnsupportedError) as cm:
            self.script(mcu="16F873A")
        self.assertIn("RAM: does not fit", str(cm.exception))

    def test_config_less_example_script_has_no_config_reference(self):
        # Amendment 2's correction: no CONFIG_SRC in the old Makefile means
        # no config translation unit and no config object in the link.
        s = self.script(module="epic-adcfilter")
        self.assertNotIn("config_", s)

    def test_config_bearing_example_compiles_and_links_the_config_object(self):
        s = self.script()
        self.assertIn("config_16F877A.c", s)
        self.assertIn("config_16F877A.p1", s)

    def test_sim_variant_swaps_the_harness_source_and_hex_name(self):
        target = self.script(variant="target")
        sim = self.script(variant="sim")
        self.assertIn("epic-common/src/core/epic_harness_target.c", target)
        self.assertNotIn("pic16f87xa-hal/src/mdb/pic16_harness_mdb.c", target)
        self.assertIn("pic16f87xa-hal/src/mdb/pic16_harness_mdb.c", sim)
        self.assertNotIn("epic-common/src/core/epic_harness_target.c", sim)
        self.assertIn("build/16F877A-tick-blink-sim.hex", sim)

    def test_sim_variant_without_one_raises(self):
        with self.assertRaises(epic_build.UnsupportedError) as cm:
            self.script(module="epic-adcfilter", mcu="16F877A", variant="sim")
        self.assertIn("no sim variant", str(cm.exception))


class TestReport(unittest.TestCase):
    LOG = """
Memory Summary:
    Program space        used   102Ch (  4140) of  2000h words   ( 50.5%)
    Data space           used    5Bh (    91) of   170h bytes   ( 24.7%)
"""

    def test_parses_flash_and_ram(self):
        usage = epic_build.parse_memory_summary(self.LOG)
        self.assertEqual(usage["flash_bytes"], 4140)
        self.assertEqual(usage["ram_bytes"], 91)

    def test_returns_none_when_absent(self):
        self.assertIsNone(epic_build.parse_memory_summary("no summary here"))


if __name__ == "__main__":
    unittest.main()


class TestEpicCcToolchain(unittest.TestCase):
    """The epic-cc backend passes the manifest mcu through untranslated.

    epic-cc resolves `16F877A`, `16f877a` and `p16f877a` to the same device, so
    a name table on this side would be a second place to update per part.
    """

    def script(self, mcu="16F877A"):
        return epic_build.emit_build_script(
            load(), "epic-tick", mcu,
            build_dir="build", dfp_dir="", toolchain="epic-cc",
        )

    def test_passes_the_manifest_mcu_to_target(self):
        self.assertIn("--target 16F877A", self.script())

    def test_does_not_translate_the_mcu_name(self):
        s = self.script()
        self.assertNotIn("--target p16f877a", s)
        self.assertNotIn("--device", s)

    def test_no_per_part_name_table_remains(self):
        self.assertFalse(hasattr(epic_build, "_device_for_epic_cc"))

    def test_unsupported_mcu_still_raises_with_the_reason(self):
        with self.assertRaises(epic_build.UnsupportedError) as ctx:
            self.script(mcu="16F873A")
        self.assertIn("RAM: does not fit", str(ctx.exception))

    def pid_script(self):
        return epic_build.emit_build_script(
            load(), "epic-pid", "16F877A",
            build_dir="build", dfp_dir="", toolchain="epic-cc",
        )

    def encoder_script(self):
        return epic_build.emit_build_script(
            load(), "epic-encoder", "16F877A",
            build_dir="build", dfp_dir="", toolchain="epic-cc",
        )

    def test_pid_links_the_host_math_mul_not_the_pic16_asm_backend(self):
        # pid.c's real epic_pid_update calls epic_math_mul_s16; the pic16
        # asm backend needs XC8's xc.h, so the epic-cc path links the
        # portable host C implementation instead (the independent oracle).
        s = self.pid_script()
        self.assertIn("epic-math/src/host/epic_math_mul.c", s)
        self.assertNotIn("src/pic16/epic_math_mul.c", s)
        self.assertIn("epic-pid/mcu/target_sizecheck_epiccc.c", s)
        self.assertNotIn("epic-pid/examples/", s)

    def test_pid_epiccc_drops_the_pic16_math_sources(self):
        # The math SOURCES that hit the pic16 asm dialect are dropped for
        # epic-cc; only the host mul joins (tested above).
        s = self.pid_script()
        self.assertNotIn("src/pic16/epic_math_addsub.c", s)

    def test_encoder_epiccc_uses_the_driver_and_drops_tick_and_serial(self):
        # The driver supplies epic_tick_get/epic_tick_elapsed_since
        # (epic-tick is HAL-3c, #86); the module's tick/serial example
        # deps must not join the link (their include dirs may remain as
        # harmless search paths).
        s = self.encoder_script()
        self.assertIn("epic-encoder/mcu/target_sizecheck_epiccc.c", s)
        self.assertNotIn("epic-tick/src", s)
        self.assertNotIn("epic-serial/src", s)
