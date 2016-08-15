#include "chimera.h"
#include <stdio.h>
#include "kiss_tnc.h"

int main(void) {
    printf("Hallo\n");
   uint8_t frame[9] = {FEND, 0x42, FESC, TFESC, 0x41, 0x40, FESC, TFEND, FEND};
   int i;

   for (i = 1; i < 8; i++) {
       printf("%x, ",frame[i]);
   }

   printf("\n"); 

   printf("Check sum = %d\n", check_data(&frame[1]));
   for (i = 1; i < 8; i++) {
       printf("%x, ",frame[i]);
   }

   printf("\n");
}
