#!/bin/bash
# Пакет для установки новой версии на машину без этого компьютера (флешка или scp).
# Кросс-сборка + архив: бинарник, скрипты установки и отката для запуска на самой Pi, инструкция.
# Результат: ../release/Auto_Cleaner_<ветка>_<коммит>.tar.gz
# Использование: pi/make_package.sh
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$HERE/.." && pwd)
BUILD=${BUILD:-$(cd "$SRC/.." && pwd)/build-rpi}
OUT=${OUT:-$(cd "$SRC/.." && pwd)/release}
APP=Auto_Cleaner_APPM_rspb

cd "$SRC"
if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    echo "В рабочей копии есть незакоммиченные изменения - в пакет попадёт не то, что в коммите." >&2
    echo "Закоммитьте их или запустите с ALLOW_DIRTY=1" >&2
    [ "${ALLOW_DIRTY:-0}" = 1 ] || exit 1
fi
BRANCH=$(git rev-parse --abbrev-ref HEAD)
COMMIT=$(git rev-parse --short HEAD)
NAME=Auto_Cleaner_${BRANCH}_${COMMIT}

BUILD="$BUILD" "$HERE/build_cross.sh"

PKG="$OUT/$NAME"
rm -rf "$PKG"
mkdir -p "$PKG"
cp "$BUILD/$APP" "$PKG/$APP"
cp "$HERE/package/install.sh" "$HERE/package/rollback.sh" "$HERE/package/install_from_computer.sh" \
   "$HERE/package/ИНСТРУКЦИЯ.txt" "$PKG/"
VERSION=$(strings -el "$BUILD/$APP" | grep -m1 'APPM v' || true)
{
    echo "Версия:  $VERSION"
    echo "Ветка:   $BRANCH"
    echo "Коммит:  $(git log -1 --format='%h %s')"
    echo "Собрано: $(date '+%d.%m.%Y %H:%M')"
    echo "MD5:     $(md5sum "$PKG/$APP" | cut -d' ' -f1)"
} > "$PKG/ВЕРСИЯ.txt"

tar -czf "$OUT/$NAME.tar.gz" -C "$OUT" "$NAME"
echo
cat "$PKG/ВЕРСИЯ.txt"
echo "Пакет: $OUT/$NAME.tar.gz"
