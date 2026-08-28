//! Runs an epic-cc-produced HEX under epic-cc's own ISA simulator and
//! asserts a register-bit toggle: the public HAL-4 gate that replaced the
//! mdb/MPLAB SIM half of the epiccc-gate job. The timer model is
//! deliberate. crates/sim simulates the CPU only, so the gates raise the
//! firmware's interrupt source the way the peripheral would: latch the
//! flag through its enable bit, take the vector when GIE allows. The
//! firmware's own ISR does the rest, exactly as on silicon, so what a
//! tick gate proves is the interrupt path, not an oscillator.

use device::{Core, Device};
use pic14_sim::{parse_hex, parse_hex_pic18, Pic14, Pic18};
use std::process::ExitCode;

/// SFR addresses the gates need, per core. The PIC14 row is shared by the
/// 877A and 887 (DS39582C/DS41286A); the PIC18 row is the PIC18F4550's
/// (DS39632E). LATB exists on the 4550 only; the PIC16 families toggle
/// PORTx directly.
#[derive(Clone, Copy)]
struct Regs {
    portb: u16,
    latb: Option<u16>,
    intcon: u16,
    pir1: u16,
    pie1: u16,
}

const PIC14_REGS: Regs = Regs {
    portb: 0x06,
    latb: None,
    intcon: 0x0B,
    pir1: 0x0C,
    pie1: 0x8C,
};
// Exercised once the 4550 smoke lands (epic-hal#60, blocked on
// epic-cc#125/#126).
const PIC18_REGS: Regs = Regs {
    portb: 0xF81,
    latb: Some(0xF8A),
    intcon: 0xFF2,
    pir1: 0xF9E,
    pie1: 0xF9D,
};
const GIE: u8 = 1 << 7;

/// An interrupt source to inject: latch `flag` (an `SFR:BIT` pair) every
/// `every` steps, gated by `enable`.
struct Irq {
    every: usize,
    flag: (u16, u8),
    enable: (u16, u8),
}

enum Sim {
    Pic14(Pic14, Regs),
    Pic18(Pic18, Regs),
}

impl Sim {
    /// Build from the resolved device, not just its core: the simulator
    /// must use the device's real bank geometry. The two families here
    /// happen to share `ram_banks` today, which is why the blink gates
    /// passed when this hard-coded `PIC16F877A`; wiring the geometry
    /// through `with_device` removes the silent-mis-simulation hazard a
    /// future part would hit (#115 insistence on not shipping silent
    /// geometry bugs).
    fn build(dev: &'static Device, hex: &str) -> Result<Sim, String> {
        match dev.core {
            Core::Pic14 => {
                let prog = parse_hex(hex);
                Ok(Sim::Pic14(Pic14::with_device(dev, prog), PIC14_REGS))
            }
            Core::Pic18 => {
                let prog = parse_hex_pic18(hex);
                Ok(Sim::Pic18(Pic18::new(prog), PIC18_REGS))
            }
            Core::Pic14e => Err("enhanced mid-range cores have no sim gate yet".into()),
        }
    }

    fn regs(&self) -> Regs {
        match self {
            Sim::Pic14(_, r) | Sim::Pic18(_, r) => *r,
        }
    }

    fn run(&mut self, steps: usize) -> usize {
        match self {
            Sim::Pic14(p, _) => p.run(steps),
            Sim::Pic18(p, _) => p.run(steps),
        }
    }

    fn halted(&self) -> bool {
        match self {
            Sim::Pic14(p, _) => p.halted(),
            Sim::Pic18(p, _) => p.halted(),
        }
    }

    /// The peripheral's interrupt raise, gated the way hardware gates it:
    /// the enable bit must be set for the flag to latch, and the vector is
    /// only taken when GIE is set. A flag latched while GIE is masked
    /// stays set, so the ISR services it on a later injection.
    ///
    /// Direct physical RAM via `ram()/ram_mut()[phys]`, not the sim's
    /// banked instruction addressing: this register table is physical
    /// (`PIC14_REGS` 0x06/0x0B/0x0C/0x8C, `PIC18_REGS` 0xF81/...), matching
    /// `crates/device` and the DS pages the gate cites. The banked path
    /// is the CPU's `BANKSEL + read_f` for firmware instructions; the
    /// gate checks the memory that path writes into, which is also the
    /// place the banking bug #173 was filed against.
    fn inject(&mut self, irq: &Irq) {
        let regs = self.regs();
        let (faddr, fbit) = irq.flag;
        let (eaddr, ebit) = irq.enable;
        let enabled = match self {
            Sim::Pic14(p, _) => p.ram()[eaddr as usize] >> ebit & 1 != 0,
            Sim::Pic18(p, _) => p.ram()[eaddr as usize] >> ebit & 1 != 0,
        };
        if !enabled {
            if std::env::var_os("SIM_RUNNER_DEBUG").is_some() {
                let ic = self.reg_byte(regs.intcon);
                let p1 = self.reg_byte(regs.pir1);
                eprintln!(
                    "inject: skipped (enable off) INTCON={ic:02X} PIR1={p1:02X} PIE1={:02X}",
                    self.reg_byte(regs.pie1)
                );
            }
            return;
        }
        let gie_on = match self {
            Sim::Pic14(p, _) => p.ram()[regs.intcon as usize] & GIE != 0,
            Sim::Pic18(p, _) => p.ram()[regs.intcon as usize] & GIE != 0,
        };
        match self {
            Sim::Pic14(p, _) => {
                p.ram_mut()[faddr as usize] |= 1 << fbit;
                if gie_on {
                    p.fire_interrupt();
                }
            }
            Sim::Pic18(p, _) => {
                p.ram_mut()[faddr as usize] |= 1 << fbit;
                if gie_on {
                    p.fire_interrupt();
                }
            }
        }
    }

    /// Read a watched bit from physical RAM (same reasoning as `inject`).
    fn watch_bit(&self, addr: u16, bit: u8) -> u8 {
        match self {
            Sim::Pic14(p, _) => (p.ram()[addr as usize] >> bit) & 1,
            Sim::Pic18(p, _) => (p.ram()[addr as usize] >> bit) & 1,
        }
    }

    fn reg_byte(&self, addr: u16) -> u8 {
        match self {
            Sim::Pic14(p, _) => p.ram()[addr as usize],
            Sim::Pic18(p, _) => p.ram()[addr as usize],
        }
    }
}

struct Args {
    hex: String,
    device: String,
    watch: String,
    samples: usize,
    steps: usize,
    irq_every: Option<usize>,
    irq_flag: Option<String>,
    irq_enable: Option<String>,
    trace: Vec<String>,
}

/// Resolve a `NAME:BIT` register pair against the device's table.
fn parse_reg(spec: &str, regs: &Regs) -> Result<(u16, u8), String> {
    let (name, bit) = spec
        .rsplit_once(':')
        .ok_or_else(|| format!("{spec}: wants SFR:BIT"))?;
    let bit: u8 = bit.parse().map_err(|e| format!("{spec}: bad bit: {e}"))?;
    if bit > 7 {
        return Err(format!("{spec}: bit must be 0 to 7"));
    }
    let addr = match name.to_ascii_uppercase().as_str() {
        "PORTB" => regs.portb,
        "LATB" => regs
            .latb
            .ok_or_else(|| format!("{spec}: LATB does not exist on this core"))?,
        "INTCON" => regs.intcon,
        "PIR1" => regs.pir1,
        "PIE1" => regs.pie1,
        other => return Err(format!("{spec}: unknown register {other}")),
    };
    Ok((addr, bit))
}

fn parse_args() -> Result<Args, String> {
    let mut raw = std::env::args().skip(1);
    let (mut hex, mut device, mut watch) = (None, None, None);
    let (mut samples, mut steps) = (12usize, 200_000usize);
    let (mut irq_every, mut irq_flag, mut irq_enable, mut trace) =
        (None, None, None, Vec::new());
    while let Some(arg) = raw.next() {
        let mut val = |name: &str| -> Result<String, String> {
            raw.next().ok_or_else(|| format!("{name} needs a value"))
        };
        match arg.as_str() {
            "--hex" => hex = Some(val("--hex")?),
            "--device" => device = Some(val("--device")?),
            "--watch" => watch = Some(val("--watch")?),
            "--samples" => samples = val("--samples")?.parse().map_err(|e| format!("--samples: {e}"))?,
            "--steps" => steps = val("--steps")?.parse().map_err(|e| format!("--steps: {e}"))?,
            "--irq-every" => {
                irq_every = Some(val("--irq-every")?.parse().map_err(|e| format!("--irq-every: {e}"))?)
            }
            "--irq-flag" => irq_flag = Some(val("--irq-flag")?),
            "--irq-enable" => irq_enable = Some(val("--irq-enable")?),
            "--trace" => trace = val("--trace")?.split(',').map(str::to_string).collect(),
            other => return Err(format!("unknown argument {other}")),
        }
    }
    let hex = hex.ok_or("--hex is required")?;
    let device = device.ok_or("--device is required")?;
    let watch = watch.ok_or("--watch is required (SFR:BIT)")?;
    if samples == 0 || steps == 0 {
        return Err("--samples and --steps must be >0".into());
    }
    if let Some(v) = irq_every {
        if v == 0 {
            return Err("--irq-every must be >0".into());
        }
    }
    if irq_every.is_some() != (irq_flag.is_some() && irq_enable.is_some()) {
        return Err("--irq-every needs --irq-flag and --irq-enable (and vice versa)".into());
    }
    Ok(Args {
        hex,
        device,
        watch,
        samples,
        steps,
        irq_every,
        irq_flag,
        irq_enable,
        trace,
    })
}

fn main() -> ExitCode {
    let args = match parse_args() {
        Ok(a) => a,
        Err(e) => {
            eprintln!("sim-runner: {e}");
            return ExitCode::FAILURE;
        }
    };
    let dev = match device::resolve(&args.device) {
        Some(d) => d,
        None => {
            eprintln!("sim-runner: unknown device {}", args.device);
            return ExitCode::FAILURE;
        }
    };
    let hex = match std::fs::read_to_string(&args.hex) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("sim-runner: {}: {e}", args.hex);
            return ExitCode::FAILURE;
        }
    };
    let mut sim = match Sim::build(dev, &hex) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("sim-runner: {e}");
            return ExitCode::FAILURE;
        }
    };
    let regs = sim.regs();
    let watch = match parse_reg(&args.watch, &regs) {
        Ok(w) => w,
        Err(e) => {
            eprintln!("sim-runner: --watch {}: {e}", args.watch);
            return ExitCode::FAILURE;
        }
    };
    let irq = match (args.irq_every, args.irq_flag.as_deref(), args.irq_enable.as_deref()) {
        (Some(every), Some(flag), Some(enable)) => {
            let flag = match parse_reg(flag, &regs) {
                Ok(f) => f,
                Err(e) => {
                    eprintln!("sim-runner: --irq-flag {flag}: {e}");
                    return ExitCode::FAILURE;
                }
            };
            let enable = match parse_reg(enable, &regs) {
                Ok(e) => e,
                Err(e) => {
                    eprintln!("sim-runner: --irq-enable {enable}: {e}");
                    return ExitCode::FAILURE;
                }
            };
            Some(Irq {
                every,
                flag,
                enable,
            })
        }
        _ => None,
    };
    let mut traced: Vec<(String, u16)> = Vec::new();
    for name in &args.trace {
        match parse_reg(&format!("{name}:0"), &regs) {
            Ok((addr, _)) => traced.push((name.to_ascii_uppercase(), addr)),
            Err(e) => {
                eprintln!("sim-runner: --trace {name}: {e}");
                return ExitCode::FAILURE;
            }
        }
    }

    let mut values = Vec::with_capacity(args.samples);
    for s in 0..args.samples {
        let mut done = 0usize;
        let mut since_irq = 0usize;
        while done < args.steps {
            if sim.halted() {
                eprintln!(
                    "sim-runner: firmware halted (SLEEP) at sample {s}/{}, {} of {} steps done",
                    args.samples, done, args.steps
                );
                return ExitCode::FAILURE;
            }
            let mut chunk = args.steps - done;
            if let Some(irq) = &irq {
                chunk = chunk.min(irq.every - since_irq);
            }
            let ran = sim.run(chunk);
            done += ran;
            if let Some(irq) = &irq {
                since_irq += ran;
                if since_irq >= irq.every {
                    since_irq = 0;
                    sim.inject(irq);
                }
            }
        }
        let bit = sim.watch_bit(watch.0, watch.1);
        values.push(bit);
        let trace = traced
            .iter()
            .map(|(n, a)| format!("{n}={:02X}", sim.reg_byte(*a)))
            .collect::<Vec<_>>()
            .join(" ");
        if trace.is_empty() {
            println!("sample {}/{}: {}={}", s + 1, args.samples, args.watch, bit);
        } else {
            println!("sample {}/{}: {}={} {}", s + 1, args.samples, args.watch, bit, trace);
        }
    }

    let toggled = values.windows(2).any(|w| w[0] != w[1]);
    if toggled {
        println!(
            "PASS: {} toggled across {} samples of {} steps ({}, device {})",
            args.watch, args.samples, args.steps, args.hex, args.device
        );
        ExitCode::SUCCESS
    } else {
        eprintln!(
            "FAIL: {} never changed across {} samples of {} steps ({}, device {})",
            args.watch, args.samples, args.steps, args.hex, args.device
        );
        ExitCode::FAILURE
    }
}
