
// CHIMERA Memory Status Structure
struct CHI_Memory_Status_Str {
	uint8_t status;
	uint8_t no_SEU;
	uint8_t no_SEFI_LU;
	uint8_t current1;
	uint8_t current2;
};
volatile struct CHI_Memory_Status_Str CHI_Memory_Status[12];
//---------------------------------------------

// CHIMERA Memory Event Structure
struct CHI_Memory_Event_Str {
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
	uint8_t latch_up_detected; // latch up detected flag
	uint8_t SPI_timeout_detected; // SPI time-out detected flag
	uint8_t current_memory;	// id of currently processed memory
	uint16_t mem_to_test; // memories to be tested - each bit corresponds to one memory	
	uint16_t working_memories; // summary of which memory is working
	uint16_t no_cycles; // number of SCI cycles performed on memories
	uint16_t no_LU_detected; //number of Latch-Ups
	uint16_t no_SEU_detected; //number of SEUs
	uint16_t no_SEFI_detected; //number of SEFIs
};
volatile struct CHI_Board_Status_Str CHI_Board_Status;
//---------------------------------------------