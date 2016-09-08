#include <avr/io.h>
#include "uart.h"
#include "kiss_tnc.h"
#include "main.h"
#include "memories.h"

// The number of bytes in the CHI_Board_Status struct.
// Needed to send the data over UART
#define CHI_BOARD_STATUS_LEN 13

// Interrupt: ADC Latch-Up monitoring

// Interrupt: TIMER 1ms incremented timer

// Interrupt: UART Data Reception
ISR(USART0_RX_vect)
{
signed char cChar;

	/* Get the character and post it on the queue of Rxed characters.
	If the post causes a task to wake force a context switch as the woken task
	may have a higher priority than the task we have interrupted. */
	cChar = UDR0;
    UDR0 = cChar;
}

uint8_t transmit_test(uint8_t* data, uint16_t num_bytes)
{
    uint16_t i;

    for(i = 0; i < num_bytes; i++) {
       USART0SendByte(data[i]);
    }
    return 0;
} 

int main(void)
{
	// Update Board Status with reason for reset (i.e. Watchdog, BOD)
	
	// Initialize the Board

    CHI_Board_Status.reset_type = 65;
    CHI_Board_Status.device_mode = 66;
    CHI_Board_Status.latch_up_detected = 67;
    CHI_Board_Status.working_memories = 68;
    CHI_Board_Status.no_cycles = 69;
    CHI_Board_Status.no_LU_detected = 70;
	CHI_Board_Status.no_SEU_detected = 71; //number of SEUs
	CHI_Board_Status.no_SEFI_detected = 72 ; //number of SEFIs

    USART0_Init();
    
    transmit_kiss((uint8_t*) &CHI_Board_Status, CHI_BOARD_STATUS_LEN);

//    enable_cs_macro (*mem_arr[1].cs_port, mem_arr[1].PIN_CS);
    //while (1) USART0SendByte((uint8_t) CHI_Board_Status);
	
	// Load Configuration&Status from EEPROM (i.e. already failed memories, no LU event, what memory was processed when watchdog tripped)
	
	// Prepare scientific program (see if maybe one of the memories is failing all the time and exclude it)
	
	/* Main Loop */
	
	// Open issue: policy of watchdog, how to set-up watchdog and how/when to reset it?
    while (1) 
    {	
		// Record time
		
		// Send TM_SCI packet
		
		// Parse Any Command that came

		// Execute 12 times for each memory:
			// Check if memory is OK, if failed skip
			
			// Load parameters of the memory (i.e. block size, size of memory etc.)
			// Test memory (test procedure defined by device_mode(read only, write read, etc.))
				// Test procedure has to foresee the possibility of timeout in case of SEFI/LU
				// Proposal for test procedure
					// write to EEPROM that we are testing Mem X
					// set watchdog for 1.8 second
					// process each memory by fixed value, 1024 Bytes? 
					// after each page reset watchdog timer
					// open issue: what happens if watchdog is tripped? if after reboot we read from EEPROM that Mem X was being processed, we assume that watchdog tripped meanwhile?
			// Write test results to proper CHI_Memory_Status_Str	
			// If there were some problems (SEFI,LU,SEU) write it to memory_status	
			// Parse Any Command
		// 
		
		// Write all memory status to EEPROM in case there is power down
		
		// Wait for the rest of time with watchdog timer reseting
    }
}
