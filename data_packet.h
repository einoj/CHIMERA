#define NUM_SEU 0x01
#define T_STAMP 0x02
#define DT_STAMP 0x03
#define ADDR_SEU 0x04 
#define UPPER_NIB  28
#define LOWER_NIB  24

typedef struct {
 uint16_t index;
 uint32_t data[500];
} data_packet;

void put_data(data_packet *packet, uint32_t data, uint8_t memid, uint8_t dataid) {
    packet->data[packet->index] = (dataid << UPPER_NIB);
    packet->data[packet->index] |= (memid << LOWER_NIB);
    packet->data[packet->index] |= (0x00ffffff & data);
    packet->index++;
}

