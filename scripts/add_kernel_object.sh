#!/bin/bash

if [ "$1" = "" ] || [ "$2" = "" ] || [ "$1" = "-h" ]; then
	echo "Usage : add_kernel_object.sh disk.img kernel_object.ko"	
else
	#Get full path of disk.img
	disk_path=$(readlink -f $1)
	
	#Create loopback device
	sudo losetup -Pf $1

	#Create mount directory
	mkdir -p /tmp/my-rootfs
	
	loop_num=$(losetup -l | grep $disk_path | cut -d" " -f1)
	part_num="p1"

	#Mount disk
	sudo mount $loop_num$part_num /tmp/my-rootfs
	
	#Copy .ko in the disk
	sudo cp $2 /tmp/my-rootfs/root/

	#Umount disk
	sudo umount /tmp/my-rootfs 
	
	#Detach loopback device
	sudo losetup -d $loop_num
fi
