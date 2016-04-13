all:
	avr-gcc.exe -mmcu=atmega1284p -std=gnu99 -Os -D GCC_MEGA_AVR uart.c -o uart.elf
