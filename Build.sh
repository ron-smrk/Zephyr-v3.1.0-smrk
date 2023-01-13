#!/bin/bash

VER=1."`cat BuildNumber.txt`"
V=0
B=0
R=0
WORK=0

# -b - build
# -h - help
# -r - release
# -V - version
# -v - verbose

while getopts 'brvV:' opt; do
    case "$opt" in
        b)              # build
            B=1
            WORK=1
            ;;
        r)              # Release, force build first..
            R=1
            B=1
            WORK=1
            ;;
        v)
            V=1
            ;;
        V)
            VER=${OPTARG}
            ;;
        ?|h)
            echo "Usage $(basename $0) [-b] [-v] [-V Version]"
            exit 1
            ;;
    esac
done
shift "$((OPTIND -1))"

if [ "$WORK" -eq 0 ] ; then
    echo $(basename $0)": Nothing to do"
    exit 1
fi

#echo "Version="$VER
#echo "verbose="$V

if [ "$B" -eq 1 ] ; then
    if [ "$V" -eq 1 ] ; then
        set -x
    fi
    west build -d build2 -b smrk100g -s MyWork/smp_svr
    ../bootloader/mcuboot/scripts/imgtool.py sign --key ../bootloader/mcuboot/smrk-key.pem --header-size=0x200 --align 8  --version "$VER" --slot-size 0x20000 --pad build2/zephyr/zephyr.bin signed-"$VER".bin
    set +x
fi

sed "s/++VER++/$VER/" < How2Flash.template > How2Flash.txt
FILELIST="signed-${VER}.bin How2Flash.txt"
MCUMGR=`which mcumgr`
MDIR=`dirname $MCUMGR`
#echo $FILELIST $MDIR
TARFILE="Rel-"${VER}".tgz"
if [ "$R" -eq 1 ] ; then
    if [ "$V" -eq 1 ] ; then
        set -x
    fi
    tar -cf "$TARFILE" $FILELIST
    tar -C "$MDIR" -rf "$TARFILE" mcumgr
    set +x

fi
/bin/rm -f How2Flash.txt
