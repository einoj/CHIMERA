WARNINGS=-Wall -Wextra -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wunused #-Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes 
CFLAGS= -std=gnu99 -Os -D GCC_MEGA_AVR -u vprintf $(WARNINGS) 

AVRFLAGS = -mmcu=atmega128

GCC = avr-gcc.exe
all: uart.o kiss_tnc.o
	$(GCC) $(AVRFLAGS) $(CFLAGS)  main.c $^ -o CHIMERA.elf

uart.o: uart.c
	$(GCC) $(AVRFLAGS) $(CFLAGS) uart.c -c

kiss_tnc.o: crc8.o
	$(GCC) $(AVRFLAGS) $(CFLAGS) kiss_tnc.c -c

crc8.o:
	$(GCC) $(AVRFLAGS) $(CFLAGS) crc8.c -c
