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

    for(uint16_t i = 0; i < num_bytes; i++) {
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
    uint8_t pattern[2] = {0x55,0xAA};
    uint8_t ptr_idx = 0; // because the order of the pattern changes per page
    uint8_t page_errors; //SEU errors
    uint32_t addr;

		// slowing down the procedure!!!?!!?!
		//TIMER3_Enable_8s();
		//while(CHI_Board_Status.SPI_timeout_detected==0);
		//TIMER3_Disable();
		//---------------------------------------

	CHI_Memory_Status[mem_idx].current1=ADC_Median>>2; // Reading bias current measurement

    for (uint16_t i = 0; i < mem_arr[mem_idx].page_num; i++) {		
        //reset timer
		// ENABLE TIMER
		TIMER3_Enable_1s();

        // read page either with 24 bit address or 16 bit address
        if(mem_arr[mem_idx].addr_space != 0) {
            while (read_24bit_page((uint32_t) i*mem_arr[mem_idx].page_size, mem_idx, buf) == BUSY) {

                if (CHI_Board_Status.latch_up_detected==1) return 0xAC;
                
                if (CHI_Board_Status.SPI_timeout_detected) {
                    // SEFI detected, SPI timeout
                    CHI_Memory_Status[mem_idx].no_SEFI_timeout++;
                    CHI_Memory_Status[mem_idx].no_SEFI_seq++;
                    // jump to next memory
                    return 0;
                }
            }
        } else {
            while (read_16bit_page((uint32_t) i*mem_arr[mem_idx].page_size, mem_idx, buf) == BUSY) {

                if (CHI_Board_Status.latch_up_detected==1) return 0xAC;
                
                if (CHI_Board_Status.SPI_timeout_detected) {
                    // SEFI detected, SPI timeout
                    CHI_Memory_Status[mem_idx].no_SEFI_timeout++;
                    CHI_Memory_Status[mem_idx].no_SEFI_seq++;
                    // jump to next memory
                    return 0;
                }
            }
        }
		
		TIMER3_Disable();
    
        //check page
        page_errors = 0;
        for (uint16_t j = 0; j < mem_arr[mem_idx].page_size; j++) {
			
		
            if (buf[j] != pattern[ptr_idx]) {

				if (CHI_Board_Status.latch_up_detected==1) return 0xAC;

                page_errors++;
                CHI_Board_Status.mem_reprog |= (1<<mem_idx);
                CHI_Memory_Status[mem_idx].no_SEU++;
				
                // page_number*pagesize + address in page
                addr = (uint32_t) i*mem_arr[mem_idx].page_size + j; //calculate the address of the SEU

                if (page_errors == mem_arr[mem_idx].page_size-1) {
                    // remove the last page_size errors and store a SEFI
					CHI_Memory_Status[mem_idx].no_SEU -= page_errors;
					CHI_Memory_Status[mem_idx].no_SEFI_wr_error++;
					CHI_Memory_Status[mem_idx].no_SEFI_seq++;
					CHI_Board_Status.Event_cnt -= (page_errors-1);
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
	  CHI_Board_Status.mem_to_test = 0x0800;
    CHI_Board_Status.mem_reprog = 0;
	  CHI_Board_Status.no_cycles = 0;
		CHI_Board_Status.Event_cnt = 0; // EVENT counter
		
		CHI_UART_RX_BUFFER_INDEX=0;
		CHI_UART_RX_BUFFER_COUNTER=0;
		
		// clear all statistics, for loop
		for (uint8_t i=0;i<12;i++) {
			CHI_Memory_Status[i].no_SEU=0;
			CHI_Memory_Status[i].no_LU=0;
			CHI_Memory_Status[i].no_SEFI_timeout=0;
			CHI_Memory_Status[i].no_SEFI_wr_error=0;
			CHI_Memory_Status[i].no_SEFI_seq=0;
			CHI_Memory_Status[i].cycles=0;
		}
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
  wait_2ms(); // FM25W256 has a  minimum powerup time of 1ms
	
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
		
		do {
				
		CHI_Board_Status.no_cycles++; // increase number of memory cycles
				
			for (int i=0;i<12;i++) {	
				if (CHI_Board_Status.mem_to_test & (1<<i)) {
					
					if ((CHI_Memory_Status[i].no_LU) > 50 )	{
						// exclude the memory from the test if LU > 50 TBD
						CHI_Board_Status.mem_to_test&=~(1<<i);
					}

					if ((CHI_Memory_Status[i].no_SEFI_seq)>254)	{
						// exclude the memory from the test if SEFI > 10 TBD
						CHI_Board_Status.mem_to_test&=~(1<<i);
					}					
					
					if (CHI_Board_Status.mem_reprog & (1<<i))	{
					
						LDO_ON;
            wait_2ms(); // FM25W256 has a  minimum powerup time of 1ms
						
						TIMER3_Enable_8s();
						while (erase_chip(i) == BUSY) {
							if (CHI_Board_Status.SPI_timeout_detected==1) {
								CHI_Memory_Status[i].no_SEFI_timeout++;
								CHI_Memory_Status[i].no_SEFI_seq++;
								break;
							}	
						}
						TIMER3_Disable();
						if (CHI_Board_Status.SPI_timeout_detected==1) continue;

						
						if (CHI_Board_Status.latch_up_detected==1) {
							
							// Wait 1 second after the latch-up
              wait_1s();
							
							CHI_Memory_Status[i].no_LU++;
							CHI_Board_Status.latch_up_detected=0;
							continue;
						}
						
						addr = 0;
						pattern = 0;
						for (uint16_t j = 0; j < mem_arr[i].page_num; j++) {
							
							TIMER3_Enable_1s();							
              if(mem_arr[i].addr_space) {
                while (write_24bit_page(addr, pattern, i) == BUSY) {
                  if (CHI_Board_Status.SPI_timeout_detected==1) {
                    CHI_Memory_Status[i].no_SEFI_timeout++;
                    CHI_Memory_Status[i].no_SEFI_seq++;
                    break;
                  }								
                }
              } else {
                while (write_16bit_page(addr, pattern, i) == BUSY) {
                  if (CHI_Board_Status.SPI_timeout_detected==1) {
                    CHI_Memory_Status[i].no_SEFI_timeout++;
                    CHI_Memory_Status[i].no_SEFI_seq++;
                    break;
                  }								
                }
              }
							TIMER3_Disable();
														
							if (CHI_Board_Status.SPI_timeout_detected==1) break;
							
							addr+=mem_arr[i].page_size;
							pattern ^= 0x01;
							
							if (CHI_Board_Status.latch_up_detected==1) break;
						}
						
						if (CHI_Board_Status.SPI_timeout_detected==1) continue;
											
						if (CHI_Board_Status.latch_up_detected==1) {
                wait_1s(); 
								
								CHI_Memory_Status[i].no_LU++;
								CHI_Board_Status.latch_up_detected=0;
								continue;
						}						
						
						CHI_Board_Status.mem_reprog &= ~ (1<<i); // clear reporgramming flag	
						CHI_Memory_Status[i].no_SEFI_seq=0;					
					}
				}
			}

			for(uint8_t i=0;i<12;i++) {
				if (CHI_Board_Status.mem_to_test & (1<<i)) {
				
					CHI_Board_Status.current_memory=i; // not used at this moment
					
					// Test memory (test procedure defined by device_mode(read only, write read, etc.))
					switch  (CHI_Board_Status.device_mode ) {
						case 0x01: //readmode
						
							LDO_ON;
              wait_2ms(); // FM25W256 has a  minimum powerup time of 1ms
							if (read_memory(i) == 0xAC) {
								
                wait_1s();
																
								CHI_Memory_Status[i].no_LU++;
								CHI_Board_Status.latch_up_detected=0;								
							}
							else if (CHI_Board_Status.latch_up_detected==1) {
								
                wait_1s();
								
								CHI_Memory_Status[i].no_LU++;
								CHI_Board_Status.latch_up_detected=0;
							}							

							break;

						case 0x02:
							//erase_read_write(mem_arr[i]);
							break;

						default:
							read_memory(i);
							break;
					}
					CHI_Memory_Status[i].cycles++;
				}
			}
			
		}
		while ((CHI_Board_Status.local_time-start_time)<60000);

		transmit_CHI_SCI_TM();
    }
}
