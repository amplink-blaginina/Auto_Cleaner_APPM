#!/bin/bash
# Сборка пакета и установка его на Pi с этого компьютера. Сама установка - скриптом из пакета
# (package/install_from_computer.sh), тем же, которым ставят с любого другого компьютера.
# Использование: pi/install_package_to_pi.sh [Pi] [пакет.tar.gz]
#   Pi - IP (192.168.68.140), сетевое имя (appm-012.local) или имя из ~/.ssh/config; пользователь по умолчанию knight.
#   Без аргументов - knight@192.168.68.128 и свежая сборка текущего коммита.
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
source "$HERE/pi_common.sh"
PACKAGE=${2:-}

if [ -z "$PACKAGE" ]; then
    out=$("$HERE/make_package.sh" | tee /dev/stderr)
    PACKAGE=$(echo "$out" | sed -n 's/^Пакет: //p' | tail -1)
fi
[ -f "$PACKAGE" ] || { echo "Нет пакета $PACKAGE" >&2; exit 1; }

UNPACK=$(mktemp -d)
trap 'rm -rf "$UNPACK"' EXIT
tar -xzf "$PACKAGE" -C "$UNPACK"
bash "$UNPACK"/*/install_from_computer.sh "$PI"
