#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/crc16.h>
#include "kiss_tnc.h"
//#include "crc8.h" DEPRECTED
#include "uart.h"
#include "main.h"

// Interrupt: UART Data Reception
ISR(USART0_RX_vect)
{
    TCCR0 = 0x07; // turn on TIMER0 to parse/generate time-out
    CHI_UART_RX_BUFFER[CHI_UART_RX_BUFFER_INDEX]=UDR0;
    
    CHI_UART_RX_BUFFER_COUNTER++;
    CHI_UART_RX_BUFFER_INDEX++;
    
    if (CHI_UART_RX_BUFFER_INDEX>CHI_UART_RX_BUFFER_SIZE-1) {
        CHI_UART_RX_BUFFER_INDEX=0;
        CHI_UART_RX_BUFFER_COUNTER=0;
    }
}

// Interruprt: Timer 0 - Main Command Parser
ISR(TIMER0_OVF_vect) {
    uint8_t RX_BUFFER[CHI_UART_RX_BUFFER_SIZE];
    uint8_t RX_i=0;
    uint8_t checksum = 0;
    uint8_t FENDi = 0; //counter of 0xC0 bytes
    uint32_t tmp_time;
    
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
                    checksum=_crc8_ccitt_update(checksum, FEND);
                    RX_BUFFER[RX_i]=FEND;
                    RX_i++; i++;
                }
                else if (CHI_UART_RX_BUFFER[i+1]==TFESC) {
                    checksum=_crc8_ccitt_update(checksum, FESC);
                    RX_BUFFER[RX_i]=FESC;
                    RX_i++; i++;
                }
                else {
                    //error of KISS
                    // return?
                }
            }
            else {
                checksum=_crc8_ccitt_update(checksum, CHI_UART_RX_BUFFER[i]);
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
        if (checksum != 0) {
            // CRC Error
            Send_NACK();
            
            // Clear buffer for next frame
            CHI_UART_RX_BUFFER_COUNTER=0;
            CHI_UART_RX_BUFFER_INDEX=0;
            
            return;
        }
                
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
                    
                    /*
                    case (CHI_COMM_ID_EVENT):
                    transmit_CHI_EVENTS();
                    break;
                    */

                    case (CHI_COMM_ID_SCI_TM):
                    transmit_CHI_SCI_TM();
                    break;                                      
                }
            break;
            
            case (CHI_COMM_ID_TIMESTAMP): // TIMESTAMP, 20ms delay parsing, include that?
            if (RX_i==6) { // Note:AFTER UPDATE OF TIMER MAIN LOOP MIGHT BE AFFECTED !!!!!!!!
                tmp_time=(uint32_t)RX_BUFFER[1]<<24 | (uint32_t)RX_BUFFER[2]<<16 | (uint32_t)RX_BUFFER[3]<<8 | (uint32_t)RX_BUFFER[4];
                CHI_Board_Status.delta_time = tmp_time - CHI_Board_Status.local_time;
                Send_ACK();
            }
            else {
                Send_NACK();
            }
            break;
            
            case (CHI_COMM_ID_STATUS):
            transmit_CHI_STATUS();
            break;
            
            /*
            case (CHI_COMM_ID_EVENT):
            transmit_CHI_EVENTS();
            break;
            */

            case (CHI_COMM_ID_SCI_TM):
            transmit_CHI_SCI_TM();
            break;

            case (CHI_COMM_ID_MODE):
            if (RX_i==5) { // 1st byte: device mode, 2nd 3rd: mem to be tested 
                CHI_Board_Status.delta_mode=RX_BUFFER[1];
                CHI_Board_Status.delta_mem_to_test=(uint16_t) (RX_BUFFER[2]<<8) | (uint16_t) (RX_BUFFER[3]);
                CHI_Board_Status.program_sram = 1;  // The SRAMs need to be reprogrammed when set to mode 2, this variable will be set to 0 when the mode changes
                Send_ACK();
            }
            else {
                Send_NACK();
            }           
            break;

            case (CHI_COMM_ID_RESET):
                Send_ACK();
                WDTCR = (1<<WDE); //Enable watchdog, by default shortest prescaler is set, 14.7ms time-out
                while (1);

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
           }
       } else {
           i++;
           j++;
       }
   }
   return j;
}


/*
void transmit_CHI_EVENTS() {
    uint8_t checksum = 0; // Used to store the crc8 checksum
    uint8_t data; // Used to temporarily hold bytes of multibyte variables
    USART0SendByte(FEND);
    // SEND DATA TYPE
    data = (uint8_t) CHI_COMM_ID_EVENT;
    checksum = _crc8_ccitt_update(checksum,data);
    transmit_kiss(data);
    for (uint16_t i = 0; i < CHI_Board_Status.Event_cnt; i++) {
        // send timestamp 
              data =  Memory_Events[i].timestamp;
        checksum = _crc8_ccitt_update(checksum, data);
        transmit_kiss(data);
        // Send Memory_id
        data =  Memory_Events[i].memory_id;
        checksum = _crc8_ccitt_update(checksum, data);
        transmit_kiss(data);
        // Send address 
        data =  Memory_Events[i].addr1;
        checksum = _crc8_ccitt_update(checksum, data);
        transmit_kiss(data);
        data =  Memory_Events[i].addr2;
        checksum = _crc8_ccitt_update(checksum, data);
        transmit_kiss(data);
        data =  Memory_Events[i].addr3;
        checksum = _crc8_ccitt_update(checksum, data);
        transmit_kiss(data);
        // Send value of upset 
        data =  Memory_Events[i].value;
        checksum = _crc8_ccitt_update(checksum, data);
        transmit_kiss(data);
    }
    //Send checksum
    transmit_kiss(checksum);
    
    //send end of frame
    USART0SendByte(FEND);

    CHI_Board_Status.last_cmd=CHI_COMM_ID_EVENT;
  
  //TODO wait for ack before setting num_events to 0
  CHI_Board_Status.Event_cnt = 0;
}
*/

void transmit_CHI_STATUS() {
    uint8_t checksum = 0; // Used to store the crc8 checksum
    uint8_t data; // Used to temporarily hold bytes of multibyte variables
    
    USART0SendByte(FEND);
    
    // SEND DATA TYPE
    data = (uint8_t) CHI_COMM_ID_STATUS;
    checksum = _crc8_ccitt_update(checksum,data);
    transmit_kiss(data);

    // Send Instrument status
    data = (uint8_t) SOFTWARE_VERSION;
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);
        
    data = (uint8_t) (CHI_Board_Status.reset_type);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.device_mode);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.no_cycles>>8);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) CHI_Board_Status.no_cycles;
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);

    //Send checksum
    transmit_kiss(checksum);
    
    //send end of frame
    USART0SendByte(FEND);

    CHI_Board_Status.last_cmd=CHI_COMM_ID_STATUS;
}

void transmit_CHI_SCI_TM(void)
{
    uint16_t i; // Used for looping over memory status
    uint8_t checksum = 0; // Used to store the crc8 checksum
    uint8_t data; // Used to temporarily hold bytes of multibyte variables

    USART0SendByte(FEND);

    // SEND DATA TYPE
    data = (uint8_t) CHI_COMM_ID_SCI_TM ^ (CHI_Board_Status.device_mode<<4);
    checksum = _crc8_ccitt_update(checksum,data);
    transmit_kiss(data);

    // Send On Board TIME stamp
    data = (uint8_t) (CHI_Board_Status.local_time>>24);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.local_time>>16);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.local_time>>8);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);

    data = (uint8_t) (CHI_Board_Status.local_time);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);
    
    // Send Instrument status
    data = (uint8_t) (CHI_Board_Status.mem_to_test>>8);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);
    
    data = (uint8_t) (CHI_Board_Status.mem_to_test);
    checksum = _crc8_ccitt_update(checksum, data);
    transmit_kiss(data);
    
    // Send memory status of the 12 memories
    for (i = 0; i < NUM_MEMORIES; i++) {
        checksum = _crc8_ccitt_update(checksum, CHI_Memory_Status[i].cycles);
        transmit_kiss(CHI_Memory_Status[i].cycles);     
        
        data  = CHI_Memory_Status[i].no_SEU;
        checksum = _crc8_ccitt_update(checksum, data);
        transmit_kiss(data);
        
        data  = CHI_Memory_Status[i].no_MBU;
        checksum = _crc8_ccitt_update(checksum, data);
        transmit_kiss(data);
        
        checksum = _crc8_ccitt_update(checksum, CHI_Memory_Status[i].no_LU);
        transmit_kiss(CHI_Memory_Status[i].no_LU);
        
        checksum = _crc8_ccitt_update(checksum, CHI_Memory_Status[i].no_SEFI_timeout);
        transmit_kiss(CHI_Memory_Status[i].no_SEFI_timeout);
        
        checksum = _crc8_ccitt_update(checksum, CHI_Memory_Status[i].no_SEFI_wr_error);
        transmit_kiss(CHI_Memory_Status[i].no_SEFI_wr_error);
        
        checksum = _crc8_ccitt_update(checksum, CHI_Memory_Status[i].current1);
        transmit_kiss(CHI_Memory_Status[i].current1);       
    }

    // Send detailed Upset information
    // TODO Should the detailed upset info also include a timestamp?
    
    //Send checksum
    transmit_kiss(checksum);
    
    //send end of frame
    USART0SendByte(FEND);

    CHI_Board_Status.last_cmd=CHI_COMM_ID_SCI_TM;
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
    USART0SendByte(0x46); // precalculated CRC
    USART0SendByte(FEND);
}

// Send ACK, to be moved into proper file
void Send_NACK() 
{
    USART0SendByte(FEND);
    USART0SendByte(CHI_COMM_ID_NACK);
    USART0SendByte(0x6B); // precalculated CRC
    USART0SendByte(FEND);
}
