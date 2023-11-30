#
# Makefile for console application
#
#
#

##
## Commands:
##
CP	= cp
RM	= rm
LN	= ln
C	= arm-none-eabi-gcc
CC	= arm-none-eabi-gcc
CPP	= arm-none-eabi-gcc
AR	= arm-none-eabi-ar
AS	= arm-none-eabi-as
LINK	= arm-none-eabi-ld
PLINK	= arm-none-eabi-objcopy

##
## Definitions:
##
APPNAME = game

# Define the RPI hardware version: 1 is up to B+, 2 is 2, 3 is up to 3 B+
EXTRAS = -DRPI=1 -ffreestanding
# Begin Historic/Depricated/Older GCC
#RPI 3 B+
#EXTRAS = -DRPI=3 -march=armv8-a+crc -mtune=cortex-a53 -ffreestanding
#RPI 2
#EXTRAS = -DRPI=2 -march=armv7-a -mtune=cortex-a7 -ffreestanding
#RPI B+
#EXTRAS = -DRPI=1 -mcpu=arm1176jzf-s -ffreestanding
# End Historic/Depricated/Older GCC

##Warnings about everything and optimize for size or speed (-Os or -O2)
#CFLAGS = -Wall -O2 $(EXTRAS)
##Warnings and debugging build, with GDB and no optimizations (-O0)
CFLAGS = -Wall -ggdb -O0 $(EXTRAS)

ASFLAGS =
##RPI 3
#ASFLAGS = -mfpu=neon-fp-armv8 -mfloat-abi=hard
##RPI 2
#ASFLAGS = -mfpu=neon-vfpv4 -mfloat-abi=hard
##RPI B+
#ASFLAGS = -mcpu=arm1176jzf-s
INCLUDES = -I. -I../../include -I../../boards/rpi \
           -I../../boards/peripherals

##
## Console application
##

# Use app.o for normal application, vectors.o for bootloader
#APP     = ../../boards/rpi/app.o \
#APP     = ../../boards/rpi/loader.o \

APP     = ../../boards/rpi/app.o \
          ../../boards/rpi/board.o \
          ../../boards/rpi/framebuffer.o \
          ../../boards/rpi/property.o \
          ../../boards/peripherals/dwc/host.o \
          ../../boards/peripherals/dwc/transfer.o \
          ../../boards/peripherals/uart/16550.o \
          ../../boards/peripherals/uart/16C650.o \
          ../../system/character.o \
          ../../system/console.o \
          ../../system/os.o \
          ../../system/assert.o \
          ../../system/printf.o \
          ../../system/rand.o \
          ../../system/screen.o \
          ../../system/string.o \
          ../../system/stdio.o \
          ../../system/shell.o \
          ../../system/timers.o \
          ../../system/xmodem.o \
          ../../usb/device.o \
          ../../usb/endpoint.o \
          ../../usb/request.o \
          denzi_data.o \
          game_grid.o \
          virtual_world.o \
          main.o

LIBS =

##
## Implicit Targets
##
.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) $(INCLUDES) -o $@ $<

.cpp.o:
	$(CPP) -c $(CFLAGS) $(INCLUDES) -o $@ $<

##
## Targets
##

all:	app

app:	$(APP)
	$(LINK) -Map kernel.map -T ../../boards/rpi/memory.map -o $(APPNAME).elf \
	        $(APP) $(LIBS)
	$(PLINK) $(APPNAME).elf -O binary $(APPNAME).bin
	$(CP) $(APPNAME).bin kernel.img

clean:
	rm -f $(APP)
	rm -f $(APPNAME).bin
	rm -f $(APPNAME).elf
	rm -f kernel.img
	rm -f kernel.map

