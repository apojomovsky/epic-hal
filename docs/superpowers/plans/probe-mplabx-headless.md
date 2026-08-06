# Probe: can MPLAB X projects be built headlessly in the toolchain image?

Verdict: **UNTESTED** (mechanism confirmed present; not yet exercised
against a real project). Re-probe once Task 2 creates a genuine,
complete `.X` project, before Task 4 needs the final YES/NO answer.

Run on 2026-08-06 against image tag
`ghcr.io/apojomovsky/pic8-hal-ci:xc8-v4.00-dfp1.7.162-1.7.171-1.9.258-mplabx6.35`
(local cache, same tag CI resolves).

## What was run

### Step 1: what the image contains

```sh
docker run --rm pic8-hal-toolchain:local bash -c '
ls "$MPLABX_INSTALL_DIR/mplab_platform/bin/"
which make xc8-cc
ls "$MPLABX_INSTALL_DIR/mplab_platform/bin/prjMakefilesGenerator.sh" \
  && echo "generator present" || echo "generator ABSENT"
'
```

Output:

```
checksum.jar
cicdw
common-vars.sh
cppcheck
extractobjectdependencies.jar
fixDeps
hexmate
itidproc
make
mdb.sh
misracli.sh
mplab_ide
mplabwildcard
packmanagercli.sh
prjMakefilesGenerator.sh
projectPackager.sh
roam.lic
xclm
/opt/microchip/mplabx/v6.35/mplab_platform/bin/make
/opt/microchip/xc8/v4.00/bin/xc8-cc
/opt/microchip/mplabx/v6.35/mplab_platform/bin/prjMakefilesGenerator.sh
generator present
```

`make`, `xc8-cc`, `mdb.sh`, and `prjMakefilesGenerator.sh` are all
present and on `PATH` (or at their documented absolute paths).

### Step 2: try to build the one existing project

```sh
cd pic16f87xa-hal/mcu/pic16f87xa-mplabx
ls nbproject/
"$MPLABX_INSTALL_DIR/mplab_platform/bin/prjMakefilesGenerator.sh" -v .
```

`ls nbproject/` returned only `configurations.xml`. A real MPLAB X `.X`
project needs `project.xml` as well; this directory does not have one.
Running the generator against it produced:

```
Processing .
Exception in thread "main" java.lang.Exception: Project could not be found: /repo/pic16f87xa-hal/mcu/pic16f87xa-mplabx
	at com.microchip.makegenerator.handler.MakeProjectHandler.openProject(MakeProjectHandler.java:381)
	at com.microchip.makegenerator.handler.MakeProjectHandler.generateMakefiles(MakeProjectHandler.java:287)
	at com.microchip.makegenerator.handler.MakeProjectHandler.run(MakeProjectHandler.java:282)
	at com.microchip.makegenerator.handler.MakeProjectHandler.runTasks(MakeProjectHandler.java:121)
	at com.microchip.makegenerator.handler.XIDECommandsHandler.run(XIDECommandsHandler.java:42)
	at com.microchip.makegenerator.GenerateMakefiles.main(GenerateMakefiles.java:54)
```

No `nbproject/Makefile-default.mk` was generated, so the plan's next
step (`make -f nbproject/Makefile-default.mk SUBPROJECTS= .build-conf`)
fails immediately with `No such file or directory`, which is expected
given the above, not new information.

### Why the plan's assumption did not hold

The plan's own note says this probe is possible because "the previous
plan deleted this directory's Makefile but kept `nbproject/`." That
part is true, but `nbproject/` was never a complete project to begin
with in the current tree: besides missing `project.xml`, the
`configurations.xml` that does exist targets **XC8 v2.40** (the
toolchain image pins v4.00) and lists source files
(`src/core/pic16f87xa_interrupt.c`, etc.) that do not exist in the
current layout; it predates the multi-family manifest refactor and was
never kept in sync. Reading it is a useful reference for MPLAB X's XML
shape, but it cannot stand in for a real project in this probe.

## What this means

The toolchain mechanism (`prjMakefilesGenerator.sh` + `make -f
nbproject/Makefile-default.mk`) is present and runnable in the
container; whether it actually **builds** a real Epicurus reference
project headlessly is not yet known, because there is no complete
project to test it against. That is exactly what Task 2 creates.

**Action for Task 4:** re-run this probe against one of Task 2's real
`.X` projects (once a human has created it in the MPLAB X GUI and
committed it) before deciding which of Task 4's two branches to take.
Until then, Task 4 should default to the presence-check path (Step 1b)
rather than assume Step 1a works, and the probe should be updated with
a real YES/NO once re-run.
