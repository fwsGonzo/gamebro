#!/bin/bash
set -e
SERVICE=${1:-"$PWD/build/emulador"}
SERVICE_DIR=$(dirname "${SERVICE}")
OUTPUT=gamebro.iso
echo $SERVICE_DIR

# build service
pushd $SERVICE_DIR
cmake .. -DREAL_HW=ON
make -j
popd

LOCAL_DISK=temp_disk

# create grub.iso
mkdir -p $LOCAL_DISK/boot/grub
cp $SERVICE $LOCAL_DISK/boot/service
cp grub.cfg $LOCAL_DISK/boot/grub
echo "=>"
grub-mkrescue -d /usr/lib/grub/i386-pc -o $OUTPUT $LOCAL_DISK
echo "$OUTPUT constructed"
