#!/bin/sh
echo "hello from docker"

# Now in the docker container, install some packages that will be useful for the system and to work on the rootkit from the image
apk add openrc
apk add util-linux
apk add build-base

# other setup for the image (root pw, serial port for qemu to communicate) 
ln -s agetty /etc/init.d/agetty.ttyS0
echo ttyS0 > /etc/securetty
rc-update add agetty.ttyS0 default
rc-update add root default

#create user and set password
echo -e "root\nroot" | passwd root
mkdir -p /home/user
adduser -D user
echo -e "user\nuser" | passwd user

rc-update add devfs boot
rc-update add procfs boot
rc-update add sysfs boot

#set network
echo "Alpine-2600" > /etc/hostname
echo -e "auto eth0\niface eth0 inet dhcp" | tee /etc/network/interfaces
/etc/init.d/networking start
rc-update add networking boot

# copy all system files from alpine to our image
for d in bin etc lib root sbin usr home; do tar c "/$d" | tar x -C /my-rootfs; done
for dir in dev proc run sys var; do mkdir /my-rootfs/${dir}; done

# exit the container, we got everything we needed
echo "Alpine configuration done"
exit
