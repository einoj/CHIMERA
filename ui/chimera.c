#include <stdio.h>
#include "kiss_tnc.h"

#define BUFFMAX 100
int check_data(uint8_t* dataframe)
{
   while  
}

int chimera(uint8_t* kissframe) 
{

    switch (kissframe[1]) {
        case DATA:
            check_data(&kissframe[2])
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
