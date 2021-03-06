#include "chimera.h"
#include <limits.h>
#include "gtest/gtest.h"
#include "readbin.h"

void printframe(uint8_t* data, int len) 
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%d,",data[i]);
    }
    printf("\n");
}

TEST(FrameTest, Goodframes) {
    unsigned char nFrames;
    unsigned char **frames = readframes("goodframes.bin", &nFrames);
    int len;
    //uint8_t expect[] = {1, 2, 3, 42};
    //uint8_t * buffer = expect;
    //uint32_t buffer_size = sizeof(expect) / sizeof(expect[0]);
    //ASSERT_THAT(std::vector<uint8_t>(buffer, buffer + buffer_size), 
    //                    ::testing::ElementsAreArray(expect));
    for(int i=1;i<2*nFrames;i+=2){
        //for(int j=0;j<frames[i-1][0];j++)
        //    printf("%d, ",frames[i][j]);
        //printf("\n");
        len = decode_dataframe(&frames[i][1]);
        //printf("packet number %d size %d:\n",(i+1)/2, frames[i-1][0]);
        //printf("Check sum = %d\n", check_data(&frames[i][1]));
        //printf("Check sum = %d\n", check_data(&frames[i][1]));
        // test with good frames
        EXPECT_EQ(0, check_crc(&frames[i][1],len)) << "Error occured in frame number: " << (i+1)/2;
    }
    freeframes(frames, nFrames);
}

TEST(FrameTest, Badframes) {
    unsigned char nFrames;
    unsigned char **frames = readframes("badframes.bin", &nFrames);
    int len;
    unsigned char crc;
    for(int i=1;i<2*nFrames;i+=2){
        len = decode_dataframe(&frames[i][1]);
        crc = check_crc(&frames[i][1],len);
        EXPECT_TRUE(0 != crc ) << "Error occured in frame number: " << (i+1)/2;
        if (crc == 0) 
            printframe(&frames[i][1],len);
    }
    freeframes(frames, nFrames);
}
