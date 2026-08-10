rsync -avzt --progress --exclude 'build-Auto_Cleaner_APPM_rspb-rasberry-Debug/' \
  /home/helen/Projects/Auto_Cleaner_402/Auto_Cleaner_APPM_rspb/ \
  knight@raspberrypi.local:/home/knight/work/Auto_Cleaner_318D4/

d=$(date +%s)
ssh knight@raspberrypi.local "sudo date -s @${d}"

ssh knight@raspberrypi.local '
cd /home/knight/work/Auto_Cleaner_318D4/ &&
sudo killall Auto_Cleaner_APPM_rspb || true &&
make clean || true &&
rm -f ./*.o &&
rm -f ./Auto_Cleaner_APPM_rspb &&
qmake &&
make -j4
'

ssh knight@raspberrypi.local '
cd /home/knight/work/Auto_Cleaner_318D4/ &&
sudo ./Auto_Cleaner_APPM_rspb -platform eglfs
'
