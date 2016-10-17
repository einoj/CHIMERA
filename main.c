#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "adc.h"
#include "timers.h"
#include "HAL.h"
#include "kiss_tnc.h"
#include "main.h"
#include "memories.h"
#include "spi_memory_driver.h"
#include "ldo.h"

// The number of bytes in the CHI_Board_Status struct.
// Needed to send the data over UART
#define CHI_BOARD_STATUS_LEN 7

// Interrupt: ADC Latch-Up monitoring

// Interrupt: TIMER 1ms incremented timer

// Send ACK, to be moved into proper file

uint8_t transmit_test(uint8_t* data, uint16_t num_bytes)
{
    uint16_t i;

    for(i = 0; i < num_bytes; i++) {
       USART0SendByte(data[i]);
    }
    return 0;
} 

// Power On Check to see what was the cause of reset
void static Power_On_Check() {

	CHI_Board_Status.reset_type = MCUCSR; // Read MCUCSR register to

	if (CHI_Board_Status.reset_type&0x01) {
		// Normal operation
	}
	else if (CHI_Board_Status.reset_type&0x08) {
		// Watchdog reset
	}
	else if (CHI_Board_Status.reset_type&0x04) {
		// Brown-out reset
	}
	else {
		// some other source
	}
}

// Timer 1 - Instrument Time
ISR(TIMER1_OVF_vect) {
	PORTB ^=0x20; // toggle TGL2 to check frequency
	CHI_Board_Status.local_time++;
	TCNT1=0xFFFF-125; // We need 125 ticks to get 1ms interrupt
}

// Timer 3 - Instrument Time
ISR(TIMER3_OVF_vect) {
	CHI_Board_Status.SPI_timeout_detected=1;
	TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt
}

void read_memory(uint8_t mem_idx) {
    uint8_t buf[256]; // the buffer must fit a whole page of, some memories have different page sizes
    uint16_t i;
    // If we have a timeout SEFI we want to jump to the next memory
    uint8_t jmp_next_mem = 0;

    for (i = 0; i < mem_arr[mem_idx].page_num; i++) {

        //reset timer
        CHI_Board_Status.SPI_timeout_detected=0;
        TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt

        // read page
        while (read_24bit_page(0, mem_idx, buf) == BUSY) {
            if (CHI_Board_Status.SPI_timeout_detected) {
                // SEFI detected
                CHI_Memory_Status[mem_idx].no_SEFI_timeout++;
	            CHI_Board_Status.no_SEFI_detected++; //number of SEFIs
                jmp_next_mem = 1;
                break;
            }
        }
        if (jmp_next_mem) {
            break;
        }

        //check page
        
            // if pattern error..

            // if pattern error > 10
            //
            
        // if error erase and reprogram page
    }
}

int main(void)
{
    // Variables for memory access
    uint32_t read_addr; // current read address
    uint8_t curr_page; // current page that is being read

	OSCCAL=0xB3; // clock calibration
	volatile uint32_t start_time;	
	
	// Initialize the Board
	PORT_Init();
	//ADC_Init();
    SPI_Init();
	TIMER0_Init();	// Parser/time-out Timer
	TIMER1_Init();	// Instrument Time Counter
	TIMER3_Init();	// SPI Time-Out Counter
	USART0_Init();	// UART0 Initialization
		
	sei();				// Turn on interrupts	
	Power_On_Check();	// check what was the cause of reset
	
	// Initialize the Board
    CHI_Board_Status.reset_type = 65;
    CHI_Board_Status.device_mode = 0x01;
    CHI_Board_Status.latch_up_detected = 67;
    CHI_Board_Status.working_memories = 68;
    CHI_Board_Status.no_cycles = 69;
    CHI_Board_Status.no_LU_detected = 70;
	CHI_Board_Status.no_SEU_detected = 71; //number of SEUs
	CHI_Board_Status.no_SEFI_detected = FEND; //number of SEFIs
	
    //transmit_kiss((uint8_t*) &CHI_Board_Status, CHI_BOARD_STATUS_LEN);
    // Test of detailed frame transmission
    CHI_Board_Status.local_time = 0xDEADBEEF;
    //transmit_CHI_SCI_TM();

    //disable_memory(mem_arr[1]);
    uint8_t status_reg = 0x66;
    enable_pin_macro(PORTB, 0x10); // Turn on LDO

    // enable all memories
    for (uint8_t i = 0; i < 12; i++) {
        enable_pin_macro(*mem_arr[i].cs_port, mem_arr[i].PIN_CS);
        enable_memory_vcc(mem_arr[i]);
    }

    /*
    spi_command(WREN,7);
    uint8_t memid;
    get_jedec_id(7, &memid);
    USART0SendByte(memid);
    uint8_t buffer[1024];
    erase_chip(7);
    uint8_t status =0x01;

    while (write_24bit_page(0,7) == BUSY);
    while (read_24bit_page(0, 7, buffer) == BUSY);

    while(1){};
    */

        //read_status_reg(&status_reg,7); 
    //disable_memory_vcc(mem_arr[11]);

    //enable_cs_macro (*mem_arr[1].cs_port, mem_arr[1].PIN_CS);
    //while (1) USART0SendByte((uint8_t) CHI_Board_Status);
	
	// Load Configuration&Status from EEPROM (i.e. already failed memories, no LU event, what memory was processed when watchdog tripped)
	
	// Prepare scientific program (see if maybe one of the memories is failing all the time and exclude it)
	
	/* Main Loop */
	
	// Open issue: policy of watchdog, how to set-up watchdog and how/when to reset it?
    while (1) 
    {	
		start_time=CHI_Board_Status.local_time;
		
		CHI_Board_Status.no_cycles++; // increase number of memory cycles

		transmit_CHI_SCI_TM();

		for (int i=0;i<12;i++) {
			if (((CHI_Memory_Status[i].no_SEFI_LU)&0x0F)>3)	{
				// exclude the memory from the test if LU > 3 TBD
				CHI_Board_Status.mem_to_test&=~(1<<i);
			}

			else if ((((CHI_Memory_Status[i].no_SEFI_LU)&0xF0)>>4)>10)	{
				// exclude the memory from the test if SEFI > 10 TBD
				CHI_Board_Status.mem_to_test&=~(1<<i);
            }
			// write it into EEPROM or after reset we start from scratch?
		}

		for(unsigned char i=0;i<12;i++) {
			if (CHI_Board_Status.mem_to_test & (1<<i)) {
				CHI_Board_Status.current_memory=i;
		
				TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt, reset CNT every time
		
				// checked in loop, abort rest of the test if needed
				if (CHI_Board_Status.latch_up_detected) {
					CHI_Memory_Status[CHI_Board_Status.current_memory].no_SEFI_LU=(CHI_Memory_Status[CHI_Board_Status.current_memory].no_SEFI_LU+1)&0x0F;
					CHI_Board_Status.latch_up_detected=0; // clear flag
				}

				if (CHI_Board_Status.SPI_timeout_detected) {
					CHI_Memory_Status[CHI_Board_Status.current_memory].no_SEFI_LU=(CHI_Memory_Status[CHI_Board_Status.current_memory].no_SEFI_LU+16)&0xF0;
					CHI_Board_Status.SPI_timeout_detected=0; // clear flag
				}
		
				// Check if memory is OK, if failed skip

				// Load parameters of the memory (i.e. block size, size of memory etc.) Not necessary, this is all stored in the mem_arr struct
                 
				// Test memory (test procedure defined by device_mode(read only, write read, etc.))
                switch  (CHI_Board_Status.device_mode ) {
                    case 0x01: //readmode
                        // write to EEPROM that memory i is being tested
                        read_memory(i);
                        break;

                    case 0x02:
                        //erase_read_write(mem_arr[i]);
                        break;

                    default:
                        read_memory(mem_arr[i]);
                        break;
                }
                
				// Test procedure has to foresee the possibility of timeout in case of SEFI/LU
                
				// Proposal for test procedure
				// write to EEPROM that we are testing Mem X
				// set watchdog for 1.8 second
				// process each memory by fixed value, 1024 Bytes?
				// after each page reset watchdog timer
				// open issue: what happens if watchdog is tripped? if after reboot we read from EEPROM that Mem X was being processed, we assume that watchdog tripped meanwhile? It is possible to also check that the last reset was triggered by the watchdog
				// Write test results to proper CHI_Memory_Status_Str
				// If there were some problems (SEFI,LU,SEU) write it to memory_status
				// Parse Any Command
		
			}
		}
		// Write all memory status to EEPROM in case there is power down
		
		// Obsolete(?):
		while(CHI_Board_Status.local_time-start_time<10000){ // wait 1 second, RESET WATCHDOG IF NEEDED
			TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt, reset CNT every time
		}
    }
}
