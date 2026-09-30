#!/bin/bash
# Сборка на этом компьютере и запуск в режиме симуляции машины (без CAN, GPIO и техники).
# Рядом с главным окном открывается окно симуляции: положения органов, выходы, датчики.
# Использование: sim/run_sim.sh [дополнительные ключи приложения]
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$HERE/.." && pwd)
BUILD=${BUILD:-$(cd "$SRC/.." && pwd)/build-sim}
APP=Auto_Cleaner_APPM_rspb

mkdir -p "$BUILD"
cd "$BUILD"
/usr/lib/qt5/bin/qmake "$SRC/$APP.pro" CONFIG+=debug
make -j"$(nproc)"

# локальная сборка libgpiod (см. CLAUDE.md)
export LD_LIBRARY_PATH="$HOME/libgpiod-1.6/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec ./$APP --sim "$@"
