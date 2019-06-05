#GCC option -c compiles/assembles, -O2 optimizes
#    option -ffreestanding prevents auto-linking of anything
#    RPi 2 options -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=har
#    RPi B+ options -mcpu=arm1176jzf-s -mfpu=vfp

INCLUDES="-I. -I../../include -I../../boards/rpi"
CFLAGS="-ffreestanding"

arm-none-eabi-gcc -c $INCLUDES $CFLAGS -o main.o main.c
arm-none-eabi-gcc -c $INCLUDES $CFLAGS -o ../../boards/rpi/board.o ../../boards/rpi/board.c
arm-none-eabi-gcc -c $INCLUDES $CFLAGS -o ../../boards/rpi/uart0.o ../../boards/rpi/uart0.c
arm-none-eabi-gcc -c $INCLUDES $CFLAGS -o ../../system/shell.o ../../system/shell.c
arm-none-eabi-ld -T ../../boards/rpi/memory.map -o console.elf main.o ../../boards/rpi/board.o ../../boards/rpi/uart0.o ../../system/shell.o
arm-none-eabi-objcopy console.elf -O binary console.bin
cp console.bin kernel.img

