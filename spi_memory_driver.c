#include "memories.h"
#include "spi_memory_driver.h"

void SPI_Init(void) {
    DDRB = 0;
	DDRB = (1<<DDB0)|(1<<DDB1)|(1<<DDB2)|(1<<DDB4)|(1<<DDB5)|(1<<DDB6)|(1<<DDB7);//0xF7;//(1<<MOSI)|(1<<SCK);
    SPCR=(1<<SPE)|(1<<MSTR)|(1<<SPR0);//|(1<<SPIE);

    // Clear the SPIF flag by reading SPSR and SPDR
    SPSR;
    SPDR;
}

uint8_t spi_tx_byte(volatile uint8_t byte) {
    SPDR = byte;
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}

uint8_t read_status_reg(uint8_t *status, uint8_t mem_idx) {
        CHIP_SELECT(mem_idx);
        spi_tx_byte(RDSR);           // Send Read status register opcode.
        *status = spi_tx_byte(0xFF); // get the status register value, by sending 0xFF we avoid toggling the MOSI line.
        CHIP_DESELECT(mem_idx);

        return TRANSFER_COMPLETED;
}

uint8_t spi_command(uint8_t op_code, uint8_t mem_idx)
{
    CHIP_SELECT(mem_idx);
    spi_tx_byte(op_code);
    CHIP_DESELECT(mem_idx);
    return 0;
}

uint8_t read_24bit_page(uint32_t addr, uint8_t mem_idx, uint8_t *buffer) 
{
    uint16_t read_cnt=0;     // WARNING! Should be the same type as page size, to avoid integer overflow when reading
    //uint8_t *buffer_ptr;    // temporary variable for incrementing buffer pointer
    //buffer_ptr = buffer;
    uint16_t page_size = mem_arr[mem_idx].page_size;
    uint8_t status_reg;

    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) { // Is internal write or erase is currently in progress?
        CHIP_SELECT(mem_idx);
        spi_tx_byte(READ);
        spi_tx_byte((uint8_t)(addr>>16)); // Send the MSB byte. Casting to uint8_t will send onlyt the lower byte
        spi_tx_byte((uint8_t)(addr>>8));  // Send the middle byte
        spi_tx_byte((uint8_t)(addr));     // Send the LSB byte
        while (read_cnt < page_size) {
            //*buffer_ptr++ = spi_tx_byte(0xFF);
            buffer[read_cnt] = spi_tx_byte(0xFF);
            //USART0SendByte(buffer[read_cnt]);
            read_cnt++;
        }
        CHIP_DESELECT(mem_idx);
        return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}

uint8_t read_16bit_page(uint32_t addr, uint8_t mem_idx, uint8_t *buffer) 
{
    uint16_t read_cnt=0;     // WARNING! Should be the same type as page size, to avoid integer overflow when reading
    //uint8_t *buffer_ptr;    // temporary variable for incrementing buffer pointer
    //buffer_ptr = buffer;
    uint16_t page_size = mem_arr[mem_idx].page_size;
    uint8_t status_reg;

    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) { // Is internal write or erase is currently in progress?
        CHIP_SELECT(mem_idx);
        spi_tx_byte(READ);
        spi_tx_byte((uint8_t)(addr>>8));  // Send the MSB byte. Casting to uint8_t will send onlyt the lower byte
        spi_tx_byte((uint8_t)(addr));     // Send the LSB byte
        while (read_cnt < page_size) {
            //*buffer_ptr++ = spi_tx_byte(0xFF);
            buffer[read_cnt] = spi_tx_byte(0xFF);
            //USART0SendByte(buffer[read_cnt]);
            read_cnt++;
        }
        CHIP_DESELECT(mem_idx);
        return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}



/** 
 * Write a page to memory size of the page is contained in the mem_arr[i] struct
 * @addr first address of page
 * @pattern each page should have a different pattern, so that a SEFI that adresses the wrong page can
 * be detected. 0 for 0x55 0xAA, 1 for 0xAA 0x55.
 * @mem_idx which of the 12 memories to write to, indexed 0 through 11
 */
uint8_t write_24bit_page(uint32_t addr, uint8_t mem_idx)
{
    uint16_t write_cnt = 0;
    uint16_t page_size = mem_arr[mem_idx].page_size;
    uint8_t pattern[2] = {0x55,0xAA};
    uint8_t i = 0;
    uint8_t status_reg;

    //if (start_addr <= TOP_ADDR) {
    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) { // Is internal write or erase is currently in progress?
        // TODO should test for write protectioin
        spi_command(WREN, mem_idx); // Write enable always has to be sent before a write operation.
        CHIP_SELECT(mem_idx);
        spi_tx_byte(PRG);
        spi_tx_byte((uint8_t)(addr>>16)); // Send the MSB byte. Casting to uint8_t will send onlyt the lower byte
        spi_tx_byte((uint8_t)(addr>>8));  // Send the middle byte
        spi_tx_byte((uint8_t)(addr));     // Send the LSB byte
        while (write_cnt < page_size) {
            spi_tx_byte(pattern[i]);
            i ^= 0x01;
            write_cnt++;
        }
        CHIP_DESELECT(mem_idx);
        return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}

uint8_t write_16bit_page(uint32_t addr, uint8_t mem_idx)
{
    uint16_t write_cnt = 0;
    uint16_t page_size = mem_arr[mem_idx].page_size;
    uint8_t pattern[2] = {0x55,0xAA};
    uint8_t i = 0;
    uint8_t status_reg;

    //if (start_addr <= TOP_ADDR) {
    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) { // Is internal write or erase is currently in progress?
        // TODO should test for write protectioin

        spi_command(WREN, mem_idx); // Write enable always has to be sent before a write operation.
        CHIP_SELECT(mem_idx);
        spi_tx_byte(PRG);
        spi_tx_byte((uint8_t)(addr>>8));  // Send the middle byte
        spi_tx_byte((uint8_t)(addr));     // Send the LSB byte
        while (write_cnt < page_size) {
            spi_tx_byte(pattern[i]);
            i ^= 0x01;
            write_cnt++;
        }
        CHIP_DESELECT(mem_idx);
        return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}

/**
 * Enables the VCC of the memory whose information is stored in the Memory struct mem.
 * This is done by pulling the input pin of the inverter low, which will pull up the correspoding pin
 * on the output of the inverter which is connected to the corresponding memory.
 */
void enable_memory_vcc(struct Memory mem)
{
    disable_pin_macro(*(mem.vcc_port), mem.PIN_VCC);
}

/**
 * Disables the VCC of the memory whose information is stored in the Memory struct mem.
 * This is done by pulling the input pin of the inverter low, which will pull up the correspoding pin
 * on the output of the inverter which is connected to the corresponding memory.
 * TODO Consider using just a macro for less overhead
 */
void disable_memory_vcc(struct Memory mem)
{
    enable_pin_macro(*(mem.vcc_port), mem.PIN_VCC);
}

/** 
 * Get Memory JEDEC ID 
 * 
 * Returns the JEDEC ID of the serial memory.
 */
uint8_t get_jedec_id(uint8_t mem_idx, uint8_t* memID)
{
    uint8_t status_reg;
    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) {// Check if the memory is ready to be used. WIP is set an internal write process is in progress
        //DISABLE_SPI_INTERRUPT;
        //SELECT_SERIAL_MEMORY;
        CHIP_SELECT(mem_idx);
        spi_tx_byte(JEDEC);    // Transmit JEDEC-ID opcode
        spi_tx_byte(0xff);     // Recieve manufacturer ID
        *memID = spi_tx_byte(0xff);     // Memory ID
        //DESELECT_SERIAL_MEMORY;
        // ENABLE_SPI_INTERRUPT;
        CHIP_DESELECT(mem_idx);
        return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}

uint8_t erase_chip(uint8_t mem_idx)
{
    uint8_t status_reg;                                          
    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) {
            spi_command(WREN, mem_idx);    
    //        DISABLE_SPI_INTERRUPT;  
            CHIP_SELECT(mem_idx);
            spi_tx_byte(CHIP_ERASE); 
            CHIP_DESELECT(mem_idx);
            return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}
