REM sequence of commands to compile and link the application
arm-none-eabi-gcc -S -O2 -ffreestanding -I. -o main.s main.c
arm-none-eabi-gcc -c -O2 -ffreestanding -I. -o main.o main.c
arm-none-eabi-ld -T memory.map -o app.elf main.o
arm-none-eabi-objcopy led.elf -O binary app.bin
cp app.bin kernel.img
REM create assembly file
arm-none-eabi-gcc -S -I. main.c

