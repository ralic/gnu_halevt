#! /bin/sh

device=$1
label=$2

if [ z"$device" = z ]; then
 xmessage -title "error: missing device" -timeout 20 "Missing device to unmount";
 exit 1
fi

[ z"$label" = 'z' ] && label=$device

xmessage -title "$label" -buttons "umount $label:2" "Click to unmount"
if [ $? != 2 ]; then
  exit 1
fi

halevt-umount -d "$device"
