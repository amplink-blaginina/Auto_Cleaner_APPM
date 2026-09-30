#!/bin/bash
# Возврат предыдущей сборки автозапуска (сервис my-app) на Raspberry Pi.
# Меняет местами бинарник автозапуска и его .prev (оставленный deploy_default_to_pi.sh),
# поэтому повторный запуск возвращает обратно. Затем сервис перезапускается.
# Путь можно задать явно: DEFAULT_BIN=/home/knight/.../бинарник pi/rollback_default_on_pi.sh
# Использование: pi/rollback_default_on_pi.sh [user@host]      (по умолчанию knight@192.168.68.128)
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
source "$HERE/pi_common.sh"

pi_require_default_bin
if ! ssh "$PI" "test -f '$DEFAULT_BIN.prev'"; then
    echo "Нет предыдущей сборки: $DEFAULT_BIN.prev" >&2
    exit 1
fi
# версии: строка версии хранится в UTF-16, ищем её через strings -el
ssh "$PI" "for f in '$DEFAULT_BIN' '$DEFAULT_BIN.prev'; do
    printf '%s\n    %s  %s\n' \"\$f\" \"\$(stat -c %y \"\$f\" | cut -d. -f1)\" \"\$(strings -el \"\$f\" 2>/dev/null | grep -m1 'APPM v')\"
done"
pi_confirm "Вернуть $DEFAULT_BIN.prev (текущая сборка станет .prev)?"

pi_root_script "
set -e
systemctl stop $SERVICE
pkill -x $APP_COMM || true        # экземпляр, запущенный вручную
mv -f '$DEFAULT_BIN' '$DEFAULT_BIN.swap'
mv -f '$DEFAULT_BIN.prev' '$DEFAULT_BIN'
mv -f '$DEFAULT_BIN.swap' '$DEFAULT_BIN.prev'
systemctl start $SERVICE
sleep 3
systemctl status $SERVICE --no-pager | head -12
"
echo "Лог приложения: ssh $PI journalctl -u $SERVICE -f"
