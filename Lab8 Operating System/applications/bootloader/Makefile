#
# Makefile for boot loader
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
APPNAME = bootloader

# Define the RPI hardware version: 1 is up to B+, 2 is 2, 3 is up to 3 B+
EXTRAS = -DRPI=4 -ffreestanding
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
INCLUDES = -I. -I../../include -I../../boards/rpi

##
## Bootloader application (must have loader.o first!)
##
OBJECTS = ../../boards/rpi/loader.o \
          ../../boards/rpi/board.o \
          ../../boards/peripherals/uart/16C650.o \
          ../../boards/peripherals/uart/16550.o \
          ../../system/string.o \
          ../../system/stdio.o \
          ../../system/shell.o \
          ../../system/xmodem.o \
          ../../system/os.o \
          main.o \

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

app:    $(OBJECTS)
	$(LINK) -T ../../boards/rpi/memory.map -o $(APPNAME).elf $(OBJECTS) $(LIBS)
	$(PLINK) $(APPNAME).elf -O binary $(APPNAME).bin
	$(CP) $(APPNAME).bin kernel.img

clean:
	rm -f $(OBJECTS)
	rm -f *.bin
	rm -f *.elf
	rm -f *.img

