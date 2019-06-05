#options -ffreestanding ensures no linkage to GCC external libraries
#option -c compiles/assembles, -O2 optimizes, -ggdb adds debug symbols

#Compile main.c for RPI 3 with debug symbols and no optimizations
#  change -DRPI to the version of RPI system used to execute
#arm-none-eabi-gcc -c -DRPI=3 -ggdb -O0 -ffreestanding -I. -o main.o main.c
#Compile main.c for RPI 1 with no debug symbols and optimizations
arm-none-eabi-gcc -c -DRPI=3 -O2 -ffreestanding -I. -o main.o main.c

#Use ld to link with RPi memory map
arm-none-eabi-ld -T memory.map -o led.elf main.o

#strip elf into binary and copy to kernel.img
arm-none-eabi-objcopy led.elf -O binary led.bin
cp led.bin kernel.img

