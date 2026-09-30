#!/bin/bash
# Разовый запуск свежей сборки на Raspberry Pi для проверки.
# Автозапуск (сервис my-app) на время теста останавливается, после Ctrl+C или обрыва ssh - включается обратно.
# Бинарник кладётся в TEST_DIR (старый -> .prev) и берёт настройки оттуда же (settingsAutoCleaner.ini рядом с бинарником).
# Использование: pi/deploy_test_to_pi.sh [user@host]      (по умолчанию knight@192.168.68.128)
# Первый раз (и после apt upgrade на Pi): pi/sync_sysroot.sh [user@host]
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
source "$HERE/pi_common.sh"
TEST_DIR=/home/knight/work/Auto_Cleaner_318D4

pi_build
pi_sync_clock
scp "$BUILD/$APP" "$PI:$TEST_DIR/$APP.new"

pi_root_script "
set -e
cd $TEST_DIR
systemctl stop $SERVICE
# при любом выходе (Ctrl+C, обрыв ssh, падение) возвращаем автозапуск
trap 'echo \"== возвращаем автозапуск ($SERVICE)\"; systemctl start $SERVICE' EXIT
trap 'exit 130' INT TERM HUP
pkill -x $APP_COMM || true        # экземпляр, запущенный вручную
sleep 1
pgrep -ax $APP_COMM && echo 'ВНИМАНИЕ: старый экземпляр ещё жив!' || true
[ -f $APP ] && mv -f $APP $APP.prev
mv -f $APP.new $APP
chmod +x $APP
$PI_ENV
$PI_CAN_INIT
echo '== тестовый запуск, Ctrl+C - остановить и вернуть автозапуск'
# при падении ядро пишет дамп памяти ./core - для разбора причины
rm -f core
ulimit -c unlimited
set +e
./$APP
[ -f core ] && chown knight:knight core
exit 0
" || true   # Ctrl+C - обычное завершение теста

# дамп памяти после падения забираем сюда: gdb-multiarch <бинарник> <дамп>
if ssh "$PI" "test -f $TEST_DIR/core"; then
    scp "$PI:$TEST_DIR/core" "$BUILD/core"
    cp -f "$BUILD/$APP" "$BUILD/$APP.crashed"
    echo "Программа упала, дамп: $BUILD/core (бинарник: $BUILD/$APP.crashed)"
fi
