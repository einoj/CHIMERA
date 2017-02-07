// KISS TNC Commands
#define CHI_COMM_ID_ACK 0x1A
#define CHI_COMM_ID_NACK 0x15
#define CHI_COMM_ID_SCI_TM 0x01
#define CHI_COMM_ID_STATUS 0x02
#define CHI_COMM_ID_EVENT 0x04
#define CHI_COMM_ID_MODE 0x07
#define CHI_COMM_ID_TIMESTAMP 0x08
#define CHI_COMM_ID_RESET 0xE0

// KISS TNC Frame format characters
#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

volatile uint8_t CHI_UART_RX_BUFFER_INDEX;
volatile uint8_t CHI_UART_RX_BUFFER_COUNTER;

//uint8_t gen_crc(uint8_t* dataframe, uint16_t len);

void transmit_kiss(uint8_t data);

uint8_t decode_dataframe(uint8_t* dataframe);

void transmit_CHI_SCI_TM(void);

//void transmit_CHI_EVENTS(void);

void transmit_CHI_STATUS(void);

void Send_ACK(void);

void Send_NACK(void);
