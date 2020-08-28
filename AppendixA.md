# Appendix A

Using SiPeed low cost USB to UART/JTAG adapter. The SiPeed is a low cost
( < $10) USB to UART/JTAG adapter. It is typically packaged with
high quality female to female GPIO wires ideal for use with the RPi.
This USB device is based on the FTDI FT2232 part and is thus ideal
for use with OpenOCD and GDB.

To turn a Windows PC into a bare metal RPi development PC requires
this adapter, or a similar one, and a cross compiler for development.
This guide describes how to install and use MSYS2 and the arm-none-eabi
tool chain cross compiled for Windows to build, execute and debug
the RPi.

### Install MSYS2 and launch the "MSYS2 MinGW 32-bit" shell.

[https://www.msys2.org/](https://www.msys2.org/)

Next we install 'make'. From the launched shell, issue the following
to install make:

```bash
pacman -S make
```

### Install the latest arm-none-eabi toolchain for Windows.

[https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)

### Add path to c:/msys64/etc/profile

Add the directory to the toolchain from #2 to MSYS path.

```bash
MSYS2_PATH="/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/GNU Arm Embedded Toolchain/9 2020-q2-update/bin"
```

### Install OpenOCD

```bash
pacman -S mingw-w64-i686-openocd
```

Unfortunately this version of OpenOCD is too dated to support the RPi
hardware. Proceed to the next step to upgrade the OpenOCD.

### Upgrade OpenOCD

To avoid building OpenOCD from scratch, download the latest
xpack-openocd (xpack-openocd-0.10.0-14-win32-x32.zip)

[https://github.com/xpack-dev-tools/openocd-xpack/releases/](https://github.com/xpack-dev-tools/openocd-xpack/releases/)

Then unzip and copy the 'contrib', 'OpenULINK' and 'scripts' folders to
c:/msys64/mingw32/share/openocd. Replace/Update any existing files.

Then copy contents of 'bin' folder to c:/msys64/mingw32/bin, again
replacing any existing files.

### Alternatively to #4 and #5 you can build the latest openocd from Git.

### Connect all UART and JTAG pins from SiPeed to the RPI.

The UART Rx and Tx connect to the opposite side (Rx to Tx, Tx to Rx).
All other pins are direct connect. Do not forget GND pins. See
Chapter 4 for pin numbers and GPIO's. The pins on the SiPeed are
clearly shown.

WARNING - power off the RPi and disconnect the USB adapter before
connecting the GPIO wires. Double check that the connections are
correct before powering on.

### Connect the USB SiPeed adapter to the Windows PC.

This should produce two new devices within the Device Manager,
USB Serial Converter A and B.

### Change the USB driver

The USB Serial Converter A must be reconfigured to use the WinUSB
driver in order for OpenOCD to connect and control the JTAG correctly.
By default the FT2232 is configured by Windows to be two COM ports,
however, it is required to replace the driver for the primary UART (A)
with WinUSB.

To do this, download, install and execute the UsbDriverTool:

[https://visualgdb.com/UsbDriverTool/](https://visualgdb.com/UsbDriverTool/)

The UsbDriverTool Gui will appear, with a list of current USB devices
on your system. Right click the USB Serial Converter A and choose
'Install WinUSB'.

### Execute OpenOCD

Now it is time to test OpenOCD. For example, from the directory
Lab8 Operating System\applications\bootloader, the following command
will initialize OpenOCD for the SiPeed (FT2232) adapter and the
connected RPi 4 target:

```bash
openocd -f ../../boards/rpi/openocd_sipeed_jtag.cfg -f ../../boards/rpi/rpi4_jtag.cfg
```

With OpenOCD, the first configuration file is for the adapter and the
second file is for the target board.

### UART
 
Search the Windows Device Manager for the USB Serial Converter B and
note the COM port number. Use this COM port number when connecting with
TeraTerm, etc. See Chapter 6 for more details.

### Happy coding and debugging!
  
