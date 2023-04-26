
https://docs.zephyrproject.org/latest/develop/getting_started/index.html


### Create zephyr tree from git

```bash

mkdir Zephy
cd Zephy
git clone git@github.com:ron-smrk/Zephyr-v3.1.0-smrk.git
west init -l Zephyr-v3.1.0-smrk
west update
west zephyr-export
pip3 install --user -r zephyr/scripts/requirements.txt
```

### add git hook to update version at checkin time.
```bash
cp SMRKConfig/pre-commit .git/hooks
cp SMRKConfig/pre-push .git/hooks
# make sure they are executable.
```

##Setup:
### get mcumgr command
```bash
go get github.com/apache/mynewt-mcumgr-cli/mcumgr
```

## setup comms port for mucmgr
```bash
mcumgr conn add usb2  type="serial" connstring="dev=/dev/ttyUSB2,baud=115200,mtu=512"
```

## Setup Toolchain
```bash
cd ~
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.14.2/zephyr-sdk-0.14.2_linux-x86_64.tar.gz
wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.14.2/sha256.sum | shasum --check --ignore-missing
tar xvf zephyr-sdk-0.14.2_linux-x86_64.tar.gz
(I used /opt)
cd /opt/zephyr-sdk-0.14.2
./setup.sh
sudo cp ~/zephyr-sdk-0.14.2/sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d
sudo udevadm control --reload
```


## MCUBOOT
```bash

From directory: /exports/share2/RonWork/Xilinx/STM/ZEPHYR-3.1
ln -s Zephyr-v3.1.0-smrk zephyr

1) Build MCUBOOT
**building with new key**
**In ZEPHYR-3.1/bootloader/mcuboot:**
./scripts/imgtool.py  keygen -k smrk-key.pem -t rsa-2048
NOTE: key has been created once, dont do this step anymore,
## instead cp smrk-key.pem from zephyr/smrk-key.pem to bootloader/mcuboot,
## but menuconfig does need to be done
cp zephyr/smrk-key.pem bootloader/mcuboot/
#In ZEPHYR-3.1:
west build -b smrk100g -t menuconfig -d build-mcuboot bootloader/mcuboot/boot/zephyr/
search (/) for BOOT_SIGNATURE_KEY_FILE, change to 'smrk-key.pem'
################
- in ZEPHYR-3.1 (up one from main zephy directory)
west build -b smrk100g -s bootloader/mcuboot/boot/zephyr -d build-mcuboot
west flash -d build-mcuboot

2) Build Initial App
cd zephyr
west build -b smrk100g MyWork/newshell
west sign -t imgtool -- --key ../bootloader/mcuboot/smrk-key.pem 
west flash --hex-file build/zephyr/zephyr.signed.hex

3) Build updated App
   
west build -d build2 -b smrk100g -s MyWork/newshell
../bootloader/mcuboot/scripts/imgtool.py sign --key ../bootloader/mcuboot/smrk-key.pem --header-size=0x200 --align 8  --version 1.3 --slot-size 0x20000 --pad build2/zephyr/zephyr.bin signed-new.bin

4) uploading:
mcumgr -c usb2 image list
mcumgr -c usb2 image upload signed-new.bin
mcumgr -c usb2 image list
mcumgr -c usb2 image test 1c4f902ab37ce912f6986a0db5615a4738835ed99766a8f1bdb987bb1c2197bb
mcumgr -c usb2 reset
mcumgr -c usb2 image list
mcumgr -c usb2 image confirm 1c4f902ab37ce912f6986a0db5615a4738835ed99766a8f1bdb987bb1c2197bb
```
```bash
./SMRK-Release/Package.sh 
 - used to create initial flash image. flashes in bootloader and app
./SMRK-Release/Build.sh -r
 - Buids latest app, and packages it along with mcumgr binary and instructions for updating
```
## FTDI serialize
```bash
- add/change serial number
progftdi -s SMRK-99999
- display serial
ftx_prog --dump --verbose --ignore-crc-error
```




### OLD Misc commands:

<s>
### OLD Misc commands:

west build -d build2 -b smrk100g -s MyWork/shell_module/  -- -DCONFIG_BOOTLOADER_MCUBOOT=y
../bootloader/mcuboot/scripts/imgtool.py sign --key ../bootloader/mcuboot/root-rsa-2048.pem --header-size=0x200 --align 8  --version 1.3 --slot-size 0x20000 --pad build2/zephyr/zephyr.bin signed-new.bin

pyocd flash -a 0x8040000 -t STM32F412xE signed-new.bin
***Sometimes run 'west flash if above command fails...????****


Generate our own keys
cd zephyr
Generate key (1 time):
../bootloader/mcuboot/scripts/imgtool.py keygen -k smrk-key.pem -t rsa-2048
cp smrk-key.pem ../bootloader/mcuboot/
west build -b smrk100g -t menuconfig -d build_mcuboot ../bootloader/mcuboot/boot/zephyr
 - / BOOT_SIGNATURE_KEY_FILE (search for...)
 - change to smrk-key.pem


# build and install mcuboot
west build -b smrk100g -d build_mcuboot ../bootloader/mcuboot/boot/zephyr
west flash -d build_mcuboot

#build app.
west build -p -b smrk100g samples/subsys/mgmt/mcumgr/smp_svr -- -DOVERLAY_CONFIG='overlay-serial.conf;overlay-fs.conf;overlay-shell-mgmt.conf'
west sign -t imgtool -- --key smrk-key.pem
west flash --bin-file build/zephyr/zephyr.signed.bin

---
</s>

```bash
_____________
JTAG Info

STM Board Connection:

1 - N/C     (VCC)
2 - White   (JTCK/SWCLK)
3 - Black   (GND)
4 - Purple  (JTMS/SWDIO)
5 - Gray    (RESET)
6 - Brown   (SWO)

6 pin to JTAG connector
(Looking a bottom)
  2  1
  4  3
  6  5
1 - Blue
2 - Purple
3 - Gray
4 - White
5 - Black
6 - Brown
STM ST-LINK-V2 to jtag.
 RESET - 1   2 - SWDIO
 GND   - 3   4 - GND
 SWIM  - 5   6 - SWCLK
 +3.3V - 7   8 - +3.3V
 +5V   - 9  10 - +5V

TC2030-IDC

PC Plug            6 Pin Connector (Bottom View)

x 6 4 2            1 3 5
        x
x 5 3 1            2 4 6



J1 (on smrk Board) (TC2030)
1 - VCC (N/C)
2 - SWDIO
3 - RESET
4 - SWCLK
5 - GND
6 - TDO (SWO)


JTAG     Devel Board
SWO   <--> SWO
RESET <--> RESET
SWDIO <--> SWDIO
GND   <--> GND
SWCLK <--> SWCLK
VCC(N/C)

------------
Historical now use progftdi script.
Init FTD
/home/ron/Work/Xilinx/ftx-prog
 ./ftx_prog --dump --verbose --ignore-crc-error
 ./ftx_prog --dump  --ignore-crc-error	# write it
/exports/share2/RonWork/Xilinx/tools/Vivado_Lab/2022.1/Vivado_Lab/2022.1/bin
-> ./program_ftdi -write   -ftdi FT4232H -vendor "SMRK Labs" -b "smrk100g"  -d "100G" -serial 0abc02

```
