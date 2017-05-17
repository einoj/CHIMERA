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
import threading
from time import sleep
from queue import Queue
from time import sleep
from crc8 import calculateCRC8
from kiss import encode_kiss_frame, decode_kiss_frame
from kiss_constants import *
from random import randint
from math import ceil

# logger constants
LOG_LEVEL = logging.DEBUG
LOG_LEVEL_CRI = logging.CRITICAL
#LOG_FORMAT = logging.Formatter('%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
LOG_FORMAT = logging.Formatter('%(asctime)s %(message)s')

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
class CHI_SCI_DATA():
    def __init__(self):
        self.localtime = 0
        self.working_memories = 0
        self.no_LU_detected = 0
        self.no_SEFI_detected = 0
        self.no_SEFI_detected = 0
        self.mem1_status = 0
        self.mem1_no_SEU = 0 
        self.mem1_no_SEFI_timeout = 0
        self.mem1_no_SEFI_wr_error = 1
        self.mem1_curren1 = 0

def arg_auto_int(x):
    return int(x, 0)

class FatalError(RuntimeError):
    def __init__(self, message):
        RuntimeError.__init__(self, message)

class KISS(object):
    """
    KISS class based on https://github.com/ampledata/kiss/,
    Changed to python3 version using bytes instead of str
    """

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
        self.frame_queue = Queue() #messages to be sent are put here, messages are read by sending thread
        self.decoded_frames = Queue()

        if pirate == True and self.port is not None and self.speed is not None:
            self.speed = 115200
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
        elif 'serial' in self.interface_mode:
            self._logger.debug('Connection OPEN! Speed='+str(self.speed)+' Port='+str(self.port)+' \n')

            
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
            
    #reads bytes 
    def simpleread(self):
        while True:
            read_data = self.interface.read(1)
            if read_data == FEND:
                r_buffer = []
                read_data = self.interface.read(1)
                while (read_data != FEND):
                    if len(read_data)>0:
                        r_buffer.append(read_data)
                    read_data = self.interface.read(1)
                if len(r_buffer) > 0:
                    self.frame_queue.put(r_buffer)
    
    def check_checksum(self,frame):
        checksum = 0
        for i in frame:
            checksum = calculateCRC8(checksum,i)
        return checksum

    def handle_frames(self):
        if not self.frame_queue.empty():
            frame = self.frame_queue.get()
            checksum = 0
            for i in frame:
                checksum = calculateCRC8(checksum,int.from_bytes(i, byteorder='little'))
            self._logger.debug(frame)
        
    def get_frame(self):
        if not self.frame_queue.empty():
        #    frame = self.frame_queue.get()
        #    frame = [i.hex() for i in frame]
            return self.frame_queue.get()

    # Takes a data frame as input adds kiss framing and checksum
    def write(self, frame, verbose=False):
        interface_handler = self.interface.write

        if interface_handler is not None:
            frame = b''.join([ FEND, encode_kiss_frame(frame), FEND ])
            if verbose:
                self._logger.info("SENDING: " + frame.hex())
            return interface_handler(frame)

    def request_sci_tm(self, verbose=False):
        frame = b'\x01\x07'
        self.write(frame)

    def request_status(self, verbose=False):
        frame = b'\x02\x0E'
        self.write(frame)

    def send_time(self,new_time, verbose=False):
        frame = b'\x08'+(new_time).to_bytes(4,byteorder='big')
        self.write(frame)

    def send_nack(self, verbose=False):
        frame = b'\x15\x6B'
        self.write(frame)
    
    def wait_for_frame(self):
        while self.frame_queue.empty():
            sleep(0.05)

    def frame_to_string(self, frame):
        string = ""
        linewidth = 0
        for byte in frame:
            string += byte.hex()+" "
            linewidth += 2
            if linewidth == 80:
                string+='\n'
                linewidth = 0
        string = string[:-1]
        return string

def main():
    ki = KISS(port='com8', speed='38400', pirate=False)
    ki.start()

    sr_read_thread = threading.Thread(target=ki.simpleread)
    sr_read_thread.daemon = True # stop when main thread stops
    sr_read_thread.start()

    if '-ft' in sys.argv:
        ki.full_functional_test()
        #wait
    
    while True:
        ki.handle_frames()
        sleep(0.05)
        #port.close()
    #sr_read_thread._stop().set()
    #ki.interface.close()    


if __name__ == '__main__':
    try:
        main()
    except FatalError as e:
        print ('\nA fatal error occurred: %s' % e)
        sys.exit(2)
