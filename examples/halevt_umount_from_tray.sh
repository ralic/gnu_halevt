#! /bin/sh

device=$1
label=$2

if [ z"$device" = z ]; then
 xmessage -title "error: missing device" -timeout 20 "Missing device to unmount";
 exit 1
fi

[ z"$label" = 'z' ] && label=$device

while true; do
  xmessage -title "$label" -buttons "umount $label:2" "Click to unmount"
  if [ $? != 2 ]; then
    exit 1
  fi

  error_msg=`halevt-umount -d "$device" 2>&1`
  status=$?

  if [ z"$status" = z0 ]; then
    exit 0
  fi
  xmessage "$error_msg"
done
  
