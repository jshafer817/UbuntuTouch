Ubuntu Touch for the HP Touchpad<br>
How to compile:<br>

Make a build directory:<br>

mkdir UTA (or whatever name you choose )<br>
cd UTA (or the name  you chose)<br>
mkdir .repo/local_manifests<br>

To initialize your local repository using the Ubuntu phablet manifest, use commands like these:<br>

phablet-dev-bootstrap [target_directory that you chose]<br>

curl -L -o .repo/local_manifests/roomservice.xml -O -L http://raw.github.com/jshafer817/UbuntuTouch/master/roomservice.xml<br>

Then to sync up:<br>

phablet-dev-bootstrap [target_directory that you chose] -c<br>
cd external/tinyalsa<br>
git pull http://review.cyanogenmod.org/CyanogenMod/android_external_tinyalsa refs/changes/46/33646/1<br>
cd ../..<br>
cd system/extras/mkimage<br>
gcc mkimage.c -o mkimage -lz<br>
cd ../../..<br>
mkdir -p out/host/linux-x86/bin<br>
cp system/extras/mkimage/mkimage out/host/linux-x86/bin<br>
gedit build/core/Makefile around line 1231, remove this $(INSTALLED_BOOTIMAGE_TARGET_ANDROID) \ (save the file, close)<br>
gedit build/core/main.mk add this to were the other device entires are: device/hp \ <br>
cd vendor/cm<br>
./get-prebuilts<br>
cd ../..<br>
brunch tenderloin<br>

-----------------------------------------------------------------<br>
Saucy from 09-11.1-2013 with all updates including unity from 09-11-2015 minus lxc<br>

RamDisk Information:<br>
The Touchpad uses LVM partitions. I had to compile e2label and lvm for the ramdisk to detect our partitions. It is added to /init.<br>

Kernel Information:<br>
Took the kernel from Milaq and changed the config file. Added all the Ubuntu Touch stuff, especially CGROUPS and CONFIG_TMPFS_POSIX=y and CONFIG_FAIR_GROUP_SCHED=y CONFIG_RT_GROUP_SCHED=y..<br>
Added the Mer Touchpad patches, including Accept4.


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


Sound Information:<br>
Sound has an upstart called audiod.conf. You will notice it mounts webos and runs a program to initialize the sound. /usr/share/alsa/ucm should only have msm-audio the rest of the /usr/share/alsa/ucm file that I included is probably not important. default.pa for pulse has 1 line uncommened referring to alsa-sink.
Patched pulse<br>

Bluetooth Information:<br>
Look at bcattach.conf in /etc/init for an upstart job. We bought over hcattach_awesome and bcattach.<br>

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
