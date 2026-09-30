#!/bin/bash
# Сборка на этой машине и деплой готового бинарника на Raspberry Pi.
# Использование: ./deploy_bin_to_pi.sh [user@host]      (по умолчанию knight@192.168.68.128)
# Первый раз (и после apt upgrade на Pi): cross/sync_sysroot.sh [user@host]
set -euo pipefail

PI=${1:-${PI:-knight@192.168.68.128}}
DST_DIR=/home/knight/work/Auto_Cleaner_318D4
APP=Auto_Cleaner_APPM_rspb
APP_COMM=Auto_Cleaner_AP          # /proc/<pid>/comm хранит только 15 символов
HERE=$(cd "$(dirname "$0")" && pwd)
BUILD=${BUILD:-$(cd "$HERE/.." && pwd)/build-rpi}

# 1. Кросс-сборка (с проверкой совместимости с библиотеками Pi)
BUILD="$BUILD" "$HERE/cross/build_cross.sh"

# 2. Синхронизация времени (у Pi нет RTC; ssh -t - sudo сможет спросить пароль)
ssh -t "$PI" "sudo date -s @$(date +%s) && date"

# 3. Копируем бинарник рядом со старым
scp "$BUILD/$APP" "$PI:$DST_DIR/$APP.new"

# 4. Останавливаем старый процесс, подменяем бинарник (старый -> .prev для отката) и запускаем
ssh -t "$PI" "
set -e
cd $DST_DIR
sudo pkill -x $APP_COMM || true
sleep 1
pgrep -ax $APP_COMM && echo 'ВНИМАНИЕ: старый экземпляр ещё жив!' || true
[ -f $APP ] && mv -f $APP $APP.prev
mv -f $APP.new $APP
chmod +x $APP
sudo ./$APP -platform eglfs
"
