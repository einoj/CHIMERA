#include <stdio.h>
#include <stdint.h>
#include "kiss_tnc.h"
#include "crc8.h"

#define BUFFMAX 100
/* Decodes dataframe and checks CRC-8 */
int check_data(uint8_t* dataframe)
{
   int i = 0; // Index used for sent dataframe
   int j = 0; // Index used to modify dataframe by merging the FESC, TFEND and TFESC characters
   uint8_t checksum = 0;
   while (dataframe[i] != FEND) {
       printf("temp checksum %d\n", checksum);
       dataframe[j] = dataframe[i];
       checksum = RMAP_CalculateCRC(checksum, dataframe[i]);
       if (dataframe[i] == FESC) {
           i++;
           checksum = RMAP_CalculateCRC(checksum, dataframe[i]);
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
   // at this point the checksum should be equal to zero as the last byte to be input 
   // is the dataframe checksum together with the incremental checksum
   return checksum;
}

int chimera(uint8_t* kissframe) 
{

    switch (kissframe[1]) {
        case DATA:
            check_data(&kissframe[2]);
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
