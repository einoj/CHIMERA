from pirate import KISS, NACK, STATUS, SCI_TM
from time import sleep
from crc8 import calculateCRC8
import threading

class FatalError(RuntimeError):
    def __init__(self, message):
        RuntimeError.__init__(self, message)

def main():
    ki = KISS(port='com7', speed='115200', pirate=True)
    ki.start()
    sr_read_thread = threading.Thread(target=ki.simpleread)
    sr_read_thread.daemon = True # stop when main thread stops
    sr_read_thread.start()

    #Send MODE receive ACK
    ki._logger.debug("Entering Test")
    
    ki._logger.debug("==================")
    ki._logger.debug("= Setting mode")
    ki.write(b'\x07\x01\x00\x07')
    while (ki.frame_queue.empty()):
        sleep (0.01)
    frame = ki.get_frame()
    if frame != b'\x1a\x46':
        ki._logger.debug("= ERROR Expected ACK, received: %s", frame)
    else:
        ki._logger.debug("= OK: Changing MODE sucessful")

     
    #Send bad packet receive NACK
    ki._logger.debug("==================")
    ki._logger.debug("= Sending bad packet")
    ki.interface.write(b'\xAC\xDC')
    while (ki.frame_queue.empty()):
        sleep (0.01)
    frame = ki.get_frame()
    if frame != b'\x15\x6B':
        ki._logger.debug("= ERROR: Expected NACK, received: %s", frame)
    else:
        ki._logger.debug("= OK: Sending bad packet received NACK")

    # Request STATUS packet    
    ki._logger.debug("=========================")
    ki._logger.debug("= Requesting Status packet")
    ki.write(STATUS)
    while (ki.frame_queue.empty()):
        sleep (0.01)
    frame = ki.get_frame()
    if len(frame) != 7:
        ki._logger.debug("= ERROR: Expected Status frame length to be 7 got: %d", len(frame))
    else:
        ki._logger.debug("= OK: Length Status of packet 7")
    checksum = 0
    for byte in frame:
        checksum = calculateCRC8(checksum,byte)
    if checksum != 0:
        ki._logger.debug("= ERROR: Error in checksum")
    else:
        ki._logger.debug("= OK: Status packet has correct checksum")


    #Send NACK receive previous packet
    ki._logger.debug("==================")
    ki._logger.debug("= Sending NACK ")
    ki.write(NACK)
    while (ki.frame_queue.empty()):
        sleep (0.01)
    new_frame = ki.get_frame()
    if new_frame != frame:
        ki._logger.debug("= ERROR: Expected to receive previous frame %s, received %s ", frame, new_frame)
    else:
        ki._logger.debug("= OK: Sent NACK received previous frame")

    #Send SCI_TM receive data packet.
    ki._logger.debug("=========================")
    ki._logger.debug("= Requesting Status packet")
    ki.write(SCI_TM)
    while (ki.frame_queue.empty()):
        sleep (0.01)
    frame = ki.get_frame()
    if len(frame) != 98:
        ki._logger.debug("= ERROR: Expected Status frame length to be 98 got: %d", len(frame))
    else:
        ki._logger.debug("= OK: Length Status of packet 98")
    checksum = 0
    for byte in frame:
        checksum = calculateCRC8(checksum,byte)
    if checksum != 0:
        ki._logger.debug("= ERROR: Error in checksum")
    else:
        ki._logger.debug("= OK: Status packet has correct checksum")
    
    #Send TIME command and request SCI_TM packet to check that local time has changed
    ki._logger.debug("=========================")
    ki._logger.debug("= Change local Time")
    #ki.write(SCI_TM)
    sleep (0.01)
    time_frame = b'\x08\x12\x34\x56\x78'
    ki.write(time_frame)
    #ki.write(SCI_TM)
    while (ki.frame_queue.empty()):
        sleep (0.01)
    old_frame = ki.get_frame()
    new_frame = ki.get_frame()
    ki._logger.debug("Frame %s", old_frame)
    ki._logger.debug("Frame %s", new_frame)
    checksum = 0
    for byte in frame:
        checksum = calculateCRC8(checksum,byte)
    if checksum != 0:
        ki._logger.debug("= ERROR: Error in checksum")
    else:
        ki._logger.debug("= OK: Status packet has correct checksum")
    

    
        #wait
    #ki.simpleread()
        #port.close()


if __name__ == '__main__':
    try:
        main()
    except FatalError as e:
        print ('\nA fatal error occurred: %s' % e)
        sys.exit(2)
