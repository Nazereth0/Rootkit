#!/bin/bash
set -eu

if [ $(pwd | rev | cut -d "/" -f 1 | rev) != "rootfs_builder" ]; then
	echo "Please run the script in the directory 'rootfs_builder'"
	exit
fi

# Create a minimal linux system based on Alpine Linux

# We saw how to do it from scratch (initramfs and busybox) but then there is a lot of additional work to make it usable (we don't have users or a lot of other things)
# Instead we will use docker to get a minimal alpine linux image and then copy all files from this image to our image so that we can run it on qemu

MNT_PATH=/tmp/my-rootfs
LINUX_PATH=${1} #/home/user/Documents/cours/kernel/linux-5.15.131
DOCKER_NAME=rootkit_docker

# create a build dir
SCRIPTS_DIR=$(pwd)
#echo "script dir ${SCRIPTS_DIR}"
#cd $SCRIPTS_DIR


if [ -d "${SCRIPTS_DIR}/buildfs" ]; then
	rm -r buildfs
	echo "rm buildfs"
fi

echo "before before"
docker_up=$(docker ps --all | grep "Up.*${DOCKER_NAME}" | wc -l)
echo "before stop docker"
if ((docker_up > 0))
then
    docker stop ${DOCKER_NAME}
    echo "stop rootkit_docker"
fi

echo "before before rm"
docker_exist=$(docker ps --all | grep "Exited.*${DOCKER_NAME}" | wc -l)
echo "before rm docker"
if ((docker_exist > 0)); then
    docker rm ${DOCKER_NAME}
    echo "rm rootkit_docker"
fi

mnt_count=$(df | grep "${MNT_PATH}" | wc -l)
if ((mnt_count > 0)); then
    sudo umount "${MNT_PATH}"
    echo "umount /tmp/my-rootfs"    
fi

losetup_count=$(losetup -l | grep "${SCRIPTS_DIR}/buildfs/disk.img" | wc -l)
if ((losetup_count > 0)); then
    loop=$(losetup -l | grep "${SCRIPTS_DIR}/buildfs/disk.img" | cut -d" " -f1)
    sudo losetup -d ${loop}
fi

mkdir -p ${SCRIPTS_DIR}/buildfs && cd ${SCRIPTS_DIR}/buildfs

# build an empty image
truncate -s 450M disk.img
/sbin/parted -s ./disk.img mktable msdos
/sbin/parted -s ./disk.img mkpart primary ext4 1 "100%"
/sbin/parted -s ./disk.img set 1 boot on

# create a loopback device
LO_DEVICE=$(sudo losetup --show -Pf disk.img) 
#format it in ext4
sudo mkfs.ext4 ${LO_DEVICE}p1

# mount the image and run the docker container
mkdir -p ${MNT_PATH}
sudo mount ${LO_DEVICE}p1 ${MNT_PATH}

docker run -it -d --name rootkit_docker -v /tmp/my-rootfs:/my-rootfs alpine
docker cp ../alpine_config.sh rootkit_docker:/tmp/alpine_config.sh
docker exec rootkit_docker /tmp/alpine_config.sh

docker stop rootkit_docker
docker rm rootkit_docker
# And now let's install grub on the image (we wanna use bios and not uefi, so we need the file /usr/lib/grub/i386-pc for some reason on debian you can't install grub-pc if you already have grub uefi, but you can install apt install grub-pc-bin instead of the parent package so theres no problem). then: 
sudo mkdir -p ${MNT_PATH}/boot/grub
# kernel compiled with defconfig
sudo cp ${LINUX_PATH}/arch/x86_64/boot/bzImage ${MNT_PATH}/boot/vmlinuz

# make a grub config for your minimal system
#sudo cat >> ${MNT_PATH}/boot/grub/grub.cfg <<END
#serial
#terminal_input serial
#terminal_output serial
#set root=(hd0,1)
#
#menuentry "Linux2600" {
#    linux /boot/vmlinuz root=/dev/sda1 console=ttyS0
#}
#END
echo -e "serial\nterminal_input serial\nterminal_output serial\nset root=(hd0,1)\n\nmenuentry "Linux2600" {\n\tlinux /boot/vmlinuz root=/dev/sda1 console=ttyS0\n}" | sudo tee -a ${MNT_PATH}/boot/grub/grub.cfg


sudo grub-install --directory=/usr/lib/grub/i386-pc --boot-directory=${MNT_PATH}/boot ${LO_DEVICE}


# now we're done, unmount and disable loop device
sudo umount ${MNT_PATH} && rmdir ${MNT_PATH} && true # sometimes it's impossible to unmount correctly idk why... you can change MNT_PATH if that happens and it will be deleted at next reboot
sudo losetup -d $LO_DEVICE

# and finally start your new system :)
#qemu-system-x86_64 -hda disk.img -nographic
