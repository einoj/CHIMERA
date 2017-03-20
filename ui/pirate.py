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
            self._logger.debug(decode_kiss_frame(frame))
        
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

    def full_functional_test(self, filename="Functional_test.txt"):
        _file_handler = logging.FileHandler(filename)
        _file_handler.setLevel(LOG_LEVEL)
        _file_handler.setFormatter(LOG_FORMAT)
        self._logger.addHandler(_file_handler)
        self._logger.propagate = False
        # receive power-on status packet
        errors = 0
        tests = 0 

        self._logger.info("TEST checklist #3 Receive power-on status packet after power-cycling the board\n________________________________________________________________________________")
        tests += 1
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED:  "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != STATUS:
            self._logger.info("FAIL: First byte of status frame should be " + hex(STATUS) + ", is: " + hex(frame[0]))
            errors += 1
        else:
            self._logger.info("OK: First byte of status frame is " + hex(STATUS))
        if self.check_checksum(frame):
            self._logger.info("FAIL: Checksum not correct")
            errors += 1
        else:
            self._logger.info("OK: Checksum correct")
        if len(frame) != 7:
            self._logger.info("FAIL: Length of frame should be 7\n")
            errors += 1
        else:
            self._logger.info("OK: Length of frame is 7\n")
        
        # Send SCI_TM packet and receive Scientific telemetry
        self._logger.info("TEST checklist #4 Request ‘SCI_TM’ packet\n________________________________________________________________________________")
        tests += 1
        self.request_sci_tm(verbose=True)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED:\n"+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != SCI_TM ^ MODE1<<4:
            self._logger.info("FAIL: First byte of SCI_TM frame should be " + hex(SCI_TM ^ MODE1<<4) + ", is: " + hex(frame[0]))
            errors += 1
        else:
            self._logger.info("OK: First byte of SCI_TM frame is " + hex(SCI_TM ^ MODE1<<4))
        if self.check_checksum(frame):
            self._logger.info("FAIL: Checksum not correct")
            errors += 1
        else:
            self._logger.info("OK: Checksum correct")
        if len(frame) != 92:
            self._logger.info("FAIL: Length of frame should be 92, is: " + str(len(frame)) +"\n")
            errors += 1
        else:
            self._logger.info("OK: Length of frame is 92\n")

        # Request ‘STATUS’ packet
        self._logger.info("TEST checklist #5 Request ‘STATUS’ packet\n________________________________________________________________________________")
        tests += 1
        self.request_status(verbose=True)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED:  "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != STATUS:
            self._logger.info("FAIL: First byte of status frame should be " + hex(STATUS) + ", is: " + hex(frame[0]))
            errors += 1
        else:
            self._logger.info("OK: First byte of status frame is " + hex(STATUS))
        if self.check_checksum(frame):
            self._logger.info("FAIL: Checksum not correct")
            errors += 1
        else:
            self._logger.info("OK: Checksum correct")
        if len(frame) != 7:
            self._logger.info("FAIL: Length of frame should be 7\n")
            errors += 1
        else:
            self._logger.info("OK: Length of frame is 7\n")

        # Send NACK to receive ‘STATUS’ packet again
        self._logger.info("TEST checklist #6 Send NACK to receive ‘STATUS’ packet again\n________________________________________________________________________________")
        tests += 1
        self.send_nack(verbose=True)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != STATUS:
            self._logger.info("FAIL: First byte of status frame should be " + hex(STATUS) + ", is: " + hex(frame[0]))
            errors += 1
        else:
            self._logger.info("OK: First byte of status frame is " + hex(STATUS))
        if self.check_checksum(frame):
            self._logger.info("FAIL: Checksum not correct")
            errors += 1
        else:
            self._logger.info("OK: Checksum correct")
        if len(frame) != 7:
            self._logger.info("FAIL: Length of frame should be 7\n")
            errors += 1
        else:
            self._logger.info("OK: Length of frame is 7\n")

        # Send random bytes and check if NACK was received.
        self._logger.info("TEST checklist #9\n________________________________________________________________________________")
        tests += 1
        frame = randint(99999,99999999999999)
        self._logger.info("SENDING random bytes: "+ hex(frame))
        frame = frame.to_bytes(ceil(frame.bit_length()/8),byteorder='little')
        self.interface.write(frame)
        self.wait_for_frame()
        while not self.frame_queue.empty():
            frame = self.get_frame()
            self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
            frame = decode_kiss_frame(frame)
            if frame[0] != NACK:
                self._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
                errors += 1
            else:
                self._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

        #Check if commands are properly interpreted when KISS formatting has to be applied.
        #This should not be accepted:
        self._logger.info("TEST checklist #12 Check if commands are properly interpreted when KISS formatting has to be applied\n________________________________________________________________________________")
        tests += 1
        frame = [0xC0, 0x07, 0x01, 0x00, 0xC0, 0x47, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != NACK:
            self._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            self._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

        #This should not be accepted:
        tests += 1
        frame = [0xC0, 0x07, 0x01, 0x00, 0xDB, 0x06, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != NACK:
            self._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            self._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

        #This should be accepted:
        tests += 1
        frame = [0xC0, 0x07, 0x01, 0x00, 0xDB, 0xDC, 0x47, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != ACK:
            self._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            self._logger.info("OK: received ACK: " + hex(frame[0])+'\n')

        #This should be accepted:
        tests += 1
        frame  = [0xC0, 0x07, 0x01, 0x00, 0xDC, 0x13, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != ACK:
            self._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            self._logger.info("OK: received ACK: " + hex(frame[0])+'\n')

        #This should be accepted:
        tests += 1
        frame = [0xC0, 0x07, 0x01, 0x00, 0xDD, 0x14, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != ACK:
            self._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            self._logger.info("OK: received ACK: " + hex(frame[0])+'\n')

        #This should be accepted:
        tests += 1
        frame = [0xC0, 0x07, 0x01, 0x00, 0xDB, 0xDD, 0x06, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != ACK:
            self._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            self._logger.info("OK: received ACK: " + hex(frame[0])+'\n')
        
        self._logger.info("TEST checklist #13 Send empty KISS frame\n________________________________________________________________________________")
        tests += 1
        frame = [0xc0, 0xc0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != NACK:
            self._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            self._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

        self._logger.info("TEST checklist #14 Send very long random frame\n________________________________________________________________________________")
        tests += 1
        frame = b''
        for i in range(10):
            number = randint(0,9999999999999999999999999999999999999999999) 
            frame += number.to_bytes(ceil(number.bit_length()/8),byteorder='little') 
        self._logger.info("SENDING random bytes: "+ frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        sleep(2)
        while not self.frame_queue.empty():
            frame = self.get_frame()
            self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
            frame = decode_kiss_frame(frame)
            if frame[0] != NACK:
                self._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
                errors += 1
            else:
                self._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

        self._logger.info("TEST checklist #15 Send two frames merged into one KISS formatting\n________________________________________________________________________________")
        tests += 1
        frame = [0xC0, 0x01, 0x01, 0x12, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED:\n"+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != SCI_TM ^ MODE1<<4:
            self._logger.info("FAIL: First byte of SCI_TM frame should be " + hex(SCI_TM ^ MODE1<<4) + ", is: " + hex(frame[0]))
            errors += 1
        else:
            self._logger.info("OK: First byte of SCI_TM frame is " + hex(SCI_TM ^ MODE1<<4))
        if self.check_checksum(frame):
            self._logger.info("FAIL: Checksum not correct")
            errors += 1
        else:
            self._logger.info("OK: Checksum correct")
        if len(frame) != 92:
            self._logger.info("FAIL: Length of frame should be 92, is: " + str(len(frame)) +"\n")
            errors += 1
        else:
            self._logger.info("OK: Length of frame is 92\n")

        self._logger.info("TEST checklist #16 Send babbling idiot frame\n________________________________________________________________________________")
        tests += 1
        frame = 10*[0xC0, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        while not self.frame_queue.empty():
            frame = self.get_frame()
            self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
            frame = decode_kiss_frame(frame)
            if frame[0] != NACK:
                self._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
                errors += 1
            else:
                self._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

        frame = 100*[0xC0, 0xC0]
        tests += 1
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        sleep(2)
        while not self.frame_queue.empty():
            frame = self.get_frame()
            self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
            frame = decode_kiss_frame(frame)
            if frame[0] != NACK:
                self._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
                errors += 1
            else:
                self._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

        # Send ‘TIME’ command 
        # Check if ACK is received.
        # Send ‘STATUS’ to check if time was updated.
        # Note: time will be updated only after next SCI_TM packet was sent.
        self._logger.info("TEST checklist #12 Send 'TIME' command and Send 'STATUS' to check if time was updated.\n________________________________________________________________________________")
        tests += 1
        self.send_time(0x12345678, verbose=True)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != ACK:
            self._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0]))
            errors += 1
        else:
            self._logger.info("OK: received ACK: " + hex(ACK))
        self._logger.info("Waiting for next SCI_TM frame..")
        self.wait_for_frame()
        self.get_frame()
        self.request_sci_tm(verbose=True)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED:\n"+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        new_time = int.from_bytes(frame[1:5],  byteorder="big")
        if new_time < 0x12345678:
            self._logger.info("FAIL: expected new time > " + hex(0x12345678) + ", is: " + frame[1:5].hex()+'\n')
            errors += 1
        else:
            self._logger.info("OK: new time is " + frame[1:5].hex()+'\n')

        self._logger.info("TEST checklist #X Check that all memories have been tested and that none of them has errors.\n________________________________________________________________________________")
        tests += 1
        tmp_errors = errors
        for i in range(12):
            sub_frame = frame[(i+1)*7:(i+1)*7+7]
            if sub_frame[0] < 1:
              self._logger.info("FAIL: "+str(sub_frame[0])+" cycles on memory u" + str(i+1))
              errors += 1
            if sub_frame[1] != 0:
              self._logger.info("FAIL: "+str(sub_frame[1])+"SEUs in memory u" + str(i+1))
              errors += 1
            if sub_frame[2] != 0:
              self._logger.info("FAIL: "+str(sub_frame[2])+" MBUs in memory u" + str(i+1))
              errors += 1
            if sub_frame[3] != 0: 
              self._logger.info("FAIL: "+str(sub_frame[3])+" LUs in memory u" + str(i+1))
              errors += 1
            if sub_frame[4] != 0:
              self._logger.info("FAIL: "+str(sub_frame[4])+" timeout SEFIs in memory u" + str(i+1))
              errors += 1
            if sub_frame[5] != 0:
              self._logger.info("FAIL: "+str(sub_frame[5])+" RW SEFIs in memory u" + str(i+1))
              errors += 1
        if tmp_errors == errors:
            self._logger.info("OK: All memories functioning as expected\n")
        self._logger.info("TEST checklist #18 Set MODE 2 to test only 6 SRAMs\n________________________________________________________________________________")
        tests += 1
        frame = [0xC0, 0x07, 0x02, 0x0f, 0xDB, 0xDC, 0x39, 0xC0]
        frame = bytes(frame)
        self._logger.info("SENDING: " + frame.hex())
        self.interface.write(frame)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED: "+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != ACK:
            self._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            self._logger.info("OK: received ACK: " + hex(frame[0])+'\n')
        self._logger.info("Waiting for next SCI_TM frame to check if mode changed correctly...")
        self.wait_for_frame()
        frame = self.get_frame()
        self.request_sci_tm(verbose=True)
        self.wait_for_frame()
        frame = self.get_frame()
        self._logger.info("RECEIVED:\n"+ self.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != SCI_TM ^ MODE2<<4:
            self._logger.info("FAIL: First byte of SCI_TM frame should be " + hex(SCI_TM ^ MODE1<<4) + ", is: " + hex(frame[0]))
            errors += 1
        else:
            self._logger.info("OK: First byte of SCI_TM frame is " + hex(SCI_TM ^ MODE1<<4))
        if self.check_checksum(frame):
            self._logger.info("FAIL: Checksum not correct")
            errors += 1
        else:
            self._logger.info("OK: Checksum correct")
        if frame[5] != 0x0f and frame[6] != 0xc0:
            self._logger.info("FAIL: Wrong number of memories activated")
            errors += 1
        else:
            self._logger.info("OK: Only SRAMS activated")
        if frame[0]>>4 != MODE2:
            self._logger.info("FAIL: MODE did not change to 0x02"+"\n")
            errors += 1
        else:
            self._logger.info("OK: MODE Changed to " +hex(frame[0]>>4) +"\n")


        self._logger.info("TEST Summary:\n________________________________________________________________________________")
        self._logger.info("Performed " + str(tests) + " tests")
        self._logger.info("Found " + str(errors) + " errors")

def main():
    ki = KISS(port='com9', speed='38400', pirate=False)
    ki.start()

    sr_read_thread = threading.Thread(target=ki.simpleread)
    sr_read_thread.daemon = True # stop when main thread stops
    sr_read_thread.start()

    if '-ft' in sys.argv:
        ki.full_functional_test()
        #wait
    #ki.simpleread()
        #port.close()
    #sr_read_thread._stop().set()
    #ki.interface.close()    


if __name__ == '__main__':
    try:
        main()
    except FatalError as e:
        print ('\nA fatal error occurred: %s' % e)
        sys.exit(2)
