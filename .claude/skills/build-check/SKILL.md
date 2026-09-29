---
name: build-check
description: Compile the project out-of-source to verify C++/Qt changes build cleanly. Use after editing .cpp/.h/.ui/.pro files, before telling the user a change is done.
---

Build out-of-source so the repo root stays free of `*.o`, `moc_*`, `ui_*.h` and `Makefile`.

1. Pick a build dir outside the repo: `$SCRATCH/build-check` if a scratchpad directory is available, otherwise `/tmp/auto_cleaner_build_check`. Reuse it between runs for incremental builds.
2. Use Qt 5's qmake (`qmake -v` must report Qt 5.x; if plain `qmake` is Qt 6, look for `qmake-qt5` or `/usr/lib/qt5/bin/qmake`).
3. Run:
   ```
   mkdir -p "$BUILD" && cd "$BUILD" && qmake /home/helen/Projects/Auto_Cleaner_402/Auto_Cleaner_APPM_rspb/Auto_Cleaner_APPM_rspb.pro CONFIG+=debug && make -j"$(nproc)" 2>&1 | tail -200
   ```
   `qrc_images.cpp` is huge (~48 MB); the first build is slow — use a long timeout or run in the background.
4. If you added/removed files, make sure they're listed in the `.pro` first, and re-run qmake.
5. Report: success/failure, every error, and warnings only in files you changed. Don't fix unrelated pre-existing warnings.
6. If a dependency is missing locally (e.g. `$HOME/libgpiod-1.6`, poppler-qt5, gstreamer dev packages), say so and stop — don't install anything or edit the `.pro` to work around it.

Never run qmake/make in the repo root and never run the `deploy_to_pi*.sh` scripts.
