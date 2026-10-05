#!/bin/bash
# Возврат предыдущей версии после install.sh. Запускать на самой Pi: sudo bash rollback.sh
# Меняет местами программу автозапуска и её копию .prev; повторный запуск возвращает обратно.
# Путь можно задать явно: sudo bash rollback.sh /home/knight/.../Auto_Cleaner_APPM_rspb
set -euo pipefail

APP=Auto_Cleaner_APPM_rspb
APP_COMM=Auto_Cleaner_AP
SERVICE=my-app

if [ "$(id -u)" != 0 ]; then
    echo "Запустите от root: sudo bash rollback.sh" >&2
    exit 1
fi

target=${1:-}
if [ -z "$target" ]; then
    cg=$(systemctl show -p ControlGroup --value "$SERVICE" 2>/dev/null || true)
    if [ -n "$cg" ]; then
        for pid in $(cat "/sys/fs/cgroup$cg/cgroup.procs" 2>/dev/null); do
            exe=$(readlink "/proc/$pid/exe") || continue
            exe=${exe% (deleted)}
            case $exe in */bash|*/sh|*/dash|*/sleep) continue ;; esac
            target=$exe
            break
        done
    fi
fi
if [ -z "$target" ]; then
    echo "Сервис $SERVICE не работает - не найти программу автозапуска." >&2
    echo "Запустите с путём: sudo bash rollback.sh /путь/к/$APP (путь - в скрипте из systemctl cat $SERVICE)" >&2
    exit 1
fi
[ -f "$target.prev" ] || { echo "Нет предыдущей версии $target.prev" >&2; exit 1; }

echo "Программа автозапуска: $target"
read -r -p "Вернуть предыдущую версию? [y/N] " answer
[[ $answer == [yYдД]* ]] || { echo "Отменено"; exit 1; }

systemctl stop "$SERVICE"
pkill -x "$APP_COMM" || true
mv -f "$target" "$target.tmp"
mv -f "$target.prev" "$target"
mv -f "$target.tmp" "$target.prev"
sync
systemctl start "$SERVICE"
sleep 5
systemctl is-active --quiet "$SERVICE" && echo "Готово: запущена предыдущая версия" \
    || { echo "Сервис $SERVICE не запустился" >&2; systemctl status "$SERVICE" --no-pager | head -15 >&2; exit 1; }
