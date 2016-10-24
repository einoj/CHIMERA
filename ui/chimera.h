#include <stdint.h>
#include "crc8.h"

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


int chimera(uint8_t* kissframe);

/* Check CRC-8. This should be done after a data frame has been decoded by CRC-8 */

/* Decode dataframes. Needs to be done before checking the CRC-8 
 * this is beause the CRC might contain FEND FESC TFEND or TFESC commands
 * so the CRC has also been encoded
 */

uint8_t check_crc(uint8_t* data, int len);

uint8_t decode_dataframe(uint8_t* dataframe);
