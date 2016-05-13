// VCC chip enable pins, connected to hex inverter inputs
//  VCC_EN 1-5 are connected to PC, 6-7 to PG, 8-11 to PD and 12 to PB.
#define VCC_EN1 4

void hex_init(void) {
    // Set MEM_VCC_EN1 as output
    DDRA|=(1<<VCC_EN1);

    // Turn off by default
    PORTA |= (0<<VCC_EN1);
}

void toggle_hex(uint8_t port) {
    PORTA = PORTA ^ (1<<port);
}
