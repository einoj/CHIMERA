from random import randint

#calculate crc8
RMAP_CRCTable = [0x00, 0x91, 0xe3, 0x72, 0x07, 0x96, 0xe4, 0x75, 
0x0e, 0x9f, 0xed, 0x7c, 0x09, 0x98, 0xea, 0x7b, 
0x1c, 0x8d, 0xff, 0x6e, 0x1b, 0x8a, 0xf8, 0x69, 
0x12, 0x83, 0xf1, 0x60, 0x15, 0x84, 0xf6, 0x67, 
0x38, 0xa9, 0xdb, 0x4a, 0x3f, 0xae, 0xdc, 0x4d, 
0x36, 0xa7, 0xd5, 0x44, 0x31, 0xa0, 0xd2, 0x43, 
0x24, 0xb5, 0xc7, 0x56, 0x23, 0xb2, 0xc0, 0x51, 
0x2a, 0xbb, 0xc9, 0x58, 0x2d, 0xbc, 0xce, 0x5f, 
0x70, 0xe1, 0x93, 0x02, 0x77, 0xe6, 0x94, 0x05, 
0x7e, 0xef, 0x9d, 0x0c, 0x79, 0xe8, 0x9a, 0x0b, 
0x6c, 0xfd, 0x8f, 0x1e, 0x6b, 0xfa, 0x88, 0x19, 
0x62, 0xf3, 0x81, 0x10, 0x65, 0xf4, 0x86, 0x17, 
0x48, 0xd9, 0xab, 0x3a, 0x4f, 0xde, 0xac, 0x3d, 
0x46, 0xd7, 0xa5, 0x34, 0x41, 0xd0, 0xa2, 0x33, 
0x54, 0xc5, 0xb7, 0x26, 0x53, 0xc2, 0xb0, 0x21, 
0x5a, 0xcb, 0xb9, 0x28, 0x5d, 0xcc, 0xbe, 0x2f, 
0xe0, 0x71, 0x03, 0x92, 0xe7, 0x76, 0x04, 0x95, 
0xee, 0x7f, 0x0d, 0x9c, 0xe9, 0x78, 0x0a, 0x9b, 
0xfc, 0x6d, 0x1f, 0x8e, 0xfb, 0x6a, 0x18, 0x89, 
0xf2, 0x63, 0x11, 0x80, 0xf5, 0x64, 0x16, 0x87, 
0xd8, 0x49, 0x3b, 0xaa, 0xdf, 0x4e, 0x3c, 0xad, 
0xd6, 0x47, 0x35, 0xa4, 0xd1, 0x40, 0x32, 0xa3, 
0xc4, 0x55, 0x27, 0xb6, 0xc3, 0x52, 0x20, 0xb1, 
0xca, 0x5b, 0x29, 0xb8, 0xcd, 0x5c, 0x2e, 0xbf, 
0x90, 0x01, 0x73, 0xe2, 0x97, 0x06, 0x74, 0xe5, 
0x9e, 0x0f, 0x7d, 0xec, 0x99, 0x08, 0x7a, 0xeb, 
0x8c, 0x1d, 0x6f, 0xfe, 0x8b, 0x1a, 0x68, 0xf9, 
0x82, 0x13, 0x61, 0xf0, 0x85, 0x14, 0x66, 0xf7, 
0xa8, 0x39, 0x4b, 0xda, 0xaf, 0x3e, 0x4c, 0xdd, 
0xa6, 0x37, 0x45, 0xd4, 0xa1, 0x30, 0x42, 0xd3, 
0xb4, 0x25, 0x57, 0xc6, 0xb3, 0x22, 0x50, 0xc1, 
0xba, 0x2b, 0x59, 0xc8, 0xbd, 0x2c, 0x5e, 0xcf 
]

def RMAP_CalculateCRC(INCRC, INBYTE):
    return RMAP_CRCTable[INCRC ^ INBYTE]

def generate_packet(good):
    FEND  = 0xC0
    FESC  = 0xDB
    TFEND = 0xDC
    TFESC = 0xDD

    #generate data list
    data = [randint(0,255) for i in range(randint(1,100))]
    originaldata = data

    # The checksum needs to be generated before KISS encoding, this is because the CRC might equal 
    # FEND, FESC, TFEND or TFESC, which needs to be KISS encoded
    checksum = 0
    for i in data:
       checksum = RMAP_CalculateCRC(checksum, i) 

    #corrupt the dataframe by changing a few values
    #The produced frame can then be used to test that the crc8 function detects the error
    if (good != True):
        for i in range(5):
            data[randint(0,len(data)-1)] = randint(0,255)

    data.append(checksum);

    # translate FEND and FESC bytes to FESC TFEND  FESC TFESC
    for i in range(len(data)):
        if data[i] == FEND:
            data[i] = TFEND
            data.insert(i, FESC)
        elif data[i] == FESC:
            data[i] = TFESC
            data.insert(i, FESC)



    # Insert frame beginning and end
    data.insert(0,FEND)
    data.append(FEND)
    
    return data, originaldata

if __name__ == "__main__":
    nFrames = 100
    binaryfile = open("goodframes.bin", "wb")
    originalbinaryfile = open("originaldata.bin", "wb")
    binaryfile.write(bytearray([nFrames]))
    originalbinaryfile.write(bytearray([nFrames]))
    for i in range(nFrames):
        data, originaldata = generate_packet(True)
        binaryfile.write(bytearray([len(data)]))
        originalbinaryfile.write(bytearray([len(originaldata)]))
        binaryfile.write(bytearray(data))
        originalbinaryfile.write(bytearray(originaldata))

    binaryfile = open("badframes.bin", "wb")
    binaryfile.write(bytearray([nFrames]))
    for i in range(nFrames):
        data, originaldata = generate_packet(False)
        binaryfile.write(bytearray([len(data)]))
        binaryfile.write(bytearray(data))
