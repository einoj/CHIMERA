// KISS TNC Commands
#define STATUS_DATA 0x00
#define SCI_DATA    0x01
#define EVENTS_DATA 0x03
#define TIME 0x0F
#define MODE 0x0C
#define ACK  0x09
#define NAK  0x0A

// KISS TNC Frame format characters
#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

//uint8_t gen_crc(uint8_t* dataframe, uint16_t len);

void transmit_kiss(uint8_t data);

uint8_t decode_dataframe(uint8_t* dataframe);

void transmit_CHI_SCI_TM(void);

void transmit_CHI_EVENTS(uint16_t num_events);

void transmit_CHI_STATUS(void);

void Send_ACK(void);

void Send_NACK(void);
