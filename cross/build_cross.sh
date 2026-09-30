#!/bin/bash
# Кросс-сборка Auto_Cleaner под Raspberry Pi (armhf) на этой машине.
# Нужен sysroot с Pi: cross/sync_sysroot.sh
# Результат: $BUILD/Auto_Cleaner_APPM_rspb
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$HERE/.." && pwd)
SYSROOT=${SYSROOT:-$(cd "$SRC/.." && pwd)/rpi-sysroot}
BUILD=${BUILD:-$(cd "$SRC/.." && pwd)/build-rpi}
APP=Auto_Cleaner_APPM_rspb

if [ ! -d "$SYSROOT/usr/lib/arm-linux-gnueabihf/qt5" ]; then
    echo "Нет sysroot в $SYSROOT - сначала запустите cross/sync_sysroot.sh" >&2
    exit 1
fi

mkdir -p "$BUILD"
cd "$BUILD"

# Qt с Pi (заголовки, библиотеки, mkspecs), а moc/uic/rcc - местные
cat > qt.conf <<EOF
[Paths]
Sysroot=$SYSROOT
SysrootifyPrefix=true
Prefix=/usr
ArchData=lib/arm-linux-gnueabihf/qt5
Headers=include/arm-linux-gnueabihf/qt5
Libraries=lib/arm-linux-gnueabihf
Plugins=lib/arm-linux-gnueabihf/qt5/plugins
Qml2Imports=lib/arm-linux-gnueabihf/qt5/qml
Data=share/qt5
HostPrefix=/usr
HostBinaries=lib/qt5/bin
HostLibraries=lib/x86_64-linux-gnu
HostData=$SYSROOT/usr/lib/arm-linux-gnueabihf/qt5
EOF

export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
export PKG_CONFIG_LIBDIR="$SYSROOT/usr/lib/arm-linux-gnueabihf/pkgconfig:$SYSROOT/usr/share/pkgconfig"
unset PKG_CONFIG_PATH

/usr/lib/qt5/bin/qmake -qtconf qt.conf -spec "$HERE/linux-armhf-g++" "$SRC/$APP.pro" CONFIG+=release
make -j"$(nproc)"

# Проверяем, что бинарник не требует glibc/libstdc++ новее, чем на Pi
LIBDIR="$SYSROOT/usr/lib/arm-linux-gnueabihf"
missing=0
for v in $(arm-linux-gnueabihf-objdump -T "$APP" | grep -oE '(GLIBC|GLIBCXX|CXXABI)_[0-9.]+' | sort -u); do
    case $v in
        GLIBC_*) lib="$LIBDIR/libc.so.6 $LIBDIR/libm.so.6" ;;
        *)       lib="$LIBDIR/libstdc++.so.6" ;;
    esac
    if ! grep -qa -- "$v" $lib; then
        echo "На Pi нет $v" >&2
        missing=1
    fi
done
if [ $missing -ne 0 ]; then
    echo "Бинарник не запустится на Pi" >&2
    exit 1
fi

echo "Готово: $BUILD/$APP"
