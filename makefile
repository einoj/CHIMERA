WARNINGS=-Wall -Wextra -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wunused #-Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes 
CFLAGS=-D GCC_MEGA_AVR -u vprintf #$(WARNINGS) 
all:
	avr-gcc -mmcu=atmega1284p -std=gnu99 -Os $(CFLAGS) uart.c -o uart.elf

clean:
	rm uart.elf
