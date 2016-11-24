/* Address space */
#define TOP_ADDR 0xfffff  //This is last address of SST25VF080B, the other memories are smaller,//memory sizes should probably be defined in a struct instead, and the struct be an parameter for the read/write functions 
#define OUT_OF_RANGE 0xFD // The address is out of range of available memory
#define NB_ADDR_BYTE 3 // The serial memory has a 3 bytes long address phase

// The SPI interface is ready to send
#define READY_TO_SEND 0x0

/* OP codes */
#define AAI         0xAD
#define PRG         0x02
#define CHIP_ERASE  0x60
#define DBSY        0x80
#define EBSY        0x70
#define JEDEC       0x9F
#define RDSR        0x05
#define READ        0x03
#define WRDI        0x04
#define WREN        0x06
#define WRSR        0x01

/* Memory access status */
#define BUSY                0xFF        // The SPI memory or the SPI interface is busy.
#define OUT_OF_RANGE        0xFD       // The address is out of range of available memory
#define TRANSFER_COMPLETED  0x1  // The read access is completed without error
#define TRANSFER_STARTED    0x0    // The write access is started without error

/* Status register bits */
#define WEL 1 // Bit 1 indicates whether memory is write enabled (1) or disabled (0)
#define WIP 0 // Bit 0 indicates whether an internal write operation is in progress

/* Macros */
// Pull down chip select line
#define SELECT_SERIAL_MEMORY PORTB=(PORTB & ~(1<<DDB4));
// pull up chip select line
#define DESELECT_SERIAL_MEMORY PORTB=(PORTB | (1<<DDB4));
// disable SPI interrupts
#define ENABLE_SPI_INTERRUPT SPCR|=(1 << SPIE);
// enable SPI interrupts
#define DISABLE_SPI_INTERRUPT SPCR&=~(1 << SPIE);
// MISO interrupts are only used if interrupt driven writing is used on the SST memories, currently it is not
// the MISO line will be pulled high when a write command is finished if the interrupt drive mode is used.
// For this to work jumper P9 needs to be mounted on V1 of the board. This connects MISO to PE7, and INT7 can be used to
// interrupt and issue a new spi write command. See sst memory data sheets for more info on interrupt driven writing
// TODO REMEMBER TO ALSO CONFIGURE THE EICRB AND EICRA REGISTERS IF USING INTERRUPT MISO!!
// enable MISO interrput 
#define ENABLE_MISO_INTERRUPT EIMSK|=(1 << INT7);
// disable MISO interrput
#define DISABLE_MISO_INTERRUPT EIMSK&=~(1 << INT7);

/* SPI states */
// The op_code has been transferred
#define INSTRUCTION 0x1
// One of the address bytes has been transferred
#define ADDRESS 0x2
// The current data byte has been transferred
//#define DATA 0x4

/* PIN Locations */
// SPI pins are located on the PB
// Set MOSI, SCK , and SS as Output
#define MOSI 2
#define SCK 1
#define MISO 3

/* Global variables */
//static uint32_t address; // Used by SPI to address memory device
//static uint8_t *data_ptr; // Used by SPI interrupt, points to data that is to be written
//static uint8_t byte_cnt; // Used by SPI interrupt to know how many bytes have been written
//static uint8_t nb_byte; // Used by SPI interrupt to know how many bytes need to be written
static uint8_t state; // Used by SPI interrupt to indicate whether to write data or addr

/* Function definitions */
static void printuart(char *msg);
void USART0SendByte(uint8_t u8Data);
void enable_memory_vcc(struct Memory mem);
void disable_memory_vcc(struct Memory mem);
uint8_t read_status_reg(uint8_t *status, uint8_t mem_idx);
void SPI_Init(void);
uint8_t read_24bit_page(uint32_t addr, uint8_t mem_idx, uint8_t *buffer);
uint8_t read_16bit_page(uint32_t addr, uint8_t mem_idx, uint8_t *buffer);
uint8_t write_24bit_page(uint32_t addr, uint8_t pattern, uint8_t mem_idx);
uint8_t write_16bit_page(uint32_t addr, uint8_t ptr_i, uint8_t mem_idx);
uint8_t spi_command(uint8_t op_code, uint8_t mem_idx);
uint8_t erase_chip(uint8_t mem_idx);
