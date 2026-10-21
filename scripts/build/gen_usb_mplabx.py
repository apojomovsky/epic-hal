#!/usr/bin/env python3
"""Generate lib/usb's MPLAB X bring-up project.

The project is a real MPLAB X project (a .X directory with an
nbproject/ tree), openable in the IDE: it names the device, the
configuration words, the include dirs and the source files, so a board
can be programmed and debugged from the IDE.

Its file list is the manifest's own answer for lib/usb on 18F4550, so
the IDE project and the CI-gated build cannot drift: the manifest
decides which sources, include dirs and config words the project
carries, and this script renders them into MPLAB X's project format.
scripts/audit/mplabx-audit.py checks the committed tree against the
manifest, so a change to the module that forgets the project fails CI.

The committed tree deliberately carries no nbproject/Makefile-default.mk:
MPLAB X generates that file when the project is opened in the IDE, and
it is a generated artifact of the IDE, not a description of the
project. The command-line build of the same sources is the
manifest-driven one (`make xc8-build MODULE=lib/usb MCU=18F4550`),
which is what CI gates.

Usage: gen_usb_mplabx.py [--check]

--check fails without writing when the committed tree differs from
what the manifest produces.
"""
from __future__ import annotations

import argparse
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import epicmanifest  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[2]
PROJECT = REPO / "lib" / "usb" / "mcu" / "pic18fxx5x-usb-mplabx" / \
    "pic18fxx5x-usb.X"
MODULE = "lib/usb"
FAMILY = "PIC18Fxx5x"
MCU = "18F4550"
PROJECT_NAME = "pic18fxx5x-usb"

# The family reference project: the static nbproject files (the XC8
# toolchain locations and the NetBeans makefile plumbing) are the same
# for every project of this family, so they are taken from there
# rather than re-invented.
REFERENCE = REPO / "examples" / "epic-hal-demo-pic18fxx5x.X"

# The files the IDE needs and that do not depend on the module's file
# list.
STATIC_FILES = (
    "Makefile",
    "nbproject/Makefile-impl.mk",
    "nbproject/Makefile-variables.mk",
    "nbproject/Makefile-local-default.mk",
    "nbproject/Makefile-genesis.properties",
    "nbproject/private/configurations.xml",
)


def _source_roots(sources: list[str]) -> list[str]:
    """The source roots the project's files live in, deduplicated."""
    roots: list[str] = []
    for src in sources:
        parent = "/".join(src.split("/")[:-1])
        if parent and parent not in roots:
            roots.append(parent)
    return roots


def _configurations_xml(sources: list[str], includes: list[str],
                        defines: list[str]) -> str:
    """The project's description: what the IDE builds and how.

    Four fields carry the selection, exactly as the reference projects
    do: the device, the macros, the include dirs and the source roots.
    Everything else (the XC8 flag set, the linker options) is the
    family default, unchanged.
    """
    inc_value = ";".join(includes)
    def_value = ";".join(defines)
    items = "\n".join(
        f'        <itemPath>{s}</itemPath>' for s in sources)
    roots = "\n".join(
        f'        <Elem>{d}</Elem>' for d in _source_roots(sources))
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<configurationDescriptor version="65">
  <logicalFolder name="root" displayName="root" projectFiles="true">
    <logicalFolder name="HeaderFiles"
                   displayName="Header Files"
                   projectFiles="true">
    </logicalFolder>
    <logicalFolder name="ExternalFiles"
                   displayName="Important Files"
                   projectFiles="true">
      <itemPath>Makefile</itemPath>
    </logicalFolder>
    <logicalFolder name="LinkerScript"
                   displayName="Linker Files"
                   projectFiles="true">
    </logicalFolder>
    <logicalFolder name="SourceFiles"
                   displayName="Source Files"
                   projectFiles="true">
{items}
    </logicalFolder>
  </logicalFolder>
  <sourceRootList>
{roots}
  </sourceRootList>
  <projectmakefile>Makefile</projectmakefile>
  <confs>
    <conf name="default" type="2">
      <toolsSet>
        <developmentServer>localhost</developmentServer>
        <targetDevice>PIC{MCU}</targetDevice>
        <targetHeader></targetHeader>
        <targetPluginBoard></targetPluginBoard>
        <platformTool>noID</platformTool>
        <languageToolchain>XC8</languageToolchain>
        <languageToolchainVersion>4.00</languageToolchainVersion>
        <platform>2</platform>
      </toolsSet>
      <packs>
        <pack name="PIC18Fxxxx_DFP" vendor="Microchip" version="1.7.171"/>
      </packs>
      <ScriptingSettings>
      </ScriptingSettings>
      <compileType>
        <linkerTool>
          <linkerLibItems>
          </linkerLibItems>
        </linkerTool>
        <archiverTool>
        </archiverTool>
        <loading>
          <useAlternateLoadableFile>false</useAlternateLoadableFile>
          <parseOnProdLoad>false</parseOnProdLoad>
          <alternateLoadableFile></alternateLoadableFile>
        </loading>
        <subordinates>
        </subordinates>
      </compileType>
      <makeCustomizationType>
        <makeCustomizationPreStepEnabled>false</makeCustomizationPreStepEnabled>
        <makeUseCleanTarget>false</makeUseCleanTarget>
        <makeCustomizationPreStep></makeCustomizationPreStep>
        <makeCustomizationPostStepEnabled>false</makeCustomizationPostStepEnabled>
        <makeCustomizationPostStep></makeCustomizationPostStep>
        <makeCustomizationPutChecksumInUserID>false</makeCustomizationPutChecksumInUserID>
        <makeCustomizationEnableLongLines>false</makeCustomizationEnableLongLines>
        <makeCustomizationNormalizeHexFile>false</makeCustomizationNormalizeHexFile>
      </makeCustomizationType>
      <HI-TECH-COMP>
        <property key="additional-warnings" value="true"/>
        <property key="asmlist" value="true"/>
        <property key="call-prologues" value="false"/>
        <property key="default-bitfield-type" value="true"/>
        <property key="default-char-type" value="true"/>
        <property key="define-macros" value="{def_value}"/>
        <property key="disable-optimizations" value="true"/>
        <property key="extra-include-directories" value="{inc_value}"/>
        <property key="favor-optimization-for" value="-speed,+space"/>
        <property key="garbage-collect-data" value="true"/>
        <property key="garbage-collect-functions" value="true"/>
        <property key="identifier-length" value="255"/>
        <property key="local-generation" value="false"/>
        <property key="operation-mode" value="free"/>
        <property key="opt-xc8-compiler-strict_ansi" value="false"/>
        <property key="optimization-assembler" value="true"/>
        <property key="optimization-assembler-files" value="true"/>
        <property key="optimization-debug" value="false"/>
        <property key="optimization-invariant-enable" value="false"/>
        <property key="optimization-invariant-value" value="16"/>
        <property key="optimization-level" value="-O0"/>
        <property key="optimization-speed" value="false"/>
        <property key="optimization-stable-enable" value="false"/>
        <property key="preprocess-assembler" value="true"/>
        <property key="short-enums" value="true"/>
        <property key="tentative-definitions" value="-fno-common"/>
        <property key="undefine-macros" value=""/>
        <property key="use-cci" value="false"/>
        <property key="use-iar" value="false"/>
        <property key="verbose" value="false"/>
        <property key="warning-level" value="-3"/>
        <property key="what-to-do" value="ignore"/>
      </HI-TECH-COMP>
      <HI-TECH-LINK>
        <property key="additional-options-checksum" value=""/>
        <property key="additional-options-checksumAVR" value=""/>
        <property key="additional-options-checksumAVR2" value="0"/>
        <property key="additional-options-code-offset" value=""/>
        <property key="additional-options-command-line" value=""/>
        <property key="additional-options-errata" value=""/>
        <property key="additional-options-extend-address" value="false"/>
        <property key="additional-options-fillAVR2" value="0"/>
        <property key="additional-options-fillAVR3" value="0"/>
        <property key="additional-options-fillAVR4" value="0"/>
        <property key="additional-options-fillAVR5" value="0"/>
        <property key="checksum-flash-options-addressce" value=""/>
        <property key="checksum-flash-options-addresscs" value=""/>
        <property key="checksum-flash-options-algorithmc"
                  value="Select checksum algorithm"/>
        <property key="checksum-flash-options-destc" value=""/>
        <property key="checksum-flash-options-src" value=""/>
        <property key="checksum-flash-options-widthc" value="2"/>
        <property key="clear-bss" value="true"/>
        <property key="code-model-external" value="wordwrite"/>
        <property key="code-model-rom" value=""/>
        <property key="create-html-files" value="false"/>
        <property key="data-model-ram" value=""/>
        <property key="data-model-size-of-double" value="24"/>
        <property key="data-model-size-of-double-gcc" value="no-short-double"/>
        <property key="data-model-size-of-float" value="24"/>
        <property key="data-model-size-of-float-gcc" value="no-short-float"/>
        <property key="display-class-usage" value="false"/>
        <property key="display-hex-usage" value="false"/>
        <property key="display-overall-usage" value="true"/>
        <property key="display-psect-usage" value="false"/>
        <property key="extra-lib-directories" value=""/>
        <property key="fill-flash-options-addr" value=""/>
        <property key="fill-flash-options-addrfe" value=""/>
        <property key="fill-flash-options-addrfs" value=""/>
        <property key="fill-flash-options-constf" value=""/>
        <property key="fill-flash-options-const" value=""/>
        <property key="fill-flash-options-conste" value=""/>
        <property key="fill-flash-options-constl" value=""/>
        <property key="fill-flash-options-constw" value=""/>
        <property key="format-hex-file-for-download" value="false"/>
        <property key="initialize-data" value="true"/>
        <property key="input-libraries" value="libm"/>
        <property key="keep-generated-startup.as" value="false"/>
        <property key="link-in-c-library" value="true"/>
        <property key="link-in-c-library-gcc" value=""/>
        <property key="link-in-peripheral-library" value="false"/>
        <property key="managed-stack" value="false"/>
        <property key="opt-xc8-linker-file" value="false"/>
        <property key="opt-xc8-linker-link_startup" value="false"/>
        <property key="opt-xc8-linker-serial" value=""/>
        <property key="program-the-device-with-default-config-words" value="false"/>
        <property key="remove-unused-sections" value="true"/>
      </HI-TECH-LINK>
      <XC8-CO>
        <property key="coverage-enable" value=""/>
        <property key="stack-guidance" value="false"/>
      </XC8-CO>
      <XC8-config-global>
        <property key="advanced-elf" value="true"/>
        <property key="constdata-progmem" value="false"/>
        <property key="gcc-opt-driver-new" value="true"/>
        <property key="gcc-opt-std" value="-std=c99"/>
        <property key="gcc-output-file-format" value="dwarf-3"/>
        <property key="mapped-progmem" value="false"/>
        <property key="omit-pack-options" value="false"/>
        <property key="omit-pack-options-new" value="1"/>
        <property key="output-file-format" value="-mcof,+elf"/>
        <property key="smart-io-format" value=""/>
        <property key="stack-size-high" value="auto"/>
        <property key="stack-size-low" value="auto"/>
        <property key="stack-size-main" value="auto"/>
        <property key="stack-type" value="compiled"/>
        <property key="user-pack-device-support" value=""/>
        <property key="wpo-lto" value="false"/>
      </XC8-config-global>
    </conf>
  </confs>
</configurationDescriptor>
"""


def _project_xml(sources: list[str]) -> str:
    roots = "\n".join(
        f'                <sourceRootElem>{d}</sourceRootElem>'
        for d in _source_roots(sources))
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://www.netbeans.org/ns/project/1">
    <type>com.microchip.mplab.nbide.embedded.makeproject</type>
    <configuration>
        <data xmlns="http://www.netbeans.org/ns/make-project/1">
            <name>{PROJECT_NAME}</name>
            <creation-uuid>6b1f6c2e-9a41-4d0a-9c1a-2c1f0a7b3d55</creation-uuid>
            <make-project-type>0</make-project-type>
            <sourceEncoding>ISO-8859-1</sourceEncoding>
            <make-dep-projects/>
            <sourceRootList>
{roots}
            </sourceRootList>
            <confList>
                <confElem>
                    <name>default</name>
                    <type>2</type>
                </confElem>
            </confList>
            <formatting>
                <project-formatting-style>false</project-formatting-style>
            </formatting>
        </data>
    </configuration>
</project>
"""


def _config_source(example: epicmanifest.Example) -> str:
    """The configuration-word translation unit.

    The manifest's example config is the single source of truth for the
    config words: the manifest build emits the same pragmas in its own
    TU, and the IDE needs them in a file it can compile. The values
    are copied verbatim, so the two builds configure the part
    identically.
    """
    lines = [
        "/* Generated by scripts/build/gen_usb_mplabx.py. Do not edit;",
        " * regenerate. The configuration words are lib/usb's manifest",
        " * example config, the same set the manifest-driven build emits.",
        " */",
        "#include <xc.h>",
    ]
    lines += [f"#pragma config {key} = {value}"
              for key, value in example.config.items()]
    return "\n".join(lines) + "\n"


def _static_files() -> dict[str, str]:
    """The nbproject files that do not depend on the module's file list.

    They are the reference project's own, with the project name
    re-targeted: they carry the XC8 toolchain locations and the
    NetBeans makefile plumbing, which is the same for every project
    of this family.
    """
    out: dict[str, str] = {}
    for name in STATIC_FILES:
        text = (REFERENCE / name).read_text()
        out[name] = text.replace("epic-hal-demo-pic18fxx5x", PROJECT_NAME)
    return out


def project_contents() -> tuple[list[str], list[str], list[str], str]:
    """The project's file list, include dirs, macros and config TU.

    Returned as the manifest resolves them, with the paths relative to
    the .X directory, which is what the project files carry.
    """
    manifest = epicmanifest.load(epicmanifest.default_path())
    fam = manifest.families[FAMILY]

    mod = manifest.modules[MODULE]
    example = manifest.example_for(MODULE, FAMILY, MCU)
    if example is None:
        raise SystemExit(f"gen_usb_mplabx: {MODULE} has no {FAMILY} example")

    # The same source list the manifest build compiles for this part.
    sources = [epicmanifest.join_module_path(mod.dir, s)
               for s in mod.sources]
    sources += [epicmanifest.join_module_path(mod.dir, s)
                for s in example.sources]

    # The project's paths are relative to the .X directory, so they climb
    # back to the repository root by one level per path segment.
    depth = len(PROJECT.relative_to(REPO).parts)
    prefix = "/".join([".."] * depth)
    rel_sources = [f"{prefix}/{s}" for s in sources]
    rel_sources.append("config.c")

    includes = [f"{prefix}/{epicmanifest.join_module_path(mod.dir, i)}"
                for i in mod.includes]
    defines = [f"PIC{MCU}", f"FOSC_HZ={fam.fosc_hz}"]
    return rel_sources, includes, defines, _config_source(example)


def generate() -> dict[str, str]:
    """Every file of the project, as repo-root-relative path -> content."""
    sources, includes, defines, config = project_contents()
    base = str(PROJECT.relative_to(REPO))
    files = {
        base + "/nbproject/configurations.xml":
            _configurations_xml(sources, includes, defines),
        base + "/nbproject/project.xml": _project_xml(sources),
        base + "/config.c": config,
    }
    for name, content in _static_files().items():
        files[f"{base}/{name}"] = content
    return files


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="fail on a stale project instead of writing it")
    args = parser.parse_args()

    contents = generate()
    stale = [rel for rel, content in contents.items()
             if not (REPO / rel).is_file()
             or (REPO / rel).read_text() != content]
    if args.check:
        if stale:
            print("gen_usb_mplabx: stale: " + ", ".join(stale)
                  + "; run scripts/build/gen_usb_mplabx.py", file=sys.stderr)
            return 1
        return 0
    for rel, content in contents.items():
        path = REPO / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        print(f"gen_usb_mplabx: wrote {rel}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
