#define NUM_SEU 0x01
#define T_STAMP 0x02
#define DT_STAMP 0x03
#define ADDR_SEU_AA 0x04 
#define ADDR_SEU_55 0x05 
#define UPPER_NIB  28
#define LOWER_NIB  24

void USART1SendByte(uint8_t u8Data);

typedef struct {
 uint16_t index;
 uint32_t data[500];
} data_packet;

/* Put data into a data packet
 * @packet, pointer to a data_packet struct into which data is to be written
 * @data, the data to be written, can be: timestamp, delta timestamp, address, number of upsets
 * @memid, a number between 1 and 12 identifying the memory
 * @dataid, number idenfigying the data type, upset, timestamp etc.*/
void put_data(data_packet *packet, uint32_t data, uint8_t memid, uint8_t dataid) {
    packet->data[packet->index] = (uint32_t)dataid << UPPER_NIB;
    packet->data[packet->index] |= (uint32_t)memid << LOWER_NIB;
    packet->data[packet->index] |= (0x00ffffff & data);
    packet->index++;
}

void send_packet(data_packet *packet) {
    uint16_t i;
    for (i=0; i<packet->index; i++) {
        USART1SendByte((uint8_t)(packet->data[i]>>24));
        USART1SendByte((uint8_t)(packet->data[i]>>16));
        USART1SendByte((uint8_t)(packet->data[i]>>8));
        USART1SendByte((uint8_t)(packet->data[i]));
    }
}
