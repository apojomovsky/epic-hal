# scripts/tests/test_epicurus_init.py
import pathlib, sys, unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
import epicmanifest, epicurus_init

MANIFEST = """
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
fosc_hz  = 20000000
includes = ["pic16f87xa-hal/include/target", "pic16f87xa-hal/include", "epic-common/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c"]

[modules.pic16f87xa-hal]
dir        = "pic16f87xa-hal"
sources    = []
includes   = []
depends_on = []
needs_hal  = true
[modules.pic16f87xa-hal.supported]
PIC16F87XA = ["16F873A", "16F877A"]
[modules.pic16f87xa-hal.example.PIC16F87XA]
name    = "blink"
sources = ["tests/example_blink.c"]
config  = { FOSC = "HS", WDTE = "ON", PWRTE = "ON", BOREN = "ON", LVP = "OFF", WRT = "OFF" }

[modules.epic-tick]
dir        = "epic-tick"
sources    = ["src/epic_tick.c"]
includes   = ["include"]
depends_on = []
[modules.epic-tick.supported]
PIC16F87XA = ["16F873A", "16F877A"]
[modules.epic-tick.example.PIC16F87XA]
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
16F873A = "UART only on 28-pin? no: demo exclusion reason"
"""

def load():
    import tempfile
    fd, p = tempfile.mkstemp(suffix=".toml")
    with open(fd, "w") as f: f.write(MANIFEST)
    return epicmanifest.load(pathlib.Path(p))

class TestResolveSelection(unittest.TestCase):
    def setUp(self): self.m = load()

    def test_hal_pseudo_module_is_dir_match(self):
        fam = self.m.families["PIC16F87XA"]
        self.assertEqual(epicurus_init.hal_pseudo_module(self.m, fam), "pic16f87xa-hal")

    def test_depends_on_expands_transitively(self):
        sel = epicurus_init.resolve_selection(self.m, "PIC16F87XA", "16F877A", ["epic-serial"])
        self.assertIn("epic-serial", sel)
        self.assertIn("epic-tick", sel)

    def test_unsupported_part_refuses_with_reason(self):
        with self.assertRaises(epicurus_init.SelectionError) as cm:
            epicurus_init.resolve_selection(self.m, "PIC16F87XA", "16F873A", ["epic-serial"])
        self.assertIn("epic-serial", str(cm.exception))
        self.assertIn("16F873A", str(cm.exception))

    def test_unknown_module_refuses(self):
        with self.assertRaises(epicurus_init.SelectionError):
            epicurus_init.resolve_selection(self.m, "PIC16F87XA", "16F877A", ["nope"])

    def test_unknown_family_refuses(self):
        with self.assertRaises(epicurus_init.SelectionError):
            epicurus_init.resolve_selection(self.m, "NOPE", "16F877A", ["epic-tick"])

class TestEmitMakefile(unittest.TestCase):
    def setUp(self): self.m = load()

    def test_makefile_has_required_fields(self):
        mk = epicurus_init.emit_makefile(
            self.m, "PIC16F87XA", "16F877A", ["serial"], "../..", "myapp")
        self.assertIn("EPICURUS_DIR := ../..", mk)
        self.assertIn("EPICURUS_MCU := 16F877A", mk)
        self.assertIn("EPICURUS_MODULES := serial", mk)
        self.assertIn("include $(EPICURUS_DIR)/epicurus.mk", mk)
        self.assertIn("DFOSC_HZ=20000000", mk)
        self.assertIn("myapp.hex: $(SRCS)", mk)

class TestEmitMainC(unittest.TestCase):
    def setUp(self): self.m = load()

    def test_has_xc_and_tick_and_gpio_headers(self):
        src = epicurus_init.emit_main_c(self.m, "PIC16F87XA", "16F877A", ["tick"])
        self.assertIn("#include <xc.h>", src)
        self.assertIn('#include "epic_tick.h"', src)
        self.assertIn('#include "peripherals/pic16f87xa_gpio.h"', src)

    def test_pragma_config_from_family_pseudo_module(self):
        src = epicurus_init.emit_main_c(self.m, "PIC16F87XA", "16F877A", ["tick"])
        self.assertIn("#pragma config FOSC = HS", src)
        self.assertIn("#pragma config WDTE = ON", src)

    def test_skeleton_uses_tick_and_gpio(self):
        src = epicurus_init.emit_main_c(self.m, "PIC16F87XA", "16F877A", ["tick"])
        self.assertIn("epic_tick_init(FOSC_HZ);", src)
        self.assertIn("EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);", src)

    def test_serial_skeleton(self):
        src = epicurus_init.emit_main_c(self.m, "PIC16F87XA", "16F877A", ["serial"])
        self.assertIn('#include "epic_serial.h"', src)
        self.assertIn("epic_serial_init(FOSC_HZ, 115200u);", src)
        self.assertNotIn("epic_tick", src)

    def test_bare_gpio_skeleton(self):
        src = epicurus_init.emit_main_c(self.m, "PIC16F87XA", "16F877A", [])
        self.assertIn('#include "peripherals/pic16f87xa_gpio.h"', src)
        self.assertIn("EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);", src)
        self.assertNotIn("epic_tick", src)
        self.assertNotIn("epic_serial", src)

    def test_bare_gpio_skeleton_fsm_only(self):
        src = epicurus_init.emit_main_c(self.m, "PIC16F87XA", "16F877A", ["fsm"])
        self.assertIn('#include "peripherals/pic16f87xa_gpio.h"', src)
        self.assertIn("EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);", src)
        self.assertNotIn("epic_tick", src)
        self.assertNotIn("epic_serial", src)

import xml.etree.ElementTree as ET

SAMPLE_XML = """<?xml version="1.0" encoding="UTF-8"?>
<configurationDescriptor version="65">
  <sourceRootList>
    <Elem>../../epic-tick/src</Elem>
    <Elem>../../pic16f87xa-hal/src/peripherals</Elem>
  </sourceRootList>
  <confs>
    <conf name="default" type="2">
      <toolsSet><targetDevice>PIC16F877A</targetDevice></toolsSet>
      <HI-TECH-COMP>
        <property key="define-macros" value="PIC16F877A;FOSC_HZ=20000000"/>
        <property key="extra-include-directories" value="../../pic16f87xa-hal/include/target"/>
      </HI-TECH-COMP>
    </conf>
  </confs>
</configurationDescriptor>"""

class TestPatchX(unittest.TestCase):
    def setUp(self): self.m = load()

    def test_source_dirs_hal_plus_modules(self):
        fam = self.m.families["PIC16F87XA"]
        sel = epicurus_init.resolve_selection(self.m, "PIC16F87XA", "16F877A", ["epic-serial"])
        d = epicurus_init.source_dirs(self.m, fam, "16F877A", sel)
        self.assertIn("pic16f87xa-hal/src/peripherals", d)
        self.assertIn("epic-serial/src", d)
        self.assertIn("epic-tick/src", d)

    def test_include_dirs_family_first(self):
        fam = self.m.families["PIC16F87XA"]
        sel = epicurus_init.resolve_selection(self.m, "PIC16F87XA", "16F877A", ["epic-serial"])
        d = epicurus_init.include_dirs(self.m, fam, "16F877A", sel)
        self.assertEqual(d[0], "pic16f87xa-hal/include/target")
        self.assertIn("epic-serial/include", d)
        self.assertIn("epic-tick/include", d)

    def test_patch_changes_device_macros_includes_sourceroots(self):
        fam = self.m.families["PIC16F87XA"]
        sel = epicurus_init.resolve_selection(self.m, "PIC16F87XA", "16F877A", ["epic-serial"])
        sd = epicurus_init.source_dirs(self.m, fam, "16F877A", sel)
        idd = epicurus_init.include_dirs(self.m, fam, "16F877A", sel)
        out = epicurus_init.patch_configurations_xml(SAMPLE_XML, fam, "16F873A", sd, idd)
        root = ET.fromstring(out)
        self.assertEqual(root.find(".//targetDevice").text, "PIC16F873A")
        dm = root.find(".//property[@key='define-macros']").get("value")
        self.assertEqual(dm, "PIC16F873A;FOSC_HZ=20000000")
        inc = root.find(".//property[@key='extra-include-directories']").get("value")
        self.assertTrue(inc.startswith("../../pic16f87xa-hal/include/target;"))
        elems = [e.text for e in root.findall(".//sourceRootList/Elem")]
        self.assertIn("../../epic-serial/src", elems)
        self.assertIn("../../epic-tick/src", elems)  # serial depends on tick

import os, tempfile, shutil

class TestInitProject(unittest.TestCase):
    def setUp(self):
        self.m = load()
        self.bundle = tempfile.mkdtemp()
        os.makedirs(os.path.join(self.bundle, "examples", "epicurus-demo.X", "nbproject"))
        with open(os.path.join(self.bundle, "examples", "epicurus-demo.X",
                               "nbproject", "configurations.xml"), "w") as f:
            f.write(SAMPLE_XML)
        with open(os.path.join(self.bundle, "examples", "epicurus-demo.X", "main.c"), "w") as f:
            f.write("/* old */\n")
        self.out = tempfile.mkdtemp()

    def test_writes_main_makefile_and_x(self):
        epicurus_init.init_project(
            self.m, "PIC16F87XA", "16F877A", ["epic-serial"],
            self.bundle, self.out, "myapp")
        self.assertTrue(os.path.isfile(os.path.join(self.out, "main.c")))
        self.assertTrue(os.path.isfile(os.path.join(self.out, "Makefile")))
        cfg = os.path.join(self.out, "myapp.X", "nbproject", "configurations.xml")
        self.assertTrue(os.path.isfile(cfg))
        root = ET.parse(cfg).getroot()
        self.assertEqual(root.find(".//targetDevice").text, "PIC16F877A")

    def test_refuses_existing_project(self):
        epicurus_init.init_project(
            self.m, "PIC16F87XA", "16F877A", ["epic-serial"],
            self.bundle, self.out, "myapp")
        with self.assertRaises(FileExistsError):
            epicurus_init.init_project(
                self.m, "PIC16F87XA", "16F877A", ["epic-serial"],
                self.bundle, self.out, "myapp")

    def test_writes_x_from_repo_layout(self):
        # Repo-checkout layout: reference project is examples/epicurus-demo-<slug>.X
        # (Resolution A fallback), not examples/epicurus-demo.X.
        bundle = pathlib.Path(tempfile.mkdtemp())
        xdir = bundle / "examples" / "epicurus-demo-pic16f87xa.X"
        (xdir / "nbproject").mkdir(parents=True)
        (xdir / "nbproject" / "configurations.xml").write_text(SAMPLE_XML)
        (xdir / "main.c").write_text("/* old */\n")
        out = tempfile.mkdtemp()
        epicurus_init.init_project(
            self.m, "PIC16F87XA", "16F877A", ["epic-serial"],
            str(bundle), out, "myapp")
        cfg = pathlib.Path(out) / "myapp.X" / "nbproject" / "configurations.xml"
        self.assertTrue(cfg.is_file())
        root = ET.parse(cfg).getroot()
        self.assertEqual(root.find(".//targetDevice").text, "PIC16F877A")

    def test_makefile_epicurus_dir_is_relative_to_bundle(self):
        # Scaffolding one level below the bundle root (the documented
        # layout) must yield EPICURUS_DIR := .. so the consumer Makefile
        # reaches the bundle's epicurus.mk.
        out = pathlib.Path(self.bundle) / "scaffold"
        epicurus_init.init_project(
            self.m, "PIC16F87XA", "16F877A", ["epic-serial"],
            self.bundle, str(out), "myapp")
        mk = (out / "Makefile").read_text()
        self.assertIn("EPICURUS_DIR := ..\n", mk)
        self.assertIn("include $(EPICURUS_DIR)/epicurus.mk", mk)
        self.assertTrue((out / "myapp.X" / "nbproject" / "configurations.xml").is_file())


class TestBundlePresence(unittest.TestCase):
    """Task 6: a release bundle must be self-sufficient for `epicurus init`:
    it carries the CLI and all four Python modules the CLI imports, plus the
    manifest at the path _find_manifest expects.
    """
    def _make_bundle(self, out_dir):
        import make_bundle
        argv = ["make_bundle", "--family", "PIC16F87XA", "--version",
                "ci-test", "--out-dir", str(out_dir), "--no-tarball"]
        saved = sys.argv
        sys.argv = argv
        try:
            make_bundle.main()
        finally:
            sys.argv = saved

    def test_bundle_contains_cli_modules_and_manifest(self):
        repo = pathlib.Path(__file__).resolve().parents[2]
        out_dir = pathlib.Path(tempfile.mkdtemp(dir=repo))
        self._make_bundle(out_dir)
        self.addCleanup(shutil.rmtree, out_dir, ignore_errors=True)
        root = out_dir / "epicurus-pic16f87xa-ci-test"
        self.assertTrue(root.is_dir(), f"bundle root not created at {root}")
        # 1. CLI, executable, no extension.
        cli = root / "epicurus"
        self.assertTrue(cli.is_file(), "missing bundle-root epicurus CLI")
        self.assertTrue(os.access(cli, os.X_OK), "epicurus is not executable")
        # 2-4. The three helper modules the CLI imports from its own dir.
        for mod in ("epicurus_init.py", "epicmanifest.py", "bundlegen.py"):
            self.assertTrue((root / mod).is_file(), f"missing bundle-root {mod}")
        # 5. The manifest at the path _find_manifest resolves from the root.
        manifest = root / "epic-common" / "manifest" / "modules.toml"
        self.assertTrue(manifest.is_file(), "missing epic-common/manifest/modules.toml")

if __name__ == "__main__": unittest.main()
