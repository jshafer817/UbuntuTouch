Ubuntu Touch for the HP Touchpad
Saucy from 08-29-2013.2

Android sources are from Milaq WITHOUT JCSullins Bluetooth fixes.

Download Ubuntu Touch, replace the folders from the "Source to compile" Folder.
It *should compile* as of 09-01-2013.

RamDisk Information
The Touchpad uses LVM partitions. I had to compile e2label and lvm for the ramdisk to detect our partitions. I edited the touch script. Added udev rules, but removed dm-* from 60-persistent-storage.rules to get UUID's and Labels for the Data Partition to be DATAFS on every boot, no matter what.

Kernel Information:
Took the kernel from Milaq and changed the config file. Added all the Ubuntu Touch stuff, especially CGROUPS and CONFIG_TMPFS_POSIX=y.. I added a patch I found that makes the kernel automatically mount /sys/fs/cgroups as this seems to be a requirement.

System Partition.
1. Add ts_srv, make it executable
2. Create /vendor/etc/audio_effects, and the vendor directory, as it is missing
3. Imported files from Milaq's CM10.1 Rom /system/lib/hw

Data Partition
Add the files from Important Files After Compiling
1. I created the fstab
2. Told lxc-android-boot not to run update-fstab
3. Created udev rules for lvm that match the ramdisk
4. Created the 70-tenderloin.rules for lvm
5. Created system and vendor mount points just in case for fstab mounting
6. Created init.rc and init.tenderloin for LXC Overrides. Added the insmod ath6kl.ko for wifi, and told the Sensor Service to start on main. The last part may not be necessary?
7. /etc/init/ubuntu-touch-session-setup.conf has sleep 60 to start the session a minute later.
8. ubuntu-touch-session.conf has the display settings set at:
  GRID_UNIT_PX=8
  QTWEBKIT_DPR=1.0
9. Used audiod.conf and bcattach.conf from Ubuntu non touch projects.. from.. various people!

