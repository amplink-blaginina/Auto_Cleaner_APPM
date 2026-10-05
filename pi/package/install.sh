#!/bin/bash
# Установка новой версии на Raspberry Pi машины. Запускать на самой Pi из папки пакета:
#     sudo bash install.sh
# Заменяет программу, которую запускает автозапуск (сервис my-app). Старая сохраняется рядом с суффиксом .prev,
# вернуть её: sudo bash rollback.sh. Настройки (settingsAutoCleaner.ini) не трогаются.
# Путь к программе можно задать явно: sudo bash install.sh /home/knight/.../Auto_Cleaner_APPM_rspb
set -euo pipefail

APP=Auto_Cleaner_APPM_rspb
APP_COMM=Auto_Cleaner_AP   # /proc/<pid>/comm хранит только 15 символов
SERVICE=my-app
HERE=$(cd "$(dirname "$0")" && pwd)
NEW="$HERE/$APP"

if [ "$(id -u)" != 0 ]; then
    echo "Запустите от root: sudo bash install.sh" >&2
    exit 1
fi
[ -f "$NEW" ] || { echo "Рядом со скриптом нет файла $APP" >&2; exit 1; }
cat "$HERE/ВЕРСИЯ.txt" 2>/dev/null || true
echo

# 1. Подходит ли программа этой Pi (до остановки машины ничего не меняем)
case $(uname -m) in
    armv7l|aarch64) ;;
    *) echo "Процессор этой Pi ($(uname -m)) не подходит для этой сборки (нужна Pi 2 или новее). Ничего не изменено." >&2
       exit 1 ;;
esac
missing=$(ldd "$NEW" 2>&1 | grep 'not found' || true)
if [ -n "$missing" ]; then
    echo "На этой Pi не хватает библиотек, программа не запустится. Ничего не изменено:" >&2
    echo "$missing" >&2
    exit 1
fi

# 2. Какую программу запускает автозапуск
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
    echo "Не удалось найти программу автозапуска: сервис $SERVICE сейчас не работает." >&2
    echo "Посмотрите путь в скрипте из: systemctl cat $SERVICE" >&2
    echo "и запустите с ним: sudo bash install.sh /путь/к/$APP" >&2
    exit 1
fi
echo "Pi:      $(hostname), серийный номер $(tr -d '\0' < /proc/device-tree/serial-number 2>/dev/null || echo неизвестен)"
echo "Автозапуск запускает: $target"
echo "Сейчас:  $(md5sum "$target" 2>/dev/null | cut -d' ' -f1)"
echo "Новая:   $(md5sum "$NEW" | cut -d' ' -f1)"
read -r -p "Заменить? Машина на несколько секунд перезапустит программу [y/N] " answer
[[ $answer == [yYдД]* ]] || { echo "Отменено, ничего не изменено"; exit 1; }

# 3. Замена: старая -> .prev, новая на её место, перезапуск
cp "$NEW" "$target.new"
chmod +x "$target.new"
systemctl stop "$SERVICE"
pkill -x "$APP_COMM" || true   # экземпляр, запущенный вручную
[ -f "$target" ] && mv -f "$target" "$target.prev"
mv -f "$target.new" "$target"
sync
systemctl start "$SERVICE"
sleep 5
if systemctl is-active --quiet "$SERVICE"; then
    echo "Готово: новая версия запущена. Старая сохранена как $target.prev"
    echo "Вернуть старую: sudo bash rollback.sh"
else
    echo "Сервис $SERVICE не запустился! Верните старую версию: sudo bash rollback.sh" >&2
    systemctl status "$SERVICE" --no-pager | head -15 >&2
    exit 1
fi
