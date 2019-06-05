set INCLUDES=-I. -I..\..\include -I..\..\boards\rpi
#Change the DRPI below for your RPI hardware version: 1, 2 or 3
set CFLAGS=-DRPI=3 -ffreestanding

arm-none-eabi-gcc -c %CFLAGS% %INCLUDES% -o main.o main.c
arm-none-eabi-gcc -c %CFLAGS% %INCLUDES% -o ..\..\boards\rpi\board.o ..\..\boards\rpi\board.c
arm-none-eabi-gcc -c %CFLAGS% %INCLUDES% -o ..\..\boards\rpi\uart0.o ..\..\boards\rpi\uart0.c
arm-none-eabi-gcc -c %CFLAGS% %INCLUDES% -o ..\..\system\shell.o ..\..\system\shell.c
arm-none-eabi-ld -T memmap -o console.elf main.o ..\..\boards\rpi\board.o ..\..\boards\rpi\uart0.o ..\..\system\shell.o
arm-none-eabi-objcopy console.elf -O binary console.bin
cp console.bin kernel7.img

