#!/bin/bash
# Копирует с Raspberry заголовки и библиотеки (sysroot) для кросс-сборки.
# Запускать заново после обновления пакетов на Pi (apt upgrade).
# Использование: cross/sync_sysroot.sh [user@host]
set -euo pipefail

PI=${1:-${PI:-knight@192.168.68.128}}
HERE=$(cd "$(dirname "$0")" && pwd)
SYSROOT=${SYSROOT:-$(cd "$HERE/../.." && pwd)/rpi-sysroot}

mkdir -p "$SYSROOT/usr/lib" "$SYSROOT/usr/share"
for dir in /usr/include /usr/lib/arm-linux-gnueabihf /usr/lib/linux /usr/share/pkgconfig; do
    echo "== $dir"
    rsync -a --delete --info=stats1 "$PI:$dir/" "$SYSROOT$dir/"
done
# bookworm: /lib -> usr/lib; загрузчик лежит прямо в /usr/lib (на него ссылается libc.so)
ln -sfn usr/lib "$SYSROOT/lib"
ln -sfn arm-linux-gnueabihf/ld-linux-armhf.so.3 "$SYSROOT/usr/lib/ld-linux-armhf.so.3"

# абсолютные симлинки (/lib/arm-linux-gnueabihf/libX.so.1) переводим внутрь sysroot
find "$SYSROOT" -type l -lname '/*' | while read -r link; do
    ln -sfn "$SYSROOT$(readlink "$link")" "$link"
done

echo "sysroot: $SYSROOT"
