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
from time import sleep

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

def main():
    parser = argparse.ArgumentParser(description = 'Bus Pirate binary interface demo', prog = 'binaryModeDemo')

    parser.add_argument(
            '--port', '-p',
            help = 'Serial port device',
            default = 'COM7')

    parser.add_argument(
            '--baud', '-b',
            help = 'Serial port baud rate',
            type = arg_auto_int,
            default = 115200)

    args = parser.parse_args()

    print('\nTrying port: ', args.port, ' at baudrate: ', args.baud)

    try:
        port = serial.Serial(args.port, args.baud, timeout=0.1)
    except Exception as e:
        print('I/O error({0}): {1}'.format(e.errno, e.strerror))
        print('Port cannot be opened')
    else:
        print('Ready!')
        print('Entering binary mode...\n')

        count = 0
        done = False
        while count < 20 and not done:
            count += 1
            port.write(commands.get('BBIO1'))
            got = port.read(5)  # Read up to 5 bytes
            if got == b'BBIO1':
                print('Enterd binary mode!')
                done = True
        if not done:
            port.close()
            print("got " + str(got))
            raise FatalError('Buspirate failed to enter binary mode')

        # Now that the Buspirate is in binary mode, choose a BP mode
        """
        port.write(commands.get('RESET'))
        while True:
            got = port.readline()
            if not got:
                break
            print(got),
        """

        """
        port.write(commands.get('SPI1'))
        got = port.read(4)
        if got == 'SPI1':
            print 'Entered binary SPI mode'
        else:
            raise FatalError('Buspirate failed to enter new mode')
        """

        port.write(commands.get('ART1'))
        got = port.read(4)
        if got == b'ART1':
            print('Entered UART mode')
        else:
            raise FatalError('Buspirate failed to enter UART mode')
        
        port.write(commands.get('SPEED'))
        got = port.read(4)
        if got == b'\x01':
            print('Changed speed to 115200')
        else:
            print (got)
            raise FatalError('Buspirate failed to change speed')

        port.write(commands.get('RX'))
        got = port.read(1)
        print (got)
        
        # Entering Bridge mode
        port.write(commands.get('BRIDGE'))
        got = port.read(1)
        print (got)

        while True:
            got = port.read(1)
            if not got:
                sleep(0.1)
            else:
                print(got)

        port.close()


if __name__ == '__main__':
    try:
        main()
    except FatalError as e:
        print ('\nA fatal error occurred: %s' % e)
        sys.exit(2)
