#include <avr/io.h>
#include <stdint.h>

// EEPROM Write Configuration
void Read_EEPROM_Conf() {
	uint32_t conf1,conf2,conf3;
	
	eeprom_busy_wait();
	
	conf1=eeprom_read_dword((uint32_t *)0);
	conf2=eeprom_read_dword((uint32_t *)128);
	conf3=eeprom_read_dword((uint32_t *)512);

	if (conf1==conf2) {}
	else if (conf2==conf3) {}
	else if (conf1==conf3) {}
	else {}// failure, use default config
	
};

// EEPROM Read Configuration
void Write_EEPROM_Conf(uint32_t conf) {
	
	eeprom_busy_wait();
	
	eeprom_write_dword((uint32_t *)0,conf);
	eeprom_write_dword((uint32_t *)128,conf);
	eeprom_write_dword((uint32_t *)512,conf);
};