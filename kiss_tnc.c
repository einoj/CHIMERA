#include <avr/io.h>
#include <avr/interrupt.h>

#include "kiss_tnc.h"
#include "crc8.h"
#include "uart.h"
#include "main.h"

// Interrupt: UART Data Reception
ISR(USART0_RX_vect)
{
	TCCR0 = 0x07; // turn on TIMER0 to parse/generate time-out
	CHI_UART_RX_BUFFER[CHI_UART_RX_BUFFER_INDEX]=UDR0;
	
	CHI_UART_RX_BUFFER_COUNTER++;
	CHI_UART_RX_BUFFER_INDEX++;
	
	if (CHI_UART_RX_BUFFER_INDEX>CHI_UART_RX_BUFFER_SIZE)CHI_UART_RX_BUFFER_INDEX=0;
}

// Interruprt: Timer 0 - Main Command Parser
ISR(TIMER0_OVF_vect) {
	uint8_t RX_BUFFER[10];
	uint8_t RX_i=0;
	uint8_t checksum = 0;
	uint8_t FENDi = 0; //counter of 0xC0 bytes
	TCCR0=0x00; // turn clock off to wait for another UART RX interrupt
	TCNT0=0xFF-CHI_PARSER_TIMEOUT; // We need 50 ticks to get 10ms interrupt
	
	// parser with time-out:
	// Check if there is sens to parse the command
	if (CHI_UART_RX_BUFFER[0]==FEND && CHI_UART_RX_BUFFER_COUNTER>2) {
		
		// removing the KISS overhead/framing
		// and calculating crc
		for (int i=0;i<CHI_UART_RX_BUFFER_COUNTER;i++) {
			if (CHI_UART_RX_BUFFER[i]==FEND ) {FENDi++;}
			else if (CHI_UART_RX_BUFFER[i]==FESC) {
				if (CHI_UART_RX_BUFFER[i+1]==TFEND) {
					RMAP_CalculateCRC(checksum, FEND);
					RX_BUFFER[RX_i]=FEND;
					RX_i++; i++;
				}
				else if (CHI_UART_RX_BUFFER[i+1]==TFESC) {
					RMAP_CalculateCRC(checksum, FESC);
					RX_BUFFER[RX_i]=FESC;
					RX_i++;	i++;
				}
				else {
					//error of KISS
					// return?
				}
			}
			else {
				RMAP_CalculateCRC(checksum, CHI_UART_RX_BUFFER[i]);
				RX_BUFFER[RX_i]=CHI_UART_RX_BUFFER[i];
				RX_i++;
			}
		}
		if (FENDi!=2) {
			Send_NACK();
			
			// Clear buffer for next frame
			CHI_UART_RX_BUFFER_COUNTER=0;
			CHI_UART_RX_BUFFER_INDEX=0;			
			
			return;			
		}
		// CRC Parsing
		// Off for debuging
		/*
		if (checksum != 0) {
			// CRC Error
			Send_NACK();
			
			// Clear buffer for next frame
			CHI_UART_RX_BUFFER_COUNTER=0;
			CHI_UART_RX_BUFFER_INDEX=0;
			
			return;
		}
		*/
				
		switch (RX_BUFFER[0]) {
			
			case (CHI_COMM_ID_ACK): // ACK, set flag that ACK was received
				// Set ACK flag
				CHI_Board_Status.COMM_flags |= 0x01;
			break;
			
			case (CHI_COMM_ID_NACK): // ACK, set flag that ACK was received
				// set NACK flag
				CHI_Board_Status.COMM_flags |= 0x02;
				// redo last command
				switch (CHI_Board_Status.last_cmd) {
					case (CHI_COMM_ID_STATUS):
					transmit_CHI_STATUS();
					break;
					
					case (CHI_COMM_ID_EVENT):
					transmit_CHI_EVENTS(0);
					break;

					case (CHI_COMM_ID_SCI_TM):
					transmit_CHI_SCI_TM();
					break;										
				}
			break;
			
			case (CHI_COMM_ID_TIMESTAMP): // TIMESTAMP, 20ms delay parsing, include that?
			if (RX_i==6) { // Note:AFTER UPDATE OF TIMER MAIN LOOP MIGHT BE AFFECTED !!!!!!!!
				CHI_Board_Status.local_time=(uint32_t)RX_BUFFER[1]<<24 | (uint32_t)RX_BUFFER[2]<<16 | (uint32_t)RX_BUFFER[3]<<8 | (uint32_t)RX_BUFFER[4];
				Send_ACK();
			}
			else {
				Send_NACK();
			}
			break;
			
			case (CHI_COMM_ID_STATUS):
			CHI_Board_Status.last_cmd=RX_BUFFER[0];
			transmit_CHI_STATUS();
			break;
			
			case (CHI_COMM_ID_EVENT):
			CHI_Board_Status.last_cmd=RX_BUFFER[0];
			transmit_CHI_EVENTS(0);
			break;

			case (CHI_COMM_ID_SCI_TM):
			CHI_Board_Status.last_cmd=RX_BUFFER[0];
			transmit_CHI_SCI_TM();
			break;
			
			case (CHI_COMM_ID_MODE):
			if (RX_i==5) { // 1st byte: device mode, 2nd 3rd: mem to be tested 
				CHI_Board_Status.device_mode=RX_BUFFER[1];
				CHI_Board_Status.mem_to_test=(uint16_t)RX_BUFFER[2]<<8 | (uint16_t)RX_BUFFER[3];
				Send_ACK();
			}
			else {
				Send_NACK();
			}			
			break;

			default:
			Send_NACK();
		}
	}
	else {
		// SEND NACK
		Send_NACK();
	}
	
	// Clear buffer for next frame
	CHI_UART_RX_BUFFER_COUNTER=0;
	CHI_UART_RX_BUFFER_INDEX=0;
}

/* Decodes dataframe and checks CRC-8.
 * Returns the length of the data packet 
 */
uint8_t decode_dataframe(uint8_t* dataframe)
{
   int i = 0; // Index used for sent dataframe
   int j = 0; // Index used to modify dataframe by merging the FESC, TFEND and TFESC characters
   uint8_t retval = 0;
   while (dataframe[i] != FEND) {
       //printf("temp checksum %d\n", checksum);
       dataframe[j] = dataframe[i];
       if (dataframe[i] == FESC) {
           i++;
           if (dataframe[i] == TFEND) {
               dataframe[j] = FEND;
               j++;
               i++;
           } else if (dataframe[i] == TFESC) {
               dataframe[j] = FESC; 
               j++;
               i++;
           } else { // ERROR, only TFEND and TFESC allowed after FESC
               i++;
               retval = -1;
           }
       } else {
           i++;
           j++;
       }
   }
   return j;
}


void transmit_CHI_EVENTS(uint16_t num_events) {
    uint16_t i;
    uint8_t checksum = 0; // Used to store the crc8 checksum
    uint8_t data; // Used to temporarily hold bytes of multibyte variables
    USART0SendByte(FEND);
    for (i = 0; i < num_events; i++) {
        // Send Memory_id
        data =  Memory_Events[i].memory_id;
        checksum = RMAP_CalculateCRC(checksum, data);
        transmit_kiss(data);
        // Send address 
        data =  Memory_Events[i].addr1;
        checksum = RMAP_CalculateCRC(checksum, data);
        transmit_kiss(data);
        data =  Memory_Events[i].addr2;
        checksum = RMAP_CalculateCRC(checksum, data);
        transmit_kiss(data);
        data =  Memory_Events[i].addr3;
        checksum = RMAP_CalculateCRC(checksum, data);
        transmit_kiss(data);
        // Send value of upset 
        data =  Memory_Events[i].value;
        checksum = RMAP_CalculateCRC(checksum, data);
        transmit_kiss(data);
    }
    //TODO wait for ack before setting num_events to 0
    //Send checksum
    transmit_kiss(checksum);
    
    //send end of frame
    USART0SendByte(FEND);
}

void transmit_CHI_STATUS() {
	uint16_t i;
	uint8_t checksum = 0; // Used to store the crc8 checksum
	uint8_t data; // Used to temporarily hold bytes of multibyte variables
	
	USART0SendByte(FEND);

    // Send Instrument status
    data = (uint8_t) SOFTWARE_VERSION;
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    	
    data = (uint8_t) (CHI_Board_Status.reset_type);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.device_mode);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) CHI_Board_Status.no_cycles;
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    data = (uint8_t) (CHI_Board_Status.no_cycles>>8);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);

	//Send checksum
	transmit_kiss(checksum);
	
	//send end of frame
	USART0SendByte(FEND);
}

void transmit_CHI_SCI_TM(void)
{
    uint16_t i; // Used for looping over memory status
    uint8_t checksum = 0; // Used to store the crc8 checksum
    uint8_t data; // Used to temporarily hold bytes of multibyte variables

    USART0SendByte(FEND);

    // Send On Board TIME stamp
    data = (uint8_t) CHI_Board_Status.local_time;
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.local_time>>8);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.local_time>>16);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.local_time>>24);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    // Send Instrument status
    data = (uint8_t) CHI_Board_Status.working_memories;
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    data = (uint8_t) (CHI_Board_Status.working_memories>>8);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    // Send No. of LU events 
    data = (uint8_t) CHI_Board_Status.no_LU_detected;
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    data = (uint8_t) (CHI_Board_Status.no_LU_detected>>8);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    // Send No. of SEU events 
    data = (uint8_t) CHI_Board_Status.no_SEU_detected;
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    data = (uint8_t) (CHI_Board_Status.no_SEU_detected>>8);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    // Send No. of SEFI events 
    data = (uint8_t) CHI_Board_Status.no_SEFI_detected;
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);
    
    data = (uint8_t) (CHI_Board_Status.no_SEFI_detected>>8);
    checksum = RMAP_CalculateCRC(checksum, data);
    transmit_kiss(data);

    // Send memory status of the 12 memories
    for (i = 0; i < NUM_MEMORIES; i++) {
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].status);
        transmit_kiss(CHI_Memory_Status[i].status);
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].no_SEU);
        transmit_kiss(CHI_Memory_Status[i].no_SEU);
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].no_SEFI_timeout);
        transmit_kiss(CHI_Memory_Status[i].no_SEFI_timeout);
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].no_SEFI_wr_error);
        transmit_kiss(CHI_Memory_Status[i].no_SEFI_wr_error);
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].current2);
        transmit_kiss(CHI_Memory_Status[i].current1);
    }

    // Send detailed Upset information
    // TODO Should the detailed upset info also include a timestamp?
    
    //Send checksum
    transmit_kiss(checksum);
    
    //send end of frame
    USART0SendByte(FEND);
}
/* 
 * Takes a byte, KISS encodes it if needed, and transmits the KISS encoded data
 */ 
void transmit_kiss(uint8_t data)
{
    if (data == FEND) {
        USART0SendByte(FESC);
        USART0SendByte(TFEND);
    } else if (data == FESC) {
        USART0SendByte(FESC);
        USART0SendByte(TFESC);
    } else {
        USART0SendByte(data);
    }

}

void Send_ACK() 
{
	USART0SendByte(FEND);
	USART0SendByte(CHI_COMM_ID_ACK);
	USART0SendByte(0xF1); // precalculated CRC
	USART0SendByte(FEND);
}

// Send ACK, to be moved into proper file
void Send_NACK() 
{
	USART0SendByte(FEND);
	USART0SendByte(CHI_COMM_ID_NACK);
	USART0SendByte(0x8A); // precalculated CRC
	USART0SendByte(FEND);
}
