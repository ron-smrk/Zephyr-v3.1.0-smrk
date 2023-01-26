#!/bin/bash
VER=1."`cat BuildNumber.txt`"

BOOTLOADER=../build-mcuboot/zephyr/zephyr.hex
APP=build/zephyr/zephyr.signed.hex
RELDIR=Flash-${VER}
FCMD=FlashCMD.sh

if [ -d "$RELDIR" ] ; then
    read -p "${RELDIR} exists, remove it? " yn
    if [ "$yn" != "y" ] ; then
        echo "Dir not removed, exiting..."
        exit 1
    fi
    /bin/rm -rf "$RELDIR"
fi

mkdir "$RELDIR"
cp "$BOOTLOADER"  "$RELDIR"
cp "$APP"  "$RELDIR"
cp SMRK-Release/"$FCMD"  "$RELDIR"
sed "s/++VER++/$VER/" < SMRK-Release/InitialFlash.template > "$RELDIR"/InitialFlash.txt
cp -r boards/arm/smrk100g/support "$RELDIR"

TARFILE="FullRel-"${VER}".tgz"
tar -czf "$TARFILE" "$RELDIR"
