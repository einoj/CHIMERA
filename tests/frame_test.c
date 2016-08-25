#include "chimera.h"
#include <stdio.h>
#include "kiss_tnc.h"
#include "readbin.h"

int main(void) {
    int i, j;

    unsigned char nFrames;
    unsigned char **frames = readframes("dataframes.bin", &nFrames);
    printf("%d\n",nFrames);
    for(i=1;i<2*nFrames;i+=2){
        printf("packet number %d size %d:\n",(i+1)/2, frames[i-1][0]);
        // test with good frames
        printf("Check sum = %d\n", check_data(&frames[i][1]));
    }

   // test with bad frames

   printf("\n");
}
