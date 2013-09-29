cd /tmp
echo "Thanks to crimsonredmk"
echo "Repartition to create ext3 volume for Ubuntu."
pkill -SIGUSR1 cryptofs
pkill -SIGUSR1 LunaSysMgr
pkill -SIGUSR1 LunaDownloadMgr
pkill -SIGUSR1 cryptofs
pkill -SIGUSR1 tar
pkill -SIGUSR1 cryptofs
pkill -SIGUSR1 LunaSysMgr
pkill -SIGUSR1 LunaDownloadMgr
pkill -SIGUSR1 cryptofs
pkill -SIGUSR1 tar
umount /media/internal
echo "Did the previous line say something about not being able to umount /media/internal? If so, press 1, otherwise press 0."
echo "DO NOT PRESS 0 IF IT DID SAY SOMETHING; THIS CAN LEAD TO DATA LOSS! If it did say something, reboot and try again."
read PROBLEM
if [ $PROBLEM ==  1 ];
then echo "Goodbye."
exit
else echo "Ok, continuing."
fi
PARTITION1=300
PARTITION1SHRINK=310
PARTITION2=1500
PARTITION2SHRINK=1550
PARTITION3=200
PARTITION3SHRINK=210
echo "Your chosen partition is ${PARTITION1}MB."
CURRENTSIZE=$(lvm.static lvdisplay -c store/media | awk -F: '{print $7/2048}')
echo "Your current media partition is ${CURRENTSIZE}MB."
NEWSIZE=$(($CURRENTSIZE - $PARTITION1))
echo "Your new partition layout will include a ${PARTITION1}MB ext3 lvm partition (ut-system) and a ${NEWSIZE}MB media partition."
echo "Does this seem correct? If so, type 1, if not, type 0."
read OK
if [ $OK ==  1 ];
then echo "Ok, continuing."
else echo "Goodbye."
exit
fi
echo "Now resizing your media partition and creating the ut-system partition for UbuntuTouch."
resizefat /dev/store/media ${NEWSIZE}M
lvresize -L -${PARTITION1SHRINK}M /dev/store/media 
lvcreate -L ${PARTITION1}M -n ut-system store
echo "Your chosen partition is ${PARTITION2}MB."
CURRENTSIZE=$(lvm.static lvdisplay -c store/media | awk -F: '{print $7/2048}')
echo "Your current media partition is ${CURRENTSIZE}MB."
NEWSIZE=$(($CURRENTSIZE - $PARTITION2))
echo "Your new partition layout will include a ${PARTITION2}MB ext3 lvm partition (ut-data) and a ${NEWSIZE}MB media partition."
echo "Does this seem correct? If so, type 1, if not, type 0."
read OK
if [ $OK ==  1 ];
then echo "Ok, continuing."
else echo "Goodbye."
exit
fi
echo "Now resizing your media partition and creating the ut-data partition for UbuntuTouch."
resizefat /dev/store/media ${NEWSIZE}M
lvresize -L -${PARTITION2SHRINK}M /dev/store/media
lvcreate -L ${PARTITION2}M -n ut-data store
echo "Your chosen partition is ${PARTITION3}MB."
CURRENTSIZE=$(lvm.static lvdisplay -c store/media | awk -F: '{print $7/2048}')
echo "Your current media partition is ${CURRENTSIZE}MB."
NEWSIZE=$(($CURRENTSIZE - $PARTITION3))
echo "Your new partition layout will include a ${PARTITION3}MB ext3 lvm partition (ut-cache) and a ${NEWSIZE}MB media partition."
echo "Does this seem correct? If so, type 1, if not, type 0."
read OK
if [ $OK ==  1 ];
then echo "Ok, continuing."
else echo "Goodbye."
exit
fi
echo "Now resizing your media partition and creating the ut-cache partition for UbuntuTouch."
resizefat /dev/store/media ${NEWSIZE}M
lvresize -L -${PARTITION3SHRINK}M /dev/store/media
lvcreate -L ${PARTITION3}M -n ut-cache store
sync
mkfs.ext4 /dev/store/ut-data
mkfs.ext3 /dev/store/ut-system
mkfs.ext3 /dev/store/ut-cache


