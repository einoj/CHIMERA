#include "crc8.h"

uint8_t gen_crc(uint8_t* dataframe)
{
    int i;
    uint8_t checksum = 0;
    for (i = 0; i < len; i++) {
       checksum = RMAP_CalculateCRC(checksum, data[i]);
    }
    return checksum;
}

uint8_t gen_dataframe(uint8_t* data) {

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
