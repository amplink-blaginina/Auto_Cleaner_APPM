# Общая часть скриптов деплоя на Raspberry Pi (подключается через source).
# Адрес Pi: первый аргумент скрипта, переменная PI или knight@192.168.68.128

PI=${1:-${PI:-knight@192.168.68.128}}
APP=Auto_Cleaner_APPM_rspb
APP_COMM=Auto_Cleaner_AP          # /proc/<pid>/comm хранит только 15 символов
SERVICE=my-app                    # systemd-сервис автозапуска на Pi (Restart=always)
CROSS_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BUILD=${BUILD:-$(cd "$CROSS_DIR/../.." && pwd)/build-rpi}

# Окружение как в start_app.sh на Pi
PI_ENV='
export QT_QPA_PLATFORM=eglfs
export QT_QPA_EGLFS_INTEGRATION=eglfs_kms
export QT_QPA_EGLFS_HIDECURSOR=1
export QT_QPA_EGLFS_ALWAYS_SET_MODE=1
export QT_QPA_EGLFS_DISABLE_INPUT=0
export QT_QPA_EVDEV_MOUSE_PARAMETERS=/dev/null
'
PI_CAN_INIT=/home/knight/can_init.sh

# Кросс-сборка (с проверкой совместимости с библиотеками Pi)
pi_build() {
    BUILD="$BUILD" "$CROSS_DIR/build_cross.sh"
}

# У Pi нет RTC - выставляем время с этой машины (ssh -t - sudo сможет спросить пароль)
pi_sync_clock() {
    ssh -t "$PI" "sudo date -s @$(date +%s) && date"
}

# Выполнить скрипт на Pi от root в терминале (Ctrl+C доходит до приложения)
pi_root_script() {
    ssh -t "$PI" "sudo bash -c $(printf %q "$1")"
}

# Бинарник, который сейчас запускает автозапуск: процесс в cgroup сервиса, который не shell-скрипт запуска.
# Можно задать явно переменной DEFAULT_BIN. Пустая строка - сервис не запущен.
pi_default_bin() {
    if [ -n "${DEFAULT_BIN:-}" ]; then
        echo "$DEFAULT_BIN"
        return
    fi
    ssh "$PI" "
        cg=\$(systemctl show -p ControlGroup --value $SERVICE)
        [ -n \"\$cg\" ] || exit 0
        for pid in \$(cat /sys/fs/cgroup\$cg/cgroup.procs 2>/dev/null); do
            exe=\$(sudo readlink /proc/\$pid/exe) || continue
            exe=\${exe% (deleted)}
            case \$exe in */bash|*/sh|*/dash|*/sleep) continue ;; esac
            echo \$exe; break
        done"
}

pi_require_default_bin() {
    DEFAULT_BIN=$(pi_default_bin)
    if [ -z "$DEFAULT_BIN" ]; then
        echo "Не удалось определить бинарник автозапуска: сервис $SERVICE не запущен." >&2
        echo "Задайте путь явно: DEFAULT_BIN=/путь/к/бинарнику $0" >&2
        exit 1
    fi
    echo "Автозапуск ($SERVICE) запускает: $DEFAULT_BIN"
}

# Сообщение с вопросом; выход, если ответ не «да»
pi_confirm() {
    read -r -p "$1 [y/N] " answer < /dev/tty
    [[ $answer == [yYдД]* ]] || { echo "Отменено"; exit 1; }
}
