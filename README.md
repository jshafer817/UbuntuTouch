Ubuntu Touch for the HP Touchpad<br>
Saucy from 08-29-2013.2<br>
http://reports.qa.ubuntu.com/smokeng/saucy/image/3833/

Android sources are from Milaq WITHOUT JCSullins Bluetooth fixes.

Download Ubuntu Touch, replace the folders from the "Source to compile" Folder.<br>
It *should compile* as of 09-01-2013.

RamDisk Information:<br>
The Touchpad uses LVM partitions. I had to compile e2label and lvm for the ramdisk to detect our partitions. I edited the touch script. Added udev rules, but removed dm-* from 60-persistent-storage.rules to get UUID's and Labels for the Data Partition to be DATAFS on every boot, no matter what.

Kernel Information:<br>
Took the kernel from Milaq and changed the config file. Added all the Ubuntu Touch stuff, especially CGROUPS and CONFIG_TMPFS_POSIX=y and CONFIG_FAIR_GROUP_SCHED=y CONFIG_RT_GROUP_SCHED=y..<br>
I added a patch I found that makes the kernel automatically mount /sys/fs/cgroups as this seems to be a requirement.<br>
http://lists.linux-foundation.org/pipermail/containers/2010-July/024998.html<br>

System Partition:<br>
1. Add ts_srv, make it executable<br>
2. Create /vendor/etc/audio_effects, and the vendor directory, as it is missing<br>
3. Imported files from Milaq's CM10.1 Rom /system/lib/hw<br>

Data Partition<br>
Add the files from Important Files After Compiling<br>
1. I created the fstab<br>
2. Told lxc-android-boot not to run update-fstab<br>
3. Created udev rules for lvm that match the ramdisk<br>
4. Created the 70-tenderloin.rules for lvm<br>
5. Created system and vendor mount points just in case for fstab mounting<br>
6. Created init.rc and init.tenderloin for LXC Overrides. Added the insmod ath6kl.ko for wifi, and told the Sensor Service to start on main. The last part may not be necessary?<br>
7. /etc/init/ubuntu-touch-session-setup.conf has sleep 60 to start the session a minute later.<br>
8. ubuntu-touch-session.conf has the display settings set at:<br>
  GRID_UNIT_PX=8<br>
  QTWEBKIT_DPR=1.0<br>
9. Used audiod.conf and bcattach.conf from Ubuntu non touch projects.. from.. various people!<br>

Sound Information:<br>
Sound has an upstart called audiod.conf. You will notice it mounts webos and runs a program to initialize the sound. /usr/share/alsa/ucm should only have msm-audio the rest of the /usr/share/alsa/ucm file that I included is probably not important. default.pa for pulse has 1 line uncommened referring to alsa-sink.

Bluetooth Information:<br>
Look at hcattach.conf in /etc/init for an upstart job. We bought over hcattach_awesome and another file.

Wifi Information:<br>
ath6kl.ko needs to be insmodded. That is set in init.tenderloin.rc in overrides for the lxc containter.

Camera Information:
WIP

http://forum.xda-developers.com/showthread.php?t=2426924<br>
Thanks to:<br>
Ogra, castrwilliam, CalcProgrammer, Mystikal57, JCSullins, Dorregray, w-flo.. and others from the #ubuntu-touch irc channel and Ubuntu 11.04-13.10 threads, cyanogen roms, solutions. etc... "I stand on the shoulders of giants"<br>

-Justin Shafer<br>
aka OrokuSaki aka jshafer817
