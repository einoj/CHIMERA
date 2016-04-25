WARNINGS=-Wall -Wextra -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wunused
CFLAGS=-D GCC_MEGA_AVR $(WARNINGS)
all:
	avr-gcc.exe -mmcu=atmega1284p -std=gnu99 -Os $(CFLAGS)  uart.c -o uart.elf
