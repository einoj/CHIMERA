/* Address space */
#define TOP_ADDR 0xfffff  //This is last address of SST25VF080B, the other memories are smaller,//memory sizes should probably be defined in a struct instead, and the struct be an parameter for the read/write functions 
#define OUT_OF_RANGE 0xFD // The address is out of range of available memory
#define NB_ADDR_BYTE 3 // The serial memory has a 3 bytes long address phase

// The SPI interface is ready to send
#define READY_TO_SEND 0x0

/* OP codes */
#define AAI         0xAD
#define BYTE_PRG    0x02
#define CHIP_ERASE  0x60
#define JEDEC       0x9F
#define READ        0x03
#define WREN        0x06
#define WRSR        0x01
#define RDSR        0x05

/* Memory access status */
#define TRANSFER_STARTED 0x0    // The write access is started without error
#define TRANSFER_COMPLETED 0x1  // The read access is completed without error
#define OUT_OF_RANGE 0xFD       // The address is out of range of available memory
#define BUSY        0xFF        // The SPI memory or the SPI interface is busy.

/* Status register bits */
#define WIP 0 // Bit 0 indicates whether an internal write operation is in progress
#define WEL 1 // Bit 1 indicates whether memory is write enabled (1) or disabled (0)

/* Macros */
// Pull down chip select line
#define SELECT_SERIAL_MEMORY PORTB=(PORTB & ~(1<<DDB4));
// pull up chip select line
#define DESELECT_SERIAL_MEMORY PORTB=(PORTB | (1<<DDB4));
// disable SPI interrupts
#define ENABLE_SPI_INTERRUPT SPCR|=(1 << SPIE);
// enable SPI interrupts
#define DISABLE_SPI_INTERRUPT SPCR&=~(1 << SPIE);
// enable MISO interrput
#define ENABLE_MISO_INTERRUPT PCMSK1|=(1 << PCINT14);
// disable MISO interrput
#define DISABLE_MISO_INTERRUPT PCMSK1&=~(1 << PCINT14);

/* SPI states */
// The op_code has been transferred
#define INSTRUCTION 0x1
// One of the address bytes has been transferred
#define ADDRESS 0x2
// The current data byte has been transferred
#define DATA 0x4

/* Global variables */
static uint32_t address; // Used by SPI to address memory device
static uint8_t *data_ptr; // Used by SPI interrupt, points to data that is to be written
static uint8_t byte_cnt; // Used by SPI interrupt to know how many bytes have been written
static uint8_t nb_byte; // Used by SPI interrupt to know how many bytes need to be written
static uint8_t state; // Used by SPI interrupt to indicate whether to write data or addr

/* Function definitions */
static void printuart(int8_t *msg);
