#!/usr/bin/env python
# encoding: utf-8
"""
Example code to interface the Bus Pirate in binary mode
Brent Wilkins 2015

This code requires pyserial:
    $ sudo pip install pyserial
or:
    $ sudo easy_install -U pyserial
"""
import sys
import serial
import argparse
import logging
from time import sleep


# logger constants
LOG_LEVEL = logging.DEBUG
LOG_FORMAT = logging.Formatter('%(asctime)s %(name)-12s %(levelname)-8s %(message)s')

#KISS constants
FEND = chr(0xC0)
FESC = chr(0xDB)
TFEND = chr(0xDC)
TFESC = chr(0xDD)

#buspirate commands
commands = {
        'BBIO1': b'\x00',    # Enter reset binary mode
        'SPI1':  b'\x01',    # Enter binary SPI mode
        'I2C1':  b'\x02',    # Enter binary I2C mode
        'ART1':  b'\x03',    # Enter binary UART mode
        '1W01':  b'\x04',    # Enter binary 1-Wire mode
        'RAW1':  b'\x05',    # Enter binary raw-wire mode
        'RESET': b'\x0F',    # Reset Bus Pirate
        'STEST': b'\x10',    # Bus Pirate self-tests
        'BRIDGE': b'\x0F',    # When in UART mode, configures the bus pirate as a transparent uart bridge
        'RX': b'\x02',
        # Set the UART at a preconfigured speed value: 0000=300, 0001=1200, 0010=2400,0011=4800,0100=9600,0101=19200,0110=31250 (MIDI), 0111=38400,1000=57600,1010=115200 
        'SPEED': b'\x67'
}

def arg_auto_int(x):
    return int(x, 0)

class FatalError(RuntimeError):
    def __init__(self, message):
        RuntimeError.__init__(self, message)

class KISS(object):
    

    _logger = logging.getLogger(__name__)
    if not _logger.handlers: #create the handlers only once, to avoid duplicate logging
        _logger.setLevel(LOG_LEVEL)
        _console_handler = logging.StreamHandler()
        _console_handler.setLevel(LOG_LEVEL)
        _console_handler.setFormatter(LOG_FORMAT)
        _logger.addHandler(_console_handler)
        _logger.propagate = False


    def __init__(self, port=None, speed=None, pirate=False):
        """
        Initialises the serial interface

        : param port: the port to connect to serial device
        : param speed: the baud rate of the serial interface, for the bus pirate this is 115200, 
            the commuication behind the buspirate can be changed
        """
        self.port = port
        self.speed = speed
        self.interface = None
        self.interface_mode = None
        self._logger.debug("%s","INITIALIZING")

        if pirate == True and self.port is not None and self.speed is not None:
            self.interface_mode = 'buspirate'
        elif self.port is not None and self.speed is not None:
            self.interface_mode = 'serial'
    
    def start(self):
        try:
            self.interface = serial.Serial(self.port, self.speed, timeout=0.1)
        except Exception as e:
            print('I/O error({0}): {1}'.format(e.errno, e.strerror))
            print('Port cannot be opened')

        if 'buspirate' in self.interface_mode:
            # Setup bus pirate to serial bridge mode
            self.enter_buspirate_binarymode()
            
    def enter_buspirate_binarymode(self):
        self._logger.debug('Entering binary mode...\n')
        count = 0
        done = False
        while count < 20 and not done:
            count += 1
            self.interface.write(commands.get('BBIO1'))
            got = self.interface.read(5)  # Read up to 5 bytes
            if got == b'BBIO1':
                self._logger.debug('Entered binary mode!')
                done = True
        if not done:
            self.interface.close()
            self._logger.debug("got " + str(got))
            raise FatalError('Buspirate failed to enter binary mode')

        self.interface.write(commands.get('ART1'))
        got = self.interface.read(4)
        if got == b'ART1':
            self._logger.debug('Entered UART mode')
        else:
            raise FatalError('Buspirate failed to enter UART mode')
        
        self.interface.write(commands.get('SPEED'))
        got = self.interface.read(4)
        if got == b'\x01':
            self._logger.debug('Changed speed to 38400')
        else:
            self._logger.debug(got)
            raise FatalError('Buspirate failed to change speed')

        self.interface.write(commands.get('RX'))
        got = self.interface.read(1)
        self._logger.debug(got)
        
        # Entering Bridge mode
        self.interface.write(commands.get('BRIDGE'))
        got = self.interface.read(1)
        self._logger.debug(got)
            
    def read(self):
        read_buffer = ""
        while True:
            read_data = None
            read_data = self.interface.read(1)
            waiting_data = self.interface.inWaiting()
            if waiting_data:
                read_data = ''.join([read_data, self.interface.read(waiting_data)])

            if read_data is not None:
                frames = [ ] 
                split_data = read_data.split(FEND) 
                len_fend = len(split_data)
                self._logger.debug(len_fend)

                # No FEND in frame
                if len_fend == 1:
                    read_buffer = ''.join([read_buffer, split_data[0]])
                # Single FEND in frame
                elif len_fend == 2:
                # Closing FEND found
                    if split_data[0]:
                        # Partial frame continued, otherwise drop
                        frames.append(''.join([read_buffer, split_data[0]]))
                        read_buffer = ''
                    # Opening FEND found
                else:
                    frames.append(read_buffer)
                    read_buffer = split_data[1] 
               # At least one complete frame received
           elif len_fend >= 3:
               for i in range(0, len_fend - 1):
                   _str = ''.join([read_buffer, split_data[i]])
                     if _str:
                         frames.append(_str)
                        read_buffer = ''
                  if split_data[len_fend - 1]:
                      read_buffer = split_data[len_fend - 1]
        for frame in frames


def main():
    ki = KISS(port='COM7', speed='115200', pirate=True)
    ki.start()
    ki.read()
        #port.close()


if __name__ == '__main__':
    try:
        main()
    except FatalError as e:
        print ('\nA fatal error occurred: %s' % e)
        sys.exit(2)
