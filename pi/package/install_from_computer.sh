#!/bin/bash
# Установка этого пакета на Pi машины с любого компьютера (Linux, macOS; на Windows - см. ИНСТРУКЦИЯ.txt).
# Нужны только ssh и scp, репозиторий и сборка не нужны. Запуск из распакованной папки пакета:
#     bash install_from_computer.sh АДРЕС
# АДРЕС - IP (192.168.68.140), сетевое имя Pi (appm-012.local) или имя из ~/.ssh/config; пользователь по умолчанию knight.
# Пароль Pi спрашивается один раз (если нет входа по ключу), sudo на Pi может спросить его ещё раз.
set -euo pipefail

PI=${1:-}
if [ -z "$PI" ]; then
    echo "Укажите адрес Pi: bash install_from_computer.sh 192.168.68.140" >&2
    exit 1
fi
[[ $PI == *@* ]] || PI=knight@$PI
HERE=$(cd "$(dirname "$0")" && pwd)
NAME=$(basename "$HERE")
REMOTE_DIR=/home/knight/updates
SERVICE=my-app

[ -f "$HERE/install.sh" ] && [ -f "$HERE/Auto_Cleaner_APPM_rspb" ] || {
    echo "Запускайте из распакованной папки пакета" >&2; exit 1; }

# одно соединение на всю установку - пароль спрашивается один раз
SOCKET_DIR=$(mktemp -d)
trap 'ssh -o ControlPath="$SOCKET_DIR/s" -O exit "$PI" 2>/dev/null || true; rm -rf "$SOCKET_DIR"' EXIT
SSH_OPTS=(-o ControlMaster=auto -o ControlPath="$SOCKET_DIR/s" -o ControlPersist=300 -o ConnectTimeout=10)

cat "$HERE/ВЕРСИЯ.txt" 2>/dev/null || true
echo "== Подключаюсь к $PI"
ssh "${SSH_OPTS[@]}" "$PI" "mkdir -p $REMOTE_DIR && rm -rf '$REMOTE_DIR/$NAME'"

echo "== Копирую пакет"
scp "${SSH_OPTS[@]}" -q -r "$HERE" "$PI:$REMOTE_DIR/"

echo "== Выставляю время на Pi (у неё нет своих часов)"
ssh "${SSH_OPTS[@]}" -t "$PI" "sudo date -s @$(date +%s) >/dev/null && date" || echo "Время выставить не удалось - продолжаю"

echo "== Устанавливаю"
ssh "${SSH_OPTS[@]}" -t "$PI" "cd '$REMOTE_DIR/$NAME' && sudo bash install.sh"

echo "== Лог программы (последние строки)"
sleep 3
ssh "${SSH_OPTS[@]}" "$PI" "journalctl -u $SERVICE -n 20 --no-pager" || true
echo
echo "Если новая версия работает плохо, вернуть прежнюю:"
echo "    ssh -t $PI sudo bash $REMOTE_DIR/$NAME/rollback.sh"
