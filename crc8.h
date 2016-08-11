/* 
----------------------------------------------------------------------------- 
-- Cyclic Redundancy Code (CRC) for Remote Memory Access Protocol (RMAP) 
----------------------------------------------------------------------------- 
-- Purpose: 
-- Given an intermediate SpaceWire RMAP CRC byte value and an RMAP Header 
-- or Data byte, return an updated RMAP CRC byte value. 
-- 
-- Parameters: 
-- INCRC - The intermediate RMAP CRC byte value. 
-- INBYTE - The RMAP Header or Data byte. 
-- 
-- Return value: 
-- OUTCRC - The updated RMAP CRC byte value. 
-- 
-- Description: 
-- Table look-up version: uses the XOR of the intermediate CRC byte with the 
-- header/data byte to obtain the updated CRC byte from a look-up table. 
-- 
-- Generator polynomial: g(x) = x**8 + x**2 + x**1 + x**0 
-- 
-- Notes: 
-- The INCRC input CRC value must have all bits zero for the first INBYTE. 
-- 
-- The first INBYTE must be the first Header or Data byte covered by the 
-- RMAP CRC calculation. The remaining bytes must be supplied in the RMAP 
-- transmission/reception byte order. 
-- 
-- If the last INBYTE is the last Header or Data byte covered by the RMAP 
-- CRC calculation then the OUTCRC output will be the RMAP CRC byte to be 
-- used for transmission or to be checked against the received CRC byte. 
-- 
-- If the last INBYTE is the Header or Data CRC byte then the OUTCRC 
-- output will be zero if no errors have been detected and non-zero if 
-- an error has been detected. 
-- 
-- Each byte is inserted in or extracted from a SpaceWire packet without 
-- the need for any bit reversal or similar manipulation. The SpaceWire 
-- packet transmission and reception procedure does the necessary bit 
-- ordering when sending and receiving Data Characters (see ECSS-E-ST-50-12). 
----------------------------------------------------------------------------- 
*/ 
uint8_t RMAP_CalculateCRC(uint8_t INCRC, uint8_t INBYTE);


