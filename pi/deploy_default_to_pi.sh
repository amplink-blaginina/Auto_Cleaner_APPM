#!/bin/bash
# Замена сборки, которую запускает автозапуск (сервис my-app) на Raspberry Pi.
# Заменяется тот бинарник, который сервис запускает сейчас (старый -> .prev), затем сервис перезапускается.
# Путь можно задать явно: DEFAULT_BIN=/home/knight/.../бинарник pi/deploy_default_to_pi.sh
# Использование: pi/deploy_default_to_pi.sh [user@host]      (по умолчанию knight@192.168.68.128)
# Откат: pi/rollback_default_on_pi.sh
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
source "$HERE/pi_common.sh"

pi_build

pi_require_default_bin
ssh "$PI" "systemctl cat $SERVICE | grep -E '^(ExecStart|WorkingDirectory)=.' ; md5sum '$DEFAULT_BIN'" || true
echo "Новая сборка: $(md5sum "$BUILD/$APP" | cut -d' ' -f1)  $(strings -el "$BUILD/$APP" | grep -m1 'APPM v')"
pi_confirm "Заменить $DEFAULT_BIN новой сборкой?"

pi_sync_clock
scp "$BUILD/$APP" "$PI:$DEFAULT_BIN.new"

pi_root_script "
set -e
systemctl stop $SERVICE
pkill -x $APP_COMM || true        # экземпляр, запущенный вручную
[ -f '$DEFAULT_BIN' ] && mv -f '$DEFAULT_BIN' '$DEFAULT_BIN.prev'
mv -f '$DEFAULT_BIN.new' '$DEFAULT_BIN'
chmod +x '$DEFAULT_BIN'
systemctl start $SERVICE
sleep 3
systemctl status $SERVICE --no-pager | head -12
"
echo "Лог приложения: ssh $PI journalctl -u $SERVICE -f"
