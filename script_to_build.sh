rsync -avzt --progress --exclude 'build-Auto_Cleaner_APPM_rspb-rasberry-Debug/' /home/knight/projects/Machines/Auto_Cleaner_APPM_rspb/ knight@192.168.0.77:/home/knight/work/Auto_Cleaner_318D4/
#ssh knight@192.168.0.77 "cd /home/knight/work/Auto_Cleaner_318D4/;rm -f ./Auto_Cleaner_318D4;qmake;make; ./Auto_Cleaner_318D4 -platform eglfs"
d=`date +%s`
ssh knight@192.168.0.77 "sudo date -s @\"${d}\""
ssh knight@192.168.0.77 "cd /home/knight/work/Auto_Cleaner_318D4/;sudo rm ./Auto_Cleaner_APPM_rspb;qmake;make -j4; sudo ./Auto_Cleaner_APPM_rspb -platform eglfs"
ssh knight@192.168.0.77 "sudo killall Auto_Cleaner_APPM_rspb"
