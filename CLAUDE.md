# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Qt 5 / C++17 qmake application for the Auto Cleaner machine, running on a Raspberry Pi as root on the `eglfs` platform.

## Build

- qmake, Qt 5.15. Deps: gstreamer-1.0 (+app, video), glib-2.0, poppler-qt5, and a locally built libgpiod 1.6.3 at `$HOME/libgpiod-1.6` (headers must come before `/usr/include`).
- To check a change, build out-of-source with `/build-check`. Never run qmake/make in the repo root — in-source builds litter it with `*.o`, `moc_*`, `ui_*.h`, `Makefile`.
- New `.cpp`/`.h`/`.ui` files must be added to `SOURCES`/`HEADERS`/`FORMS` in `Auto_Cleaner_APPM_rspb.pro` by hand.
- Do NOT run `deploy_to_pi*.sh` / `script_to_build*.sh`: they rsync to a live machine, kill the running app and rebuild on the Pi. The user does hardware verification.
- No tests exist; a clean local build is the verification bar.

## Code conventions

- Write code comments in Russian, matching existing code.
- Use function-pointer `connect(obj, &Class::signal, ...)` only. When editing code that uses `SIGNAL()/SLOT()`, migrate those connects.
- `mainwindow.cpp` is a god-object being broken up: put new logic in `Controllers/` or `organs/` (machine-unit state machines), not in `MainWindow`.
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
