#!/bin/bash


# Bottom Usb Test:
# Bottom Usb Mountpoint: /mnt/udisk1
# Device Path: 
#	2.0: usb1-2.3
#	3.0: usb2-1.3
#

df | grep /mnt > /tmp/.tmp
NODE=`awk '{print $1}' /tmp/.tmp` 
DEV=`awk '{print $6}' /tmp/.tmp`
BOTTOM_USB2BUS="/sys/devices/3530000.xhci/usb1/1-2/1-2.2"

echo $NODE
echo $DEV
if [ "$DEV" != "/mnt/udisk1" ];then
	echo "Bottom U-Disk not insert, Please check!!!"
else
	umount $NODE $DEV
	mount $NODE $DEV
	if [ ! -d "$BOTTOM_USB2BUS" ]; then
		echo "U-Disk3.0 is exist"
	else
		echo "U-Disk2.0 is exist"
	fi

	SPEED_KB=`fio -filename=${DEV}/mytest -direct=1 -iodepth 1 -thread  -ioengine=psync -bs=32k -size=30M -numjobs=1 -group_reporting -name=mytest | grep READ | cut -d ',' -f 2 | cut -d '=' -f 2 | cut -d 'K' -f 1`
	SPEED_MB=`expr $SPEED_KB / 1024`
	if [ ! -d "$BOTTOM_USB2BUS" ]; then
		if [ $SPEED_MB -gt 30 ]; then
        		RESULT=OK
		else
        		RESULT=NG
		fi
	else
		if [ $SPEED_MB -gt 10 ]; then
			RESULT=OK
		else
			RESULT=NG
		fi
	fi

	rm ${DEV}/mytest
	rm /tmp/.tmp
	echo "read speed test: $SPEED_MB MB/s >> $RESULT"
fi