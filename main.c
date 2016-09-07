#include <avr/io.h>
#include "uart.h"

// CHIMERA Memory Status Structure
struct CHI_Memory_Status_Str {
	unsigned char status;
	unsigned char no_SEU;
	unsigned char no_SEFI_LU;
	unsigned char Current1;
	unsigned char Current2;
};
//---------------------------------------------

// CHIMERA Board Status Structure
struct CHI_Board_Status_Str {
	unsigned char reset_type;
	unsigned char device_mode;
	unsigned short working_memories;
	unsigned short no_cycles;
	unsigned short no_LU_detected;
	unsigned short no_SEU_detected;
	unsigned short no_SEFI_detected;
};

struct CHI_Board_Status_Str CHI_Board_Status;
//---------------------------------------------

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

int main(void)
{
	// Update Board Status with reason for reset (i.e. Watchdog, BOD)
	
	// Initialize the Board

    USART_Init();

        while ( !( UCSR0A & (1<<UDRE0)) ) ;
        UDR0 = 0xAA;
        while ( !( UCSR0A & (1<<UDRE0)) ) ;
        UDR0 = 0x55;
	
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
