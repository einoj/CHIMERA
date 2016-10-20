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

uint8_t read_memory(uint8_t mem_idx) {
    uint8_t buf[256]; // the buffer must fit a whole page of, some memories have different page sizes
    uint16_t i, j;
    // If we have a timeout SEFI we want to jump to the next memory
    uint8_t jmp_next_mem = 0;
    uint8_t pattern[2] = {0x55,0xAA};
    uint8_t ptr_idx = 0; // because the order of the pattern changes per page
    uint8_t page_errors;
    uint32_t addr;

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
                // jump to next memory
                return 0;
            }
        }
        // TODO Stop timer?

        //check page
        page_errors = 0;
        for (j = 0; j < mem_arr[mem_idx].page_size; j++) {
            if (buf[j] != pattern[ptr_idx]) {
                // if pattern error..
                page_errors++;
                CHI_Memory_Status[mem_idx].no_SEU++;
                //calculate the address of the SEU
                // page_number*pagesize + address in page
                addr = (uint32_t) i*mem_arr[mem_idx].page_size + j;

                if (page_errors > 10) {
                    // remove the last three errors and store a SEFI
                    Event_cnt -= 10;
                    Memory_Events[Event_cnt].memory_id = 0x80 | mem_idx; //1 in upper memory bit signifies a SEFI
                    Memory_Events[Event_cnt].addr1 = (uint8_t) (addr);
                    Memory_Events[Event_cnt].addr2 = (uint8_t) (addr>>8);
                    Memory_Events[Event_cnt].addr3 = (uint8_t) (addr>>16);
                    Memory_Events[Event_cnt].value = buf[j];
                    // Reprogram and move on to next page
                    return 1;
                }

                if (Event_cnt < 500) {
                    Memory_Events[Event_cnt].memory_id = mem_idx;
                    Memory_Events[Event_cnt].addr1 = (uint8_t) (addr);
                    Memory_Events[Event_cnt].addr2 = (uint8_t) (addr>>8);
                    Memory_Events[Event_cnt].addr3 = (uint8_t) (addr>>16);
                    Memory_Events[Event_cnt].value = buf[j];
                    Event_cnt++;
                } else {
                    //TODO transmit data
                }
            }
            ptr_idx ^= ptr_idx;
        }
        // next page has opposite pattern
        ptr_idx ^= ptr_idx;
    }
    return 0;
}

void Power_On_Init() {
	    CHI_Board_Status.device_mode = 0x01;
	    CHI_Board_Status.latch_up_detected = 0;
	    CHI_Board_Status.mem_to_test = 0x0FFF;
		CHI_Board_Status.working_memories = 0x0FFF;
	    CHI_Board_Status.no_cycles = 0;
	    CHI_Board_Status.no_LU_detected = 0;
	    CHI_Board_Status.no_SEU_detected = 0; //number of SEUs
	    CHI_Board_Status.no_SEFI_detected = 0; //number of SEFIs
}

int main(void)
{
    // Variables for memory access
    uint32_t addr; // current address
    uint8_t pattern; // pattern of current page
    
	Power_On_Check();	// check what was the cause of reset
	
	OSCCAL=0xB3; // clock calibration
	volatile uint32_t start_time;	
	
	// Initialize the Board
	PORT_Init();	//Initialize the ports
	ADC_Init();		// Initialize the ADC for latch-up
    SPI_Init();		// Initialize the SPI
	TIMER0_Init();	// Parser/time-out Timer
	TIMER1_Init();	// Instrument Time Counter
	TIMER3_Init();	// SPI Time-Out Counter
	USART0_Init();	// UART0 Initialization
	
	Power_On_Init();	// Initialize variable on board
		
	sei();				// Turn on interrupts	
	
    Event_cnt = 0; // ?????
		
	//  -v- ??????? remove?
    //disable_memory(mem_arr[1]);
    uint8_t status_reg = 0x66;
    enable_pin_macro(PORTB, 0x10); // Turn on LDO

    // enable all memories
    for (uint8_t i = 0; i < 12; i++) {
        enable_pin_macro(*mem_arr[i].cs_port, mem_arr[i].PIN_CS);
        enable_memory_vcc(mem_arr[i]);
    }

    spi_command(WREN,7);
    uint8_t memid;
    get_jedec_id(7, &memid);
    USART0SendByte(memid); 

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
			if ((CHI_Memory_Status[i].no_LU) >3)	{
				// exclude the memory from the test if LU > 3 TBD
				CHI_Board_Status.mem_to_test&=~(1<<i);
			}

			else if ((CHI_Memory_Status[i].no_SEFI_timeout+CHI_Memory_Status[i].no_SEFI_timeout)>10)	{
				// exclude the memory from the test if SEFI > 10 TBD
				CHI_Board_Status.mem_to_test&=~(1<<i);
            }
			// write it into EEPROM or after reset we start from scratch?
		}

		for(uint8_t i=0;i<12;i++) {
			if (CHI_Board_Status.mem_to_test & (1<<i)) {
				CHI_Board_Status.current_memory=i;
		
				TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt, reset CNT every time
		
				// checked in loop, abort rest of the test if needed
				if (CHI_Board_Status.latch_up_detected) {
					CHI_Memory_Status[CHI_Board_Status.current_memory].no_LU=CHI_Memory_Status[CHI_Board_Status.current_memory].no_LU+1;
					CHI_Board_Status.latch_up_detected=0; // clear flag
				}

		//		if (CHI_Board_Status.SPI_timeout_detected) {
		//			CHI_Memory_Status[CHI_Board_Status.current_memory].no_SEFI_LU=(CHI_Memory_Status[CHI_Board_Status.current_memory].no_SEFI_LU+16)&0xF0;
		//			CHI_Board_Status.SPI_timeout_detected=0; // clear flag
		//		}
		//
				// Check if memory is OK, if failed skip

				// Load parameters of the memory (i.e. block size, size of memory etc.) Not necessary, this is all stored in the mem_arr struct
                 
				// Test memory (test procedure defined by device_mode(read only, write read, etc.))
                switch  (CHI_Board_Status.device_mode ) {
                    case 0x01: //readmode
                        // write to EEPROM that memory i is being tested
                        if (read_memory(i)) {
                            // reprogram memory 
                            erase_chip(i);
                            addr = 0;
                            pattern = 0;
                            for (uint8_t j = 0; j < mem_arr[i].page_num; j++) {
                                write_24bit_page(addr, pattern, i); 
                                addr+=mem_arr[i].page_size;
                                pattern ^= 0x01;
                            }
                        }
                        break;

                    case 0x02:
                        //erase_read_write(mem_arr[i]);
                        break;

                    default:
                        read_memory(i);
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
		while(CHI_Board_Status.local_time-start_time<10000){ // wait 10 second, RESET WATCHDOG IF NEEDED
			TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt, reset CNT every time
		}
    }
}
