WARNINGS=-Wall -Wextra -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wunused #-Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes 
CFLAGS=-D GCC_MEGA_AVR -u vprintf #$(WARNINGS) 
all:
	avr-gcc -mmcu=atmega1284p -std=gnu99 -Os $(CFLAGS) uart.c -o uart.elf

clean:
	rm uart.elf

prg:
	avrdude -p m1284p -c dragon_jtag -P USB -U flash:w:uart.elf:e
