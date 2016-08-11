// KISS TNC Commands
#define DATA 0x00
#define TIME 0x0F
#define MODE 0x0C
#define ACK  0x09
#define NAK  0x0A

// KISS TNC Frame format characters
#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

int check_data(uint8_t* dataframe);
