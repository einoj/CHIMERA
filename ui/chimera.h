#include <stdint.h>
#include "crc8.h"

int chimera(uint8_t* kissframe);

/* Check CRC-8. This should be done after a data frame has been decoded by CRC-8 */

/* Decode dataframes. Needs to be done before checking the CRC-8 
 * this is beause the CRC might contain FEND FESC TFEND or TFESC commands
 * so the CRC has also been encoded
 */

int check_crc(uint8_t* data, int len);

/* Decodes dataframe and checks CRC-8 returns the length
 * of the data packet
 */
int decode_dataframe(uint8_t* dataframe);
