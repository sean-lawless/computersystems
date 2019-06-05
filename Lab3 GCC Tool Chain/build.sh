# optional to use option -S to compile C source only into assembly
arm-none-eabi-gcc -S -O2 -ffreestanding -I. -o main.s main.c

#option -c compiles and assembles
arm-none-eabi-gcc -c -O2 -ffreestanding -I. -o main.o main.c

#Use ld to link with RPi memory map
arm-none-eabi-ld -T memory.map -o app.elf main.o

#strip elf into binary and copy to kernel.img
arm-none-eabi-objcopy app.elf -O binary app.bin
cp app.bin kernel.img
