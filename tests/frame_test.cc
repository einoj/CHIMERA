#include "chimera.h"
#include <limits.h>
#include "gtest/gtest.h"
#include "kiss_tnc.h"
#include "readbin.h"

TEST(FrameTest, Goodframes) {
    unsigned char nFrames;
    unsigned char **frames = readframes("goodframes.bin", &nFrames);
    for(int i=1;i<2*nFrames;i+=2){
        //printf("packet number %d size %d:\n",(i+1)/2, frames[i-1][0]);
        //printf("Check sum = %d\n", check_data(&frames[i][1]));
        //printf("Check sum = %d\n", check_data(&frames[i][1]));
        //printf("Check sum = %d\n", check_data(&frames[i][1]));
        // test with good frames
        EXPECT_EQ(0,               check_data(&frames[i][1]));
    }
    //freeframes(frames, nFrames);

}
//int main(void) {
//    int i, j;
//
//    unsigned char nFrames;
//    unsigned char **frames = readframes("goodframes.bin", &nFrames);
//    printf("%d\n",nFrames);
//    for(i=1;i<2*nFrames;i+=2){
//        printf("packet number %d size %d:\n",(i+1)/2, frames[i-1][0]);
//        // test with good frames
//        printf("Check sum = %d\n", check_data(&frames[i][1]));
//    }
//    freeframes(frames, nFrames);
//
//    frames = readframes("badframes.bin", &nFrames);
//    printf("%d\n",nFrames);
//    for(i=1;i<2*nFrames;i+=2){
//        printf("packet number %d size %d:\n",(i+1)/2, frames[i-1][0]);
//        // test with good frames
//        printf("Check sum = %d\n", check_data(&frames[i][1]));
//    }
//    freeframes(frames, nFrames);
//
//   // test with bad frames
//
//   printf("\n");
//}
