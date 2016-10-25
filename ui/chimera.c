#include <stdio.h>
#include <stdint.h>
#include "crc8.h"
#include "chimera.h"

#define BUFFMAX 100


/* Check CRC-8. This should be done after a data frame has been decoded by CRC-8 */

/* Decode dataframes. Needs to be done before checking the CRC-8 
 * this is beause the CRC might contain FEND FESC TFEND or TFESC commands
 * so the CRC has also been encoded
 */

uint8_t check_crc(uint8_t* data, int len) 
{
    int i;
    uint8_t checksum = 0;
    for (i = 0; i < len; i++) {
       checksum = RMAP_CalculateCRC(checksum, data[i]);
    }
    return checksum;
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

int chimera(uint8_t* kissframe) 
{

    switch (kissframe[1]) {
        case DATA:
            decode_dataframe(&kissframe[2]);
            break;
        case TIME:
            break;
        case MODE:
            break;
        case ACK:
            break;
        case NAK:
            break;
    }
}
