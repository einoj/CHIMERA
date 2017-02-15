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
#include "spi_tests.h"

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
static void Power_On_Check() {

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
    uint8_t page_SEUs; //SEU errors
    uint8_t page_MBUs; //SEU errors
    //uint32_t addr;

	CHI_Memory_Status[mem_idx].current1=ADC_Median>>2; // Reading bias current measurement

    for (uint32_t i = 0; i < mem_arr[mem_idx].page_num; i++) {		
        //reset timer
		// ENABLE TIMER
		TIMER3_Enable_1s();

        // read page either with 24 bit address or 16 bit address
        if(mem_arr[mem_idx].addr_space != 0) {
            while (read_24bit_page(i*mem_arr[mem_idx].page_size, mem_idx, buf) == BUSY) {

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
            while (read_16bit_page(i*mem_arr[mem_idx].page_size, mem_idx, buf) == BUSY) {

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
        page_MBUs = 0;
        page_SEUs = 0;
		ptr_idx = 0;
        for (uint32_t j = 0; j < mem_arr[mem_idx].page_size; j++) {
	
            if ((buf[j] ^ pattern[ptr_idx]) != 0) {
                if (CHI_Board_Status.latch_up_detected==1) return 0xAC;

                buf[j] ^= pattern[ptr_idx];
                if (buf[j] == 0x80 || buf[j] == 0x40 || buf[j] == 0x20 || buf[j] == 0x10 || buf[j] == 0x08 || buf[j] == 0x04 || buf[j] == 0x02 || buf[j] == 0x01) {
                    page_SEUs++;
                    CHI_Board_Status.mem_reprog |= (1<<mem_idx);
                    CHI_Memory_Status[mem_idx].no_SEU++;
                    
                } else {

                    page_MBUs++;
                    CHI_Board_Status.mem_reprog |= (1<<mem_idx);
                    CHI_Memory_Status[mem_idx].no_MBU++;
                    
                    // page_number*pagesize + address in page
                    //addr = i*mem_arr[mem_idx].page_size + j; //calculate the address of the SEU

                    // WARNING THERE IS PROBABLY A WAY THAT THIS CAN CAUSE OUTOF BOUNDS WRITES
                }
                if (page_SEUs+page_MBUs > 15) {
                  // remove the last page_size errors and store a SEFI
                  CHI_Memory_Status[mem_idx].no_SEU -= page_SEUs;
                  CHI_Memory_Status[mem_idx].no_MBU -= page_MBUs;
                  CHI_Memory_Status[mem_idx].no_SEFI_wr_error++;
                  CHI_Memory_Status[mem_idx].no_SEFI_seq++;
                  //CHI_Board_Status.Event_cnt -= CHI_NUM_EVENT-1;
                 // if (CHI_Board_Status.Event_cnt > CHI_NUM_EVENT-1) { //OVERFLOW 
                 //  CHI_Board_Status.Event_cnt = 0;  // All stored data now delted
                 // }

                 // Memory_Events[CHI_Board_Status.Event_cnt].timestamp = CHI_Board_Status.local_time;
                 // Memory_Events[CHI_Board_Status.Event_cnt].memory_id = 0x80 | mem_idx; //1 in upper memory bit signifies a SEFI
                 // Memory_Events[CHI_Board_Status.Event_cnt].addr1 = (uint8_t) (addr);
                 // Memory_Events[CHI_Board_Status.Event_cnt].addr2 = (uint8_t) (addr>>8);
                 // Memory_Events[CHI_Board_Status.Event_cnt].addr3 = (uint8_t) (addr>>16);
                 // Memory_Events[CHI_Board_Status.Event_cnt].value = buf[j];
                 // CHI_Board_Status.Event_cnt++;
                  return 1;
                }

                /*
               // else if (CHI_Board_Status.Event_cnt < CHI_NUM_EVENT) {
               //   Memory_Events[CHI_Board_Status.Event_cnt].timestamp = CHI_Board_Status.local_time;
               //   Memory_Events[CHI_Board_Status.Event_cnt].memory_id = mem_idx;
               //   Memory_Events[CHI_Board_Status.Event_cnt].addr1 = (uint8_t) (addr);
               //   Memory_Events[CHI_Board_Status.Event_cnt].addr2 = (uint8_t) (addr>>8);
               //   Memory_Events[CHI_Board_Status.Event_cnt].addr3 = (uint8_t) (addr>>16);
               //   Memory_Events[CHI_Board_Status.Event_cnt].value = buf[j];
               //   CHI_Board_Status.Event_cnt++;
               // }
                
                else {
                    //TODO transmit data when EVENT table if full
                    }
                    */
            }
            ptr_idx ^= 0x01;
        }
    }
					
	CHI_Memory_Status[mem_idx].no_SEFI_seq=0;
    return 0;
}

void Power_On_Init() {
    CHI_Board_Status.device_mode = 0x01;
    CHI_Board_Status.latch_up_detected = 0;
    CHI_Board_Status.mem_to_test = 0x0FFF;//0x0FC7;// 0b0000000001000000;// 0x01C0; // 0x0FA7 9 memories
    CHI_Board_Status.mem_reprog = 0x0000;
    CHI_Board_Status.no_cycles = 0;
    CHI_Board_Status.program_sram = 1;	// The SRAMs need to be reprogrammed when set to mode 2, this variable will be set to 1 when the mode changes
    CHI_Board_Status.delta_time = 0;
    CHI_Board_Status.delta_mode=0;
    CHI_Board_Status.delta_mem_to_test=0;
    //CHI_Board_Status.Event_cnt = 0; // EVENT counter

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

void SPI_CYCLE() {
    // Pull CS pins down
    for (uint8_t i = 0; i < 12; i++)CHIP_SELECT(i);		

    // Pull SPI down
    SPCR &= ~(1<<SPE);
    PORTB &= 0b11110001; // clear SCK/MOSI

    wait_1s();
    wait_1s();

    // Pull CS pins up
    for (uint8_t i = 0; i < 12; i++)CHIP_DESELECT(i);
    SPCR |= (1<<SPE);	
}

// returns -1 means continue repogram for loop continue 
int8_t reprogram_memory(uint8_t i) {
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
    if (CHI_Board_Status.SPI_timeout_detected==1) return -1;


    if (CHI_Board_Status.latch_up_detected==1) {

        // Wait 1 second after the latch-up
        SPI_CYCLE();
        CHI_Memory_Status[i].no_LU++;
        CHI_Board_Status.latch_up_detected=0;
        return -1;
    }

    for (uint32_t j = 0; j < mem_arr[i].page_num; j++) {

        TIMER3_Enable_1s();							
        if(mem_arr[i].addr_space) {
            while (write_24bit_page(j*mem_arr[i].page_size, i) == BUSY) {
                if (CHI_Board_Status.SPI_timeout_detected==1) {
                    CHI_Memory_Status[i].no_SEFI_timeout++;
                    CHI_Memory_Status[i].no_SEFI_seq++;
                    break;
                }								
            }
        } else {
            while (write_16bit_page(j*mem_arr[i].page_size, i) == BUSY) {
                if (CHI_Board_Status.SPI_timeout_detected==1) {
                    CHI_Memory_Status[i].no_SEFI_timeout++;
                    CHI_Memory_Status[i].no_SEFI_seq++;
                    break;
                }								
            }
        }
        TIMER3_Disable();

        if (CHI_Board_Status.SPI_timeout_detected==1) break;

        if (CHI_Board_Status.latch_up_detected==1) break;
    }

    if (CHI_Board_Status.SPI_timeout_detected==1) return -1;

    if (CHI_Board_Status.latch_up_detected==1) { 
        SPI_CYCLE();
        CHI_Memory_Status[i].no_LU++;
        CHI_Board_Status.latch_up_detected=0;
        return -1;
    }						

    CHI_Board_Status.mem_reprog &= ~ (1<<i); // clear reprogramming flag	
    CHI_Memory_Status[i].no_SEFI_seq=0;					
	return 0;
}

int main(void)
{
	Power_On_Check();	// check what was the cause of reset
	
	OSCCAL=0xA1; // clock calibration
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
        //enable_memory_vcc(mem_arr[i]);
        disable_memory_vcc(mem_arr[i]);
    }

	/* Main Loop */
    while (1) 
    {	
		start_time=CHI_Board_Status.local_time;
        switch  (CHI_Board_Status.device_mode ) {
            case 0x01: //read mode
                // Power off all memories, as they will be turned on individually
                for (uint8_t i=0;i<12;i++) {	
                    disable_memory_vcc(mem_arr[i]);
                }
                break;

            case 0x02: // read several memories without powering them down
                // Power on the memories to be tested
                for (uint8_t i=0;i<12;i++) {	
                    if (CHI_Board_Status.mem_to_test & (1<<i)) {
                        enable_memory_vcc(mem_arr[i]);
                    } else {
                        disable_memory_vcc(mem_arr[i]);
                    }
                }
                wait_2ms();// FM25W256 has a  minimum powerup time of 1ms
                //erase_read_write(mem_arr[i]);
                break;
        }
		do {
				
		CHI_Board_Status.no_cycles++; // increase number of memory cycles
				
        for (uint8_t i=0;i<12;i++) {	
            if (CHI_Board_Status.mem_to_test & (1<<i)) {
                // Power on the memory to Reprogram, just leaving this for mode 2 as it changes nothing
               

                if ((CHI_Memory_Status[i].no_LU) > 50 )	{
                	// exclude the memory from the test if LU > 50 TBD
                	CHI_Board_Status.mem_to_test&=~(1<<i);
                }

                if ((CHI_Memory_Status[i].no_SEFI_seq)>254)	{
                	// exclude the memory from the test if SEFI > 10 TBD
                	CHI_Board_Status.mem_to_test&=~(1<<i);
                }					
                
                // Do not reprogram SRAMs in mode 1 as the data will be lost when they are powered off
                // Reprogramming of SRAMs in mode 1 happens right before checking 
                if (CHI_Board_Status.device_mode == 0x01 && mem_arr[i].sram == 1) {
                    CHI_Board_Status.mem_reprog &= ~ (1<<i); // clear reprogramming flag	
                }

                if (CHI_Board_Status.mem_reprog & (1<<i))	{
                    enable_memory_vcc(mem_arr[i]);
                    wait_2ms(); // FM25W256 has a  minimum powerup time of 1ms
                    if (reprogram_memory(i)){
                      //Encountered a SEFI while writing
                      disable_memory_vcc(mem_arr[i]);
                      continue;
                     }
                }

                // Disable Memory to Reprogram if in mode 0x01
                if(CHI_Board_Status.device_mode == 0x01) {
                    // We need to wait for the last page to finish writing
                    // before disabling the memory
                    TIMER3_Enable_1s();
                    while (check_busy(i) == BUSY) {
                        if (CHI_Board_Status.SPI_timeout_detected==1) {
                            CHI_Memory_Status[i].no_SEFI_timeout++;
                            CHI_Memory_Status[i].no_SEFI_seq++;
                            break;
                        }								
                    }
                    TIMER3_Disable();
                    disable_memory_vcc(mem_arr[i]);
                }
            }
        }

			for(uint8_t i=0;i<12;i++) {
                if (CHI_Board_Status.mem_to_test & (1<<i)) {

                    CHI_Board_Status.current_memory=i; // not used at this moment

                    // Test memory (test procedure defined by device_mode(read only, write read, etc.))
                    LDO_ON;
                    switch  (CHI_Board_Status.device_mode ) {
                        case 0x01: //readmode
                            // Enable the memory to Test 
                            enable_memory_vcc(mem_arr[i]);
                            wait_2ms(); // FM25W256 has a  minimum powerup time of 1ms
                            if (mem_arr[i].sram) {
                                reprogram_memory(i); // if reprogramming fails it will be detected as a sefi later when reading
                            }
                            if (read_memory(i) == 0xAC) {
                                SPI_CYCLE();								
                                CHI_Memory_Status[i].no_LU++;
                                CHI_Board_Status.latch_up_detected=0;								
                            }
                            else if (CHI_Board_Status.latch_up_detected==1) {			
                                SPI_CYCLE();				
                                CHI_Memory_Status[i].no_LU++;
                                CHI_Board_Status.latch_up_detected=0;
                            }							
                            break;

                        case 0x02:
                            if (CHI_Board_Status.program_sram) {	// The SRAMs need to be reprogrammed when set to mode 2, this variable will be set to 1 when the mode changes
                                //reprogram all SRAMS to be tested
                                for(uint8_t j=0;j<12;j++) {
                                    if (CHI_Board_Status.mem_to_test & (1<<j) && mem_arr[j].sram) {
                                        reprogram_memory(j); // if reprogramming fails it will be detected as a sefi later when reading
                                    }
                                }
                                CHI_Board_Status.program_sram = 0;
                            }
                            // test multiple memories without powering them off
                            // read memories like above
                            if (read_memory(i) == 0xAC) {
                                SPI_CYCLE();								
                                CHI_Memory_Status[i].no_LU++;
                                CHI_Board_Status.latch_up_detected=0;								
                            }
                            else if (CHI_Board_Status.latch_up_detected==1) {			
                                SPI_CYCLE();				
                                CHI_Memory_Status[i].no_LU++;
                                CHI_Board_Status.latch_up_detected=0;
                            }							
                            break;

                        default:
                            // IF memory is SRAM REPROGRAM //
                            if (mem_arr[i].sram) {
                                reprogram_memory(i); // if reprogramming fails it will be detected as a sefi later when reading
                            }
                            if (read_memory(i) == 0xAC) {
                                SPI_CYCLE();								
                                CHI_Memory_Status[i].no_LU++;
                                CHI_Board_Status.latch_up_detected=0;								
                            }
                            else if (CHI_Board_Status.latch_up_detected==1) {
	                            SPI_CYCLE();
	                            CHI_Memory_Status[i].no_LU++;
	                            CHI_Board_Status.latch_up_detected=0;
                            }
                            break;
                    }
                    CHI_Memory_Status[i].cycles++;

                    // Disable Memory to Reprogram if in mode 0x01
                    if(CHI_Board_Status.device_mode == 0x01) {
                        disable_memory_vcc(mem_arr[i]);
                    }
                }
			}
			
        } while ((CHI_Board_Status.local_time-start_time)<900000);


        transmit_CHI_SCI_TM();

        // Change the mode only outside of a do-while test loop. This ensures that only the memories that are being tested will be powered on
        if (CHI_Board_Status.delta_mode) {
            CHI_Board_Status.device_mode = CHI_Board_Status.delta_mode;
            CHI_Board_Status.mem_to_test = CHI_Board_Status.delta_mem_to_test;
        }

        /*
        if (CHI_Board_Status.Event_cnt > 0) {
            // transmit_CHI_EVENTS();
            // TODO wait for ACK or resend if NACK?
        }
        */

        
        // UPDATE LOCAL TIMER 
        CHI_Board_Status.local_time += CHI_Board_Status.delta_time;
        CHI_Board_Status.delta_time = 0;
    }
}
