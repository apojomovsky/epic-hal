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

if __name__ == "__main__": unittest.main()
