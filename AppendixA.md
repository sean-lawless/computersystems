# Appendix A

Using SiPeed low cost USB to UART/JTAG adapter.

## Install MSYS2 and launch the "MSYS2 MinGW 32-bit" shell.

  1) https://www.msys2.org/

## Install the latest arm-none-eabi toolchain for Windows.

  1) https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

## Edit c:/msys64/etc/profile

  1) Add the directory to the toolchain from #2 to MSYS path.
    a) MSYS2_PATH="/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/GNU Arm Embedded Toolchain/9 2020-q2-update/bin"

## Install OpenOCD

  1) pacman -S mingw-w64-i686-openocd

## Upgrade OpenOCD

  1) Download the latest xpack-openocd

    a) https://github.com/xpack-dev-tools/openocd-xpack/releases/

      i.) xpack-openocd-0.10.0-14-win32-x32.zip

  2) Copy contrib, OpenULINK and scripts folders to c:/msys64/mingw32/share/openocd

  3) Copy contents of bin folder to c:/msys64/mingw32/bin

## Alternatively to #4 and #5 you can build the latest openocd.

## Connect all UART and JTAG pins from SiPeed to the RPI.

The UART Rx and Tx connect to the opposite side (Rx to Tx, Tx to Rx).
All other pins are direct connect. Do not forget GND pins. See
Chapter 4 for pin numbers and GPIO's. The pins on the SiPeed are
clearly shown.

## Connect the USB SiPeed adapter to the Windows PC.

## Upgrade the USB driver for the JTAG.

  1) Download, install and execute the UsbDriverTool

    a) https://visualgdb.com/UsbDriverTool/ 

  2) Right click the USB Serial Converter A

    a) Choose Install WinUSB.

## Execute OpenOCD

  For example

  1) openocd -f ../../boards/rpi/openocd_sipeed_jtag.cfg -f ../../boards/rpi/rpi4_jtag.cfg

## UART
 
Search the Windows Device Manager for the USB Serial Converter B and
not the COM port number. Use this COM port number when connecting with
TeraTerm, etc. See Chapter 6 for more details.

## Happy coding and debugging!
  
