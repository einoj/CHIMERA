#define SOFTWARE_VERSION 0x10

#define NUM_MEMORIES 12

#define CHI_UART_RX_BUFFER_SIZE 20
#define CHI_PARSER_TIMEOUT 157

#define CHI_COMM_ID_ACK 0x1A
#define CHI_COMM_ID_NACK 0x15
#define CHI_COMM_ID_SCI_TM 0x01
#define CHI_COMM_ID_STATUS 0x02
#define CHI_COMM_ID_EVENT 0x04
#define CHI_COMM_ID_MODE 0x07
#define CHI_COMM_ID_TIMESTAMP 0x08


volatile uint8_t CHI_UART_RX_BUFFER[CHI_UART_RX_BUFFER_SIZE];
volatile uint8_t CHI_UART_RX_BUFFER_INDEX;
volatile uint8_t CHI_UART_RX_BUFFER_COUNTER;
volatile uint16_t Event_cnt; // The number of events to transfer, set to 0 at startup and after a data transfer

// CHIMERA Memory Status Structure
struct __attribute__((packed)) CHI_Memory_Status_Str {
	uint8_t status;
	uint8_t no_SEU;
	uint8_t no_LU;
    uint8_t no_SEFI_timeout;
    uint8_t no_SEFI_wr_error;
	uint8_t current1;
	uint8_t current2;
};
volatile struct CHI_Memory_Status_Str CHI_Memory_Status[NUM_MEMORIES];
//---------------------------------------------

// CHIMERA Memory Event Structure
struct __attribute__((packed)) CHI_Memory_Event_Str {
	uint8_t memory_id;
	uint8_t addr1;
	uint8_t addr2;
	uint8_t addr3;
	uint8_t value;
};
volatile struct CHI_Memory_Event_Str Memory_Events[500];

//---------------------------------------------

// CHIMERA Board Status Structure
struct __attribute__((packed)) CHI_Board_Status_Str {
	uint32_t local_time; // Instrument local time
	uint8_t reset_type; // reason for last reset
	uint8_t device_mode; // mode of the instrument
	uint8_t last_cmd; // last cmd send (in case NACK arrives)
	uint8_t latch_up_detected; // latch up detected flag
	uint8_t SPI_timeout_detected; // SPI time-out detected flag
	uint8_t current_memory;	// id of currently processed memory
	uint16_t mem_to_test; // memories to be tested - each bit corresponds to one memory	
	uint16_t working_memories; // summary of which memory is working
	uint16_t no_cycles; // number of SCI cycles performed on memories
	uint16_t no_LU_detected; //number of Latch-Ups
	uint16_t no_SEU_detected; //number of SEUs
	uint16_t no_SEFI_detected; //number of SEFIs
	uint8_t COMM_flags;
};
volatile struct CHI_Board_Status_Str CHI_Board_Status;
//---------------------------------------------
