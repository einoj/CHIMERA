WARNINGS=-Wall -Wextra -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wunused #-Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes 
CFLAGS= -std=gnu99 -Os -D GCC_MEGA_AVR -u vprintf $(WARNINGS) 

AVRFLAGS = -mmcu=atmega128

GCC = avr-gcc.exe

all: uart.o kiss_tnc.o crc8.o memories.o HAL.o adc.o timers.o
	$(GCC) $(AVRFLAGS) $(CFLAGS)  main.c $^ -o CHIMERA.elf

memories.o: memories.c
	$(GCC) $(AVRFLAGS) $(CFLAGS) memories.c -c

uart.o: uart.c
	$(GCC) $(AVRFLAGS) $(CFLAGS) uart.c -c

kiss_tnc.o: kiss_tnc.c
	$(GCC) $(AVRFLAGS) $(CFLAGS) kiss_tnc.c -c

crc8.o: crc8.c
	$(GCC) $(AVRFLAGS) $(CFLAGS) crc8.c -c

HAL.o: HAL.c
	$(GCC) $(AVRFLAGS) $(CFLAGS) HAL.c -c

adc.o: adc.c
	$(GCC) $(AVRFLAGS) $(CFLAGS) adc.c -c

timers.o: timers.c
	$(GCC) $(AVRFLAGS) $(CFLAGS) timers.c -c

clean:
	rm *.o
