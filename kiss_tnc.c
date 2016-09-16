#include <avr/io.h>
#include "kiss_tnc.h"
#include "crc8.h"
#include "uart.h"
#include "main.h"

/*
uint8_t gen_crc(uint8_t* dataframe)
{
    int i;
    uint8_t checksum = 0;
    for (i = 0; i < len; i++) {
       checksum = RMAP_CalculateCRC(checksum, data[i]);
    }
    return checksum;
}
*/

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
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].no_SEFI_LU);
        transmit_kiss(CHI_Memory_Status[i].no_SEFI_LU);
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
