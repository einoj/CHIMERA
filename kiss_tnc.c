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

transmit_detailed_frame(void)
{
    uint16_t i;
    uint8_t checksum = 0;

    USART0SendByte(FEND);

    // Send On Board TIME stamp
    USART0SendByte((uint8_t) CHI_Local_Time);
    USART0SendByte((uint8_t) (CHI_Local_Time>>8));
    USART0SendByte((uint8_t) (CHI_Local_Time>>16));
    
    // Send Instruent status
    
    // Send No. of events 
}
/* Transmit the generated data in a kiss frame. 
 * This means sending FEND at the beginning,
 * Translateing FEND and FESC bytes in the data to
 * FESC TFEND and FESC TFESC respectively. During looping
 * a CRC-8 code has to be generated, which is to be transmittet 
 * at the end of a frame before FEND. if the CRC8 equals FEND or FESC
 * it too needs to be encoded as mentioned above */
uint8_t transmit_kiss(uint8_t* data, uint16_t num_bytes)
{
    uint16_t i;
    uint8_t checksum = 0;

    USART0SendByte(FEND);

    for (i = 0; i < num_bytes; i++) {
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
