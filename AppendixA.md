# Appendix A

The SiPeed UART/JTAG adapter is a user friendly (labeled pins,
probe accessible, etc.), compact and low cost USB to UART/JTAG adapter.
In my experience it was packaged with quality female to female
GPIO wires ideal for use with debugging the RPi (ie. short).
This USB device is based on the FTDI FT2232 part and allows
for debugging a remote RPi target systems utilizing OpenOCD and GDB.

To turn a Windows PC into a bare metal RPi development PC requires
this adapter, or a similar one, and a cross compiler for development.
This guide describes how to build, execute (TTL/UART) and debug
(JTAG) the RPi from a Windows PC, utilizing MSYS2, the arm-none-eabi
tool chain and OpenOCD.

An Ubuntu PC version of this guide is much simpler as everything
should work out of the box with these packages installed.

```bash
sudo apt install gcc-arm-none-eabi
sudo apt install gdb-multiarch
sudo apt install openocd
```

When debugging on Ubuntu use gdb-multiarch instead of
arm-none-eabi-gdb. For example:

```bash
gdb-multiarch bootloader.elf
```

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
replacing any existing files. Alternatively to installing and upgrading
OpenOCD you can build the latest from Git.

### Connect all UART and JTAG pins from SiPeed to the RPI.

The UART Rx and Tx connect to the opposite side (Rx to Tx, Tx to Rx).
All other pins are direct connect. Do not forget GND pins. See
Chapter 4 for pin numbers and GPIO's. The pins on the SiPeed are
clearly labled to avoid errors.

WARNING - power off the RPi and disconnect the USB adapter before
connecting the GPIO wires. Double check that each GPIO connection is
correct and snug before powering on. A loose wire is all it takes for
OpenOCD to fail spectacularly.

### Connect the USB SiPeed adapter to the Windows PC.

This should produce two new devices within the Device Manager,
USB Serial Converter A and B.

### Change the USB driver

The USB Serial Converter A must be reconfigured to use the WinUSB
driver in order for OpenOCD to connect and control the JTAG correctly.
By default the FT2232 based SiPeed is configured by Windows to be two
COM ports. However, it is required to replace the driver for the
primary UART (A) with WinUSB before OpenOCD can use it for JTAG.

To do this, download, install and execute the UsbDriverTool:

[https://visualgdb.com/UsbDriverTool/](https://visualgdb.com/UsbDriverTool/)

Upon execution, the UsbDriverTool Gui will appear, with a list of
current USB devices on your system. Right click the USB Serial
Converter A and choose 'Install WinUSB'.

### Execute OpenOCD

Now it is time to test OpenOCD. For example, from the directory
Lab8 Operating System\applications\bootloader of this Git
repository, the following command will initialize OpenOCD for the
SiPeed (FT2232) adapter and the connected RPi 4 target:

```bash
openocd -f ../../boards/rpi/openocd_sipeed_jtag.cfg -f ../../boards/rpi/rpi4_jtag.cfg
```

This openocd_sipeed_jtag.cfg is nothing more than a copy of the interface
configuration file openocd-usb.cfg released by OpenOCD.

openocd/scripts/interface/ftdi/openocd-usb.cfg

With OpenOCD, the first configuration file is for the adapter and the
second file is for the target board. See Chapter 4 of the Laboratory
book for more details on how to connect GDB to OpenOCD and debug the
remote RPi target. This chapter (4) is included in the free preview of
the book (link below).


### UART
 
Search the Windows Device Manager for the USB Serial Converter B and
note the COM port number. You will use this COM port number when
connecting with a Windows terminal software, such as TeraTerm or PuTTY.

Install either Tera Term or Putty now. If you want simplicity and only
intend to use with the RPi, choose Tera Term. For more features and
multiple configurations/target boards, choose Putty. The links
below are to the download page on lo4d.com.

[https://tera-term.en.lo4d.com/windows](https://tera-term.en.lo4d.com/windows)

[https://putty.en.lo4d.com/windows](https://putty.en.lo4d.com/windows)

Once installed, configure the terminal application to connect with the
COM port assigned by Windows to USB Serial Converter B.

In Tera Term, click the Setup menu and select Serial port... Then
select the COM port from the Port: drop down list. Keep the defaults
of 115200 baud 8N1 and press Ok. Once you have verified the UART
connection is a success choose Setup menu and Save settup... Save the
file as the default TERATERM.INI and this COM port will be opened
by default every time Tera Term is launched.

For PuTTY, the opening PuTTY Configuration has a Serial radio button
on the top right side, in the Connection type: section. Select Serial
and then enter the COM port (COM5 for example) in the Serial line
field, and 115200 in the Speed. Press the Open button at the bottom
to connect to the RPi over the COM port. If success you can use
the Save button to save your configuration for quick loading next
time you launch PuTTY; double click the saved name in the list to
connect.

### Happy coding and debugging!

First half of the Laboratory. Click Read Free Sample and choose
your preferred reading format (PDF recommended for PC reading).
[https://leanpub.com/computersystems_lab_rpi](https://leanpub.com/computersystems_lab_rpi)

Computer Systems book preview. Click Read Free Sample and choose
your preferred reading format (PDF recommended for PC reading).
[https://leanpub.com/computersystems](https://leanpub.com/computersystems)

