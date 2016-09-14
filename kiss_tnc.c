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

void transmit_detailed_frame(void)
{
    uint16_t i; // Used for looping over memory status
    uint8_t checksum = 0; // Used to store the crc8 checksum
    uint8_t data; // Used to temporarily hold bytes of multibyte variables

    USART0SendByte(FEND);

    // Send On Board TIME stamp
    data = (uint8_t) CHI_Board_Status.local_time;
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);

    data = (uint8_t) (CHI_Board_Status.local_time>>8);
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);

    data = (uint8_t) (CHI_Board_Status.local_time>>16);
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);

    data = (uint8_t) (CHI_Board_Status.local_time>>24);
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);
    
    // Send Instrument status
    data = (uint8_t) CHI_Board_Status.working_memories;
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);
    
    data = (uint8_t) (CHI_Board_Status.working_memories>>8);
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);
    
    // Send No. of LU events 
    data = (uint8_t) CHI_Board_Status.no_LU_detected;
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);
    
    data = (uint8_t) (CHI_Board_Status.no_LU_detected>>8);
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);
    
    // Send No. of SEU events 
    data = (uint8_t) CHI_Board_Status.no_SEU_detected;
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);
    
    data = (uint8_t) (CHI_Board_Status.no_SEU_detected>>8);
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);
    
    // Send No. of SEFI events 
    data = (uint8_t) CHI_Board_Status.no_SEFI_detected;
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);
    
    data = (uint8_t) (CHI_Board_Status.no_SEFI_detected>>8);
    USART0SendByte(data);
    checksum = RMAP_CalculateCRC(checksum, data);

    // Send memory status of the 12 memories
    for (i = 0; i < NUM_MEMORIES; i++) {
        USART0SendByte(CHI_Memory_Status[i].status);
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].status);
        USART0SendByte(CHI_Memory_Status[i].no_SEU);
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].no_SEU);
        USART0SendByte(CHI_Memory_Status[i].no_SEFI_LU);
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].no_SEFI_LU);
        USART0SendByte(CHI_Memory_Status[i].current1);
        checksum = RMAP_CalculateCRC(checksum, CHI_Memory_Status[i].current2);
    }

    // Send detailed Upset information
    // TODO Should the detailed upset info also include a timestamp?
    
    //Send checksum
    USART0SendByte(checksum);
    
    //send end of frame
    USART0SendByte(FEND);
}
/* Transmit the generated data in a kiss frame. 
 * This means sending FEND at the beginning,
 * Translateing FEND and FESC bytes in the data to
 * FESC TFEND and FESC TFESC respectively. During looping
 * a CRC-8 code has to be generated, which is to be transmitted 
 * at the end of a frame before FEND. if the CRC8 equals FEND or FESC
 * it too needs to be encoded as mentioned above */
uint8_t transmit_kiss(uint8_t* data, uint16_t num_bytes)
{
    uint16_t i;
    uint8_t checksum = 0;

    USART0SendByte(FEND);

    for (i = 0; i < num_bytes; i++) {
		// is this OK? CRC calculated without special characters?
        checksum = RMAP_CalculateCRC(checksum, data[i]);
        if (data[i] == FEND) {
            USART0SendByte(FESC);
            USART0SendByte(TFEND);
        } else if (data[i] == FESC) {
            USART0SendByte(FESC);
            USART0SendByte(TFESC);
        } else {
            USART0SendByte(data[i]);
        }
    }
    if (checksum == FEND) {
        USART0SendByte(FESC);
        USART0SendByte(TFEND);
    } else if (checksum == FESC) {
        USART0SendByte(FESC);
        USART0SendByte(TFESC);
    } else {
        USART0SendByte(checksum);
    }

    USART0SendByte(FEND);
}
