SUPPORT=./support
OPENOCD=/opt/zephyr-sdk-0.14.2/sysroots/x86_64-pokysdk-linux/usr/share/openocd
CFG="${SUPPORT}/openocd.cfg"

if [ ! -e "$OPENOCD" ] ; then
    echo "Error: ${OPENOCD} Does not exist, Please install Zephyr SDK."
    exit 1
fi

echo "Writing Bootloader"
# Bootloader
openocd -s "$SUPPORT" -s "$OPENOCD" -f "$CFG" -c init -c targets -c "reset halt" -c "flash write_image erase zephyr.hex" -c "reset halt" -c "verify_image zephyr.hex" -c "reset run" -c "shutdown"

echo "Writing App"
# App
openocd -s "$SUPPORT" -s "$OPENOCD" -f "$CFG" -c init -c targets -c "reset halt" -c "flash write_image erase zephyr.signed.hex" -c "reset halt" -c "verify_image zephyr.signed.hex" -c "reset run" -c "shutdown"
