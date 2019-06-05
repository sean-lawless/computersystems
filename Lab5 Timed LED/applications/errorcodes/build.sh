#options -nostartfiles -ffreestanding -nostdlib prevent linking in GNU stuff
#option -c compiles/assembles, -O2 optimizes

# RPI 2
# options -march=armv7-a -mtune=cortex-a7 are for RPi 2
# RPI B+
#option -mcpu=arm1176jzf-s is for RPi B+

arm-none-eabi-gcc -c -DRPI=3 -ffreestanding -I. -I../../include -I../../boards/rpi -o main.o main.c
arm-none-eabi-gcc -c -DRPI=3 -ffreestanding -I. -I../../include -I../../boards/rpi -o ..\..\boards\rpi\board.o ..\..\boards\rpi\board.c

#Use ld to link with RPi memory map
arm-none-eabi-ld -T ../../boards/rpi/memory.map -o errorcodes.elf main.o ../../boards/rpi/board.o

#strip elf into binary and copy to kernel.img
arm-none-eabi-objcopy errorcodes.elf -O binary errorcodes.bin
cp errorcodes.bin kernel.img

