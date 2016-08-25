WARNINGS=-Wall -Wextra -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wunused #-Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes 
CFLAGS=-D GCC_MEGA_AVR -u vprintf $(WARNINGS) 

UI = ../ui
MAINDIR = ../

INCLUDE = -I$(UI) -I$(MAINDIR)

frame_test: chimera.o crc8.o readbin.o frame_test.c
	gcc $(INCLUDE) readbin.o chimera.o crc8.o frame_test.c -o frame_test

chimera.o: crc8.o 
	gcc $(INCLUDE) ../ui/chimera.c -c 

crc8.o: $(MAINDIR)/crc8.c
	gcc $(INCLUDE) ../crc8.c -c 

readbin.o: readbin.c
	gcc readbin.c -c 

