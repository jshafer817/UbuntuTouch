Ubuntu Touch for the HP Touchpad<br>
Saucy from 09-11.1-2013 with all updates including unity from 09-11-2015 minus lxc<br>

Android sources are from Milaq WITHOUT JCSullins Bluetooth fixes.<br>
Changed LIFTOFF_TIMEOUT in ts_srv.c<br>
Changed cameraHAL.cpp<br>
Changed main.mk for Ubuntu Touch to include /device/hp/tenderloin and vendor<br>
Included tinyalsa patch from jcsullins<br>
etc.<br>

Download Ubuntu Touch, replace the folders from the "Source to compile" Folder.<br>
It *should compile* as of 09-01-2013.<br>

RamDisk Information:<br>
The Touchpad uses LVM partitions. I had to compile e2label and lvm for the ramdisk to detect our partitions. I edited the touch script. Added udev rules, but removed dm-* from 60-persistent-storage.rules to get UUID's and Labels for the Data Partition to be DATAFS on every boot, no matter what.

Kernel Information:<br>
Took the kernel from Milaq and changed the config file. Added all the Ubuntu Touch stuff, especially CGROUPS and CONFIG_TMPFS_POSIX=y and CONFIG_FAIR_GROUP_SCHED=y CONFIG_RT_GROUP_SCHED=y..<br>
I added a patch I found that makes the kernel automatically mount /sys/fs/cgroups as this seems to be a requirement.<br>
http://lists.linux-foundation.org/pipermail/containers/2010-July/024998.html<br>
Used board-tenderloin.c from Milaq's cm10.2 or 10.1 for touchscreen rotation to be correct.
accept4 system call was going to be added, but instead we recompile udev and pulseaudio with patches!!<br>

Milaq Source Info:
Changed cameraHAL.cpp to be FRONT_FACING, just in case. Added mirroring.

System Partition:<br>
1. Create /vendor/etc/audio_effects, and the vendor directory, as it is missing<br>

Data Partition<br>
Add the files from Important Files After Compiling<br>
1. I created the fstab<br>
2. Told lxc-android-boot not to run update-fstab<br>
3. Created udev rules for lvm that match the ramdisk, changed 60-persistent-storage.rules as well for dm<br>
4. Created the 70-tenderloin.rules<br>
5. Created system, vendor, webos and sdcard mount points just in case for fstab mounting<br>
6. Created init.rc and init.tenderloin for LXC Overrides. Added the insmod ath6kl.ko for wifi, and told the Sensor Service to start on main. The last part may not be necessary?<br>
7. /etc/init/ubuntu-touch-session-setup.conf and lightdm.conf has sleep 30 to start the session a minute later.<br>
8. /etc/ubuntu-touch-session.d/tenderloin.conf<br>
9. Used audiod.conf and bcattach.conf from Ubuntu non touch projects.. from.. various people!<br>
10. Changed the camera-app.qml, ubuntu-terminal-app.qml, and ubuntu-terminal-app.desktop files.<br>
11. Added /usr/bin/aa-strip .desktop files of aa-exec settings for apparmor, and /etc/crontab to schedule it to run every 1 minute.<br>
12. After boot we install:<br>
a) apt-get update<br>
b) apt-get install --reinstall openssh-server<br>
c) install the debs in the /debs folder<br>
d) apt-get install -f<br>


To create a new RootFS?<br>
1. run on device: <br>
1a. cd /data/ubuntu<br>
1b. tar -zcvf "/sdcard/saucy-preinstalled-touch-armhf.tar.gz" *<br>
2. pull new tar file you created and untar on pc into the folder you have your RootFS in.<br>
3. tar backup as saucy-preinstalled-touch-armhf.tar.gz, add to zip<br>
3a. sudo tar -zcvf ../saucy-preinstalled-touch-armhf.tar.gz *<br>
3b. chown justin:justin tar.gz file we created<br>


Sound Information:<br>
Sound has an upstart called audiod.conf. You will notice it mounts webos and runs a program to initialize the sound. /usr/share/alsa/ucm should only have msm-audio the rest of the /usr/share/alsa/ucm file that I included is probably not important. default.pa for pulse has 1 line uncommened referring to alsa-sink.
Patched udev and pulse<br>

Bluetooth Information:<br>
Look at hcattach.conf in /etc/init for an upstart job. We bought over hcattach_awesome and another file.<br>

Wifi Information:<br>
ath6kl.ko needs to be insmodded. That is set in init.tenderloin.rc in overrides for the lxc containter.<br>

Camera Information:<br>
Had to edit the camera-app.qml file to correct the 90 degree camera sensor in landscape mode.<br>

How to compile notes?<br>
1.Create mkimage and put it in it's place<br>
a)cd system/extras/mkimage<br>
b. gcc mkimage.c -o mkimage -lz<br>
c) cd ../../..<br>
d) mkdir -p out/host/linux-x86/bin<br>
e) cp system/extras/mkimage/mkimage out/host/linux-x86/bin<br>
f) brunch tenderloin<br>
***Look at project.list, roomservice.xml, manifest.xml, main.mk, Makefile?<br>

http://forum.xda-developers.com/showthread.php?t=2426924<br>
Thanks to:<br>
Ogra, castrwilliam, CalcProgrammer, Mystikal57, JCSullins, Dorregray, w-flo.. and others from the #ubuntu-touch irc channel and Ubuntu 11.04-13.10 threads, cyanogen roms, solutions. etc... "I stand on the shoulders of giants"<br>

-Justin Shafer<br>
aka OrokuSaki aka jshafer817<br>
