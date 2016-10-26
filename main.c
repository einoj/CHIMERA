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
#include "CHIMERA_Board_Defs.h"

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

// Timer 3 - Instrument TimTCNT3
ISR(TIMER3_OVF_vect) {
	CHI_Board_Status.SPI_timeout_detected=1;
	TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt
}

uint8_t read_memory(uint8_t mem_idx) {
    uint8_t buf[256]; // the buffer must fit a whole page of, some memories have different page sizes
    uint16_t i, j;
    uint8_t pattern[2] = {0x55,0xAA};
    uint8_t ptr_idx = 0; // because the order of the pattern changes per page
    uint8_t page_errors; //SEU errors
    uint32_t addr;

    for (i = 0; i < mem_arr[mem_idx].page_num; i++) {

        //reset timer
		// ENABLE TIMER
		TIMER3_Enable_1s();
		
        // read page
        while (read_24bit_page(0, mem_idx, buf) == BUSY) {
			
			if (CHI_Board_Status.latch_up_detected==1) return 0xAC;
			
            if (CHI_Board_Status.SPI_timeout_detected) {
                // SEFI detected, SPI timeout
                CHI_Memory_Status[mem_idx].no_SEFI_timeout++;
				CHI_Memory_Status[mem_idx].no_SEFI_seq++;
	            CHI_Board_Status.no_SEFI_detected++; //number of SEFIs
                // jump to next memory
                return 0;
            }
        }
		
		TIMER3_Disable();
    
        //check page
        page_errors = 0;
        for (j = 0; j < mem_arr[mem_idx].page_size; j++) {
			
		
            if (buf[j] != pattern[ptr_idx]) {
                // if pattern error..
                page_errors++;
                CHI_Board_Status.mem_reprog |= (1<<mem_idx);
                CHI_Memory_Status[mem_idx].no_SEU++;
                //calculate the address of the SEU
                // page_number*pagesize + address in page
                addr = (uint32_t) i*mem_arr[mem_idx].page_size + j;

                if (page_errors > 10) {
                    // remove the last ten errors and store a SEFI
					CHI_Memory_Status[mem_idx].no_SEU -= 11;
					CHI_Memory_Status[mem_idx].no_SEFI_wr_error++;
					CHI_Memory_Status[mem_idx].no_SEFI_seq++;
					CHI_Board_Status.Event_cnt -= 10;
					Memory_Events[CHI_Board_Status.Event_cnt].timestamp = CHI_Board_Status.local_time;
                    Memory_Events[CHI_Board_Status.Event_cnt].memory_id = 0x80 | mem_idx; //1 in upper memory bit signifies a SEFI
                    Memory_Events[CHI_Board_Status.Event_cnt].addr1 = (uint8_t) (addr);
                    Memory_Events[CHI_Board_Status.Event_cnt].addr2 = (uint8_t) (addr>>8);
                    Memory_Events[CHI_Board_Status.Event_cnt].addr3 = (uint8_t) (addr>>16);
                    Memory_Events[CHI_Board_Status.Event_cnt].value = buf[j];
					CHI_Board_Status.Event_cnt++;
                    return 1;
                }

               else if (CHI_Board_Status.Event_cnt < CHI_NUM_EVENT) {
                    Memory_Events[CHI_Board_Status.Event_cnt].timestamp = CHI_Board_Status.local_time;
					Memory_Events[CHI_Board_Status.Event_cnt].memory_id = mem_idx;
                    Memory_Events[CHI_Board_Status.Event_cnt].addr1 = (uint8_t) (addr);
                    Memory_Events[CHI_Board_Status.Event_cnt].addr2 = (uint8_t) (addr>>8);
                    Memory_Events[CHI_Board_Status.Event_cnt].addr3 = (uint8_t) (addr>>16);
                    Memory_Events[CHI_Board_Status.Event_cnt].value = buf[j];
                    CHI_Board_Status.Event_cnt++;
                }
				
				else {
                    //TODO transmit data when EVENT table if full
                }
            }
            ptr_idx ^= 0x01;
        }
        ptr_idx = 0;
    }
					
	CHI_Memory_Status[mem_idx].no_SEFI_seq=0;
    return 0;
}

void Power_On_Init() {
	    CHI_Board_Status.device_mode = 0x01;
	    CHI_Board_Status.latch_up_detected = 0;
	    CHI_Board_Status.mem_to_test = 0x0007;
		CHI_Board_Status.mem_tested = 0;
		CHI_Board_Status.working_memories = 0x0FFF;
        CHI_Board_Status.mem_reprog = 0;
	    CHI_Board_Status.no_cycles = 0;
	    CHI_Board_Status.no_LU_detected = 0;
	    CHI_Board_Status.no_SEU_detected = 0; //number of SEUs
	    CHI_Board_Status.no_SEFI_detected = 0; //number of SEFIs
		CHI_Board_Status.Event_cnt = 0; // EVENT counter
		
		CHI_UART_RX_BUFFER_INDEX=0;
		CHI_UART_RX_BUFFER_COUNTER=0;
		
		// clear all statistics, for loop
		CHI_Memory_Status[0].no_SEU=0;
		CHI_Memory_Status[1].no_SEU=0;
		CHI_Memory_Status[2].no_SEU=0;
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
	Power_On_Init();	// Initialize variable on board
	USART0_Init();	// UART0 Initialization
		
	sei();				// Turn on interrupts	
		
	transmit_CHI_STATUS();

	// LDO for memories ON
	LDO_ON;
	
	// Disable all CS
    for (uint8_t i = 0; i < 12; i++)CHIP_DESELECT(i);
	
    // VCC enable all memories
    for (uint8_t i = 0; i < 12; i++) {
        //enable_pin_macro(*mem_arr[i].cs_port, mem_arr[i].PIN_CS);
        enable_memory_vcc(mem_arr[i]);
    }
		
	/* Main Loop */
    while (1) 
    {	
		start_time=CHI_Board_Status.local_time;
		
		CHI_Board_Status.no_cycles++; // increase number of memory cycles
		
		while(CHI_Board_Status.mem_tested<6) {
				
			for (int i=0;i<12;i++) {	
				if (CHI_Board_Status.mem_to_test & (1<<i)) {
					if (CHI_Board_Status.mem_reprog & (1<<i))	{
					
						CHI_Board_Status.mem_reprog &= ~ (1<<i); // clear reporgramming flag
						// reprogram memory
						// START TIMER, Erase can take upto 6 seconds in M25P05A
						LDO_ON;
						//enable_memory_vcc(mem_arr[i]);
						
						erase_chip(i);
					
						if (CHI_Board_Status.latch_up_detected==1) {
							CHI_Memory_Status[i].no_LU++;
							CHI_Board_Status.latch_up_detected==0;
							continue;
						}
										
						//END TIMER
						
						addr = 0;
						pattern = 0;
						for (uint8_t j = 0; j < mem_arr[i].page_num; j++) {
							//START SPI TIMER should be less than 1 second
							while (write_24bit_page(addr, pattern, i) == BUSY);
							//RESET TIMER
							addr+=mem_arr[i].page_size;
							pattern ^= 0x01;
						}
						//END SPI TIMER
						
						if (CHI_Board_Status.latch_up_detected==1) {
							CHI_Memory_Status[i].no_LU++;
							CHI_Board_Status.latch_up_detected==0;
							continue;
						}						
						
						//disable_memory_vcc(mem_arr[i]);	
						
					}
			
					if ((CHI_Memory_Status[i].no_LU) > 3 )	{
						// exclude the memory from the test if LU > 3 TBD
						CHI_Board_Status.mem_to_test&=~(1<<i);
					}

					else if ((CHI_Memory_Status[i].no_SEFI_seq)>10)	{
						// exclude the memory from the test if SEFI > 10 TBD
						CHI_Board_Status.mem_to_test&=~(1<<i);
					}
				}
			}

			for(uint8_t i=0;i<12;i++) {
				if (CHI_Board_Status.mem_to_test & (1<<i)) {
				
					CHI_Board_Status.current_memory=i;
					CHI_Board_Status.mem_tested++;
				
					// Test memory (test procedure defined by device_mode(read only, write read, etc.))
					switch  (CHI_Board_Status.device_mode ) {
						case 0x01: //readmode
						
							//ENABLE SPI lines
							LDO_ON;
							//enable_memory_vcc(mem_arr[i]);
							
							if (read_memory(i) == 0xAC) {
								CHI_Memory_Status[i].no_LU++;
								CHI_Board_Status.latch_up_detected==0;								
							}
							else if (CHI_Board_Status.latch_up_detected==1) {
								CHI_Memory_Status[i].no_LU++;
								CHI_Board_Status.latch_up_detected==0;
							}							
							
							//disable_memory_vcc(mem_arr[i]);
							break;

						case 0x02:
							//erase_read_write(mem_arr[i]);
							break;

						default:
							read_memory(i);
							break;
					}
				}
			}
		}
		CHI_Board_Status.mem_tested=0;
		transmit_CHI_SCI_TM();
    }
}
