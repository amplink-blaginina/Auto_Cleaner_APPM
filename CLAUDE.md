# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Qt 5 / C++17 qmake application for the Auto Cleaner machine, running on a Raspberry Pi as root on the `eglfs` platform.

## Build

- qmake, Qt 5.15. Deps: gstreamer-1.0 (+app, video), glib-2.0, poppler-qt5 (all via pkg-config), libgpiod 1.6.3 (on this x86 machine a local build at `$HOME/libgpiod-1.6`; on the Pi and in the cross build the system package).
- Target Pi: Raspbian 12 bookworm, 32-bit armhf userland (Qt 5.15.8, glibc 2.36, gcc 12). Build/deploy scripts live in `pi/` (described in `pi/README.md`). Cross build here: `pi/sync_sysroot.sh` once (copies libs from the Pi to `../rpi-sysroot`), then `pi/build_cross.sh` → `../build-rpi/Auto_Cleaner_APPM_rspb`. It compiles against the Pi's C++ headers (`-nostdinc++`) and links the Pi's libstdc++ (`-nostdlib++`): the local gcc 15 libstdc++ needs glibc 2.38, and gcc 15 headers don't match the Pi's gcc 12 libstdc++ (suspected cause of a segfault on start); and it fails if the binary needs newer GLIBC/GLIBCXX symbols than the Pi has.
- Stale `ui_*.h`/`moc_*` in the repo root shadow freshly generated ones (the root is on the include path before the build dir). Keep the root clean.
- To check a change, build out-of-source with `/build-check`. Never run qmake/make in the repo root — in-source builds litter it with `*.o`, `moc_*`, `ui_*.h`, `Makefile`.
- New `.cpp`/`.h`/`.ui` files must be added to `SOURCES`/`HEADERS`/`FORMS` in `Auto_Cleaner_APPM_rspb.pro` by hand.
- Do NOT run `pi/deploy_*.sh`, `pi/rollback_default_on_pi.sh` or anything in `pi/old/`: they stop the app on a live machine and restart it. The user does hardware verification.
  - On the Pi the app is autostarted by the systemd service `my-app` (`Restart=always`); the binary it runs is set by its `ExecStart` script and may be overridden by a drop-in, so a plain `pkill` + manual start gets replaced by the service.
  - `deploy_test_to_pi.sh [user@host]` cross-builds, stops `my-app`, runs the fresh binary once in the terminal and starts `my-app` again on exit.
  - `deploy_default_to_pi.sh [user@host]` replaces the binary `my-app` currently runs (old → `.prev`, asks for confirmation) and restarts the service. `rollback_default_on_pi.sh` swaps it back with `.prev`.
  - The old scripts in `pi/old/` rebuild on the Pi.
- No tests exist; a clean local build is the verification bar.
- `docs/machine_checks.md` (Russian) is the checklist for the person testing on the real machine. When a change affects machine behavior, add items there under the current branch section: what to do → what should happen, and what counts as a failure. Newest section on top.

## Code conventions

- Write code comments in Russian, matching existing code.
- Use function-pointer `connect(obj, &Class::signal, ...)` only. When editing code that uses `SIGNAL()/SLOT()`, migrate those connects.
- `mainwindow.cpp` is a god-object being broken up: put new logic in `Controllers/` or `organs/` (machine-unit state machines), not in `MainWindow`.
- Machine signals go through `IoBus` (`io/`): `io->get/set(DeviceStates)`, not `MyCan::getState/setState` (MyCan stays for board configuration). Shared resources are arbitrated in `machine/`: the power valve A1 via `HydraulicSupply::request(this, bool)`, engine rpm via `EngineRpmDemand::request/release` — never write `StateValveA1` or `setEngineCommand` directly. Organs get them through `MachineIo`.
- Simulation without hardware: `sim/run_sim.sh` (or run the app with `--sim`) uses `SimIoBus` — a plant model moves organs by their valves and sets limit switches; the sim window shows outputs and lets you set inputs. When adding an organ with position sensors, add its axis in `SimIoBus`'s constructor.
- `camera/`, `vlc/` and `organs/frontrail_old.cpp` are dead code — don't add them to the `.pro` or edit them.
- gpio uses `.hpp` headers; everything else `.h`.
- Format only the lines you changed: `git clang-format` (uses `.clang-format`). Never run `clang-format -i` on whole files — it would reformat unrelated code and bloat diffs.

## Git

- Branch off `master`. Commit messages: short, lowercase English ("blower fixes", "new connect").
- Version bumps happen only in dedicated "Version up (X.XXX)" commits at release — don't bump the version otherwise.
- Many build artifacts are tracked despite `.gitignore` (`build/`, root `*.o`, `moc_*`, `ui_*.h`, `qrc_*`, `Makefile`, `*.pro.user`, the `Auto_Cleaner_APPM_rspb` binary). Never stage them; stage files explicitly, never `git add -A`/`commit -a`. A pre-commit hook rejects them.

## Gotchas

- Settings keys `Global/canDeivce` (can1) and `Global/j1939Deivce` (can0) have a real typo — don't "fix" it, it breaks existing `settingsAutoCleaner.ini` files.
- Hardcoded runtime paths on the Pi (`/home/knight/...`, `/opt/...`) are intentional.
