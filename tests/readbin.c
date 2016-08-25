#include <stdio.h>
#include <stdlib.h>
#include "readbin.h"

/* Read example frames stored in file */
unsigned char** readframes(char *filename, unsigned char *nFrames) 
{
    int i, j;
    FILE *ptr;

    ptr = fopen(filename,"rb");  // r for read, b for binary
    if (ptr == NULL) {
          fprintf(stderr, "Can't open input file %s!\n",filename);
          exit(1);
    }
    
    fread(nFrames,sizeof(char),1,ptr); 
    //fread(buffer,sizeof(buffer),1,ptr); // read 10 bytes to our buffer
    printf("%d\n",*nFrames);

    // create 2*nFrames pointer to load the frames from file
    // and the size of the frames. We will load frame size in index 0 then first frame in index 1
    // upto the last frame size in n-2 and the last frame at index n-1
    unsigned char **buffer = malloc(2*(*nFrames)*sizeof(unsigned char *));

    for(i=1;i<2*(*nFrames);i+=2){
        buffer[i-1] = (unsigned char *) malloc(sizeof(unsigned char));
        fread(&buffer[i-1][0],sizeof(char),1,ptr);
        printf("%d\n",buffer[i-1][0]);
        buffer[i] = (unsigned char *) malloc(buffer[i-1][0]*sizeof(unsigned char));
        fread(buffer[i], sizeof(unsigned char), buffer[i-1][0], ptr);
        for (j = 0; j < buffer[i-1][0]; j++) {
        }
    }
    return buffer;
}

//int main(void) {
//    int i, j;
//
//    unsigned char nFrames;
//    unsigned char **buffer = readframes("dataframes.bin", &nFrames);
//    printf("%d\n",nFrames);
//    for(i=1;i<2*nFrames;i+=2){
//        printf("packet number %d size %d:\n",(i+1)/2, buffer[i-1][0]);
//        for (j = 0; j < buffer[i-1][0]; j++) {
//            printf("%d ", buffer[i][j]);
//        }
//            printf("\n");
//    }
//}
