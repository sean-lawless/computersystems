REM RPi 2 -march=armv7-a -mtune=cortex-a7
REM RPi B+ -mcpu=arm1176jzf-s
arm-none-eabi-gcc -c -DRPI=3 -ffreestanding -I. -I..\..\include -I..\..\boards\rpi -o main.o main.c
arm-none-eabi-gcc -c -DRPI=3 -ffreestanding -I. -I..\..\include -I..\..\boards\rpi -o ..\..\boards\rpi\board.o ..\..\boards\rpi\board.c
arm-none-eabi-ld -T ..\..\boards\rpi\memory.map -o errorcodes.elf main.o ..\..\boards\rpi\board.o
arm-none-eabi-objcopy errorcodes.elf -O binary errorcodes.bin
cp errorcodes.bin kernel.img

