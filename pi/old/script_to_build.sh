rsync -avzt --progress --exclude 'build-Auto_Cleaner_APPM_rspb-rasberry-Debug/' \
  /home/helen/Projects/Auto_Cleaner_402/Auto_Cleaner_APPM_rspb/ \
  knight@192.168.0.77:/home/knight/work/Auto_Cleaner_318D4/

d=$(date +%s)
ssh knight@192.168.0.77 "sudo date -s @${d}"

ssh knight@192.168.0.77 '
cd /home/knight/work/Auto_Cleaner_318D4/ &&
sudo killall Auto_Cleaner_APPM_rspb || true &&
make clean || true &&
rm -f ./*.o &&
rm -f ./Auto_Cleaner_APPM_rspb &&
qmake &&
make -j4
'

ssh knight@192.168.0.77 '
cd /home/knight/work/Auto_Cleaner_318D4/ &&
sudo ./Auto_Cleaner_APPM_rspb -platform eglfs
'
