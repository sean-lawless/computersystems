# Appendix A

Using SiPeed low cost USB to UART/JTAG adapter.

### Install MSYS2 and launch the "MSYS2 MinGW 32-bit" shell.

[https://www.msys2.org/](https://www.msys2.org/)

### Install the latest arm-none-eabi toolchain for Windows.

[https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)

### Edit c:/msys64/etc/profile

Add the directory to the toolchain from #2 to MSYS path.

```bash
MSYS2_PATH="/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/GNU Arm Embedded Toolchain/9 2020-q2-update/bin"
```

### Install OpenOCD

```bash
pacman -S mingw-w64-i686-openocd
```

### Upgrade OpenOCD

Download the latest xpack-openocd (xpack-openocd-0.10.0-14-win32-x32.zip)

[https://github.com/xpack-dev-tools/openocd-xpack/releases/](https://github.com/xpack-dev-tools/openocd-xpack/releases/)

Then Copy 'contrib', 'OpenULINK' and 'scripts' folders to
c:/msys64/mingw32/share/openocd

Then copy contents of 'bin' folder to c:/msys64/mingw32/bin

### Alternatively to #4 and #5 you can build the latest openocd from Git.

### Connect all UART and JTAG pins from SiPeed to the RPI.

The UART Rx and Tx connect to the opposite side (Rx to Tx, Tx to Rx).
All other pins are direct connect. Do not forget GND pins. See
Chapter 4 for pin numbers and GPIO's. The pins on the SiPeed are
clearly shown.

### Connect the USB SiPeed adapter to the Windows PC.

### Change the USB driver

The USB Serial Converter A must use the WinUSB driver for OpenOCD to
control it correctly for JTAG.

Download, install and execute the UsbDriverTool

[https://visualgdb.com/UsbDriverTool/](https://visualgdb.com/UsbDriverTool/)

Right click the USB Serial Converter A and choose Install WinUSB.

### Execute OpenOCD

For example:

```bash
openocd -f ../../boards/rpi/openocd_sipeed_jtag.cfg -f ../../boards/rpi/rpi4_jtag.cfg
```

### UART
 
Search the Windows Device Manager for the USB Serial Converter B and
note the COM port number. Use this COM port number when connecting with
TeraTerm, etc. See Chapter 6 for more details.

### Happy coding and debugging!
  
