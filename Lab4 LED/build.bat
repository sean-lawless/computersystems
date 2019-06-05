REM sequence of commands to compile and link the application

arm-none-eabi-gcc -c -DRPI=3 -ggdb -O0 -ffreestanding -I. -o main.o main.c
arm-none-eabi-ld -T memory.map -o led.elf main.o
arm-none-eabi-objcopy led.elf -O binary led.bin
cp led.bin kernel.img

