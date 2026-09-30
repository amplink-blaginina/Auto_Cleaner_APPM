#!/bin/bash
# Деплой и запуск Auto_Cleaner на Raspberry Pi.

SRC=/home/helen/Projects/Auto_Cleaner_402/Auto_Cleaner_APPM_rspb/
DST_DIR=/home/knight/work/Auto_Cleaner_318D4
DST=knight@192.168.68.155:$DST_DIR/
APP=Auto_Cleaner_APPM_rspb
APP_COMM=Auto_Cleaner_AP          # /proc/<pid>/comm хранит только 15 символов

set -e

# 1. Синхронизация исходников (без артефактов сборки)
rsync -avz --progress \
  --exclude 'build*/' \
  --exclude '*.o' \
  --exclude 'moc_*' \
  --exclude 'ui_*.h' \
  --exclude 'Makefile*' \
  --exclude '.qmake.stash' \
  --exclude "$APP" \
  "$SRC" "$DST"
# Когда убедитесь, что пути верны, можно добавить --delete.

# 2. Синхронизация времени (ssh -t — sudo сможет спросить пароль; date — контроль)
ssh -t knight@192.168.68.155 "sudo date -s @$(date +%s) && date"

# 3. Страховка от "modification time in the future".
# sudo нужен: приложение работает под root и создаёт root-owned файлы
# (settingsAutoCleaner.ini и др.) — от knight их не потрогать.
ssh knight@192.168.68.155 "sudo find $DST_DIR -exec touch {} +"

# 4. Сборка
ssh knight@192.168.68.155 "
set -e
cd $DST_DIR/

sudo pkill -x $APP_COMM || true
sleep 1
pgrep -ax $APP_COMM && echo 'ВНИМАНИЕ: старый экземпляр ещё жив!' || true

rm -f ./*.o ./$APP
qmake
make -j1 2>&1 | tee /home/knight/Auto_Cleaner_build.log
test -x ./$APP
"

# 5. Запуск
ssh knight@192.168.68.155 "
sudo pkill -x $APP_COMM || true
sleep 1
cd $DST_DIR/ &&
sudo ./$APP -platform eglfs
"
