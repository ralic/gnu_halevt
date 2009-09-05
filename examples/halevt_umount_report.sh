#! /bin/sh

tmpfile=$1
device=$2

if [ z"$device" = 'z' ]; then
  echo 'No device' > "$tmpfile"
  exit 1
fi

message=`halevt-umount -d "$device" 2>&1`
status=$?

if [ z"$status" = z0 ]; then
  echo 'Completed' > "$tmpfile"
  exit 0
fi

echo "$message" > "$tmpfile"
exit 1
