from kiss_uart import *

def main():
    ki = KISS(port='com8', speed='38400', pirate=False)
    ki.start()

    sr_read_thread = threading.Thread(target=ki.simpleread)
    sr_read_thread.daemon = True # stop when main thread stops
    sr_read_thread.start()

    filename="Functional_test.txt"
    _file_handler = logging.FileHandler(filename)
    _file_handler.setLevel(LOG_LEVEL)
    _file_handler.setFormatter(LOG_FORMAT)
    ki._logger.addHandler(_file_handler)
    ki._logger.propagate = False
    # receive power-on status packet
    errors = 0
    tests = 0 

    ki._logger.info("TEST checklist #3 Receive power-on status packet after power-cycling the board\n________________________________________________________________________________")
    tests += 1
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED:  "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != STATUS:
        ki._logger.info("FAIL: First byte of status frame should be " + hex(STATUS) + ", is: " + hex(frame[0]))
        errors += 1
    else:
        ki._logger.info("OK: First byte of status frame is " + hex(STATUS))
    if ki.check_checksum(frame):
        ki._logger.info("FAIL: Checksum not correct")
        errors += 1
    else:
        ki._logger.info("OK: Checksum correct")
    if len(frame) != 7:
        ki._logger.info("FAIL: Length of frame should be 7\n")
        errors += 1
    else:
        ki._logger.info("OK: Length of frame is 7\n")
    
    # Send SCI_TM packet and receive Scientific telemetry
    ki._logger.info("TEST checklist #4 Request ‘SCI_TM’ packet\n________________________________________________________________________________")
    tests += 1
    ki.request_sci_tm(verbose=True)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED:\n"+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != SCI_TM ^ MODE1<<4:
        ki._logger.info("FAIL: First byte of SCI_TM frame should be " + hex(SCI_TM ^ MODE1<<4) + ", is: " + hex(frame[0]))
        errors += 1
    else:
        ki._logger.info("OK: First byte of SCI_TM frame is " + hex(SCI_TM ^ MODE1<<4))
    if ki.check_checksum(frame):
        ki._logger.info("FAIL: Checksum not correct")
        errors += 1
    else:
        ki._logger.info("OK: Checksum correct")
    if len(frame) != 92:
        ki._logger.info("FAIL: Length of frame should be 92, is: " + str(len(frame)) +"\n")
        errors += 1
    else:
        ki._logger.info("OK: Length of frame is 92\n")

    # Request ‘STATUS’ packet
    ki._logger.info("TEST checklist #5 Request ‘STATUS’ packet\n________________________________________________________________________________")
    tests += 1
    ki.request_status(verbose=True)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED:  "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != STATUS:
        ki._logger.info("FAIL: First byte of status frame should be " + hex(STATUS) + ", is: " + hex(frame[0]))
        errors += 1
    else:
        ki._logger.info("OK: First byte of status frame is " + hex(STATUS))
    if ki.check_checksum(frame):
        ki._logger.info("FAIL: Checksum not correct")
        errors += 1
    else:
        ki._logger.info("OK: Checksum correct")
    if len(frame) != 7:
        ki._logger.info("FAIL: Length of frame should be 7\n")
        errors += 1
    else:
        ki._logger.info("OK: Length of frame is 7\n")

    # Send NACK to receive ‘STATUS’ packet again
    ki._logger.info("TEST checklist #6 Send NACK to receive ‘STATUS’ packet again\n________________________________________________________________________________")
    tests += 1
    ki.send_nack(verbose=True)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != STATUS:
        ki._logger.info("FAIL: First byte of status frame should be " + hex(STATUS) + ", is: " + hex(frame[0]))
        errors += 1
    else:
        ki._logger.info("OK: First byte of status frame is " + hex(STATUS))
    if ki.check_checksum(frame):
        ki._logger.info("FAIL: Checksum not correct")
        errors += 1
    else:
        ki._logger.info("OK: Checksum correct")
    if len(frame) != 7:
        ki._logger.info("FAIL: Length of frame should be 7\n")
        errors += 1
    else:
        ki._logger.info("OK: Length of frame is 7\n")

    # Send random bytes and check if NACK was received.
    ki._logger.info("TEST #7 Send random bytes and check if NACK was received.\n________________________________________________________________________________")
    tests += 1
    frame = randint(99999,99999999999999)
    ki._logger.info("SENDING random bytes: "+ hex(frame))
    frame = frame.to_bytes(ceil(frame.bit_length()/8),byteorder='little')
    ki.interface.write(frame)
    ki.wait_for_frame()
    while not ki.frame_queue.empty():
        frame = ki.get_frame()
        ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != NACK:
            ki._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            ki._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

    #Check if commands are properly interpreted when KISS formatting has to be applied.
    #This should not be accepted:
    ki._logger.info("TEST checklist #8 Check if commands are properly interpreted when KISS formatting has to be applied\n________________________________________________________________________________")
    tests += 1
    frame = [0xC0, 0x07, 0x01, 0x00, 0xC0, 0x47, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != NACK:
        ki._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
        errors += 1
    else:
        ki._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

    #This should not be accepted:
    tests += 1
    frame = [0xC0, 0x07, 0x01, 0x00, 0xDB, 0x06, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != NACK:
        ki._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
        errors += 1
    else:
        ki._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

    #This should be accepted:
    tests += 1
    frame = [0xC0, 0x07, 0x01, 0x00, 0xDB, 0xDC, 0x47, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != ACK:
        ki._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
        errors += 1
    else:
        ki._logger.info("OK: received ACK: " + hex(frame[0])+'\n')

    #This should be accepted:
    tests += 1
    frame  = [0xC0, 0x07, 0x01, 0x00, 0xDC, 0x13, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != ACK:
        ki._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
        errors += 1
    else:
        ki._logger.info("OK: received ACK: " + hex(frame[0])+'\n')

    #This should be accepted:
    tests += 1
    frame = [0xC0, 0x07, 0x01, 0x00, 0xDD, 0x14, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != ACK:
        ki._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
        errors += 1
    else:
        ki._logger.info("OK: received ACK: " + hex(frame[0])+'\n')

    #This should be accepted:
    tests += 1
    frame = [0xC0, 0x07, 0x01, 0x00, 0xDB, 0xDD, 0x06, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != ACK:
        ki._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
        errors += 1
    else:
        ki._logger.info("OK: received ACK: " + hex(frame[0])+'\n')
    
    ki._logger.info("TEST checklist #9 Send empty KISS frame\n________________________________________________________________________________")
    tests += 1
    frame = [0xc0, 0xc0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != NACK:
        ki._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
        errors += 1
    else:
        ki._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

    ki._logger.info("TEST checklist #10 Send very long random frame\n________________________________________________________________________________")
    tests += 1
    frame = b''
    for i in range(10):
        number = randint(0,9999999999999999999999999999999999999999999) 
        frame += number.to_bytes(ceil(number.bit_length()/8),byteorder='little') 
    ki._logger.info("SENDING random bytes: "+ frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    sleep(2)
    while not ki.frame_queue.empty():
        frame = ki.get_frame()
        ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != NACK:
            ki._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            ki._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

    ki._logger.info("TEST checklist #11 Send two frames merged into one KISS formatting\n________________________________________________________________________________")
    tests += 1
    frame = [0xC0, 0x01, 0x01, 0x12, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED:\n"+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != SCI_TM ^ MODE1<<4:
        ki._logger.info("FAIL: First byte of SCI_TM frame should be " + hex(SCI_TM ^ MODE1<<4) + ", is: " + hex(frame[0]))
        errors += 1
    else:
        ki._logger.info("OK: First byte of SCI_TM frame is " + hex(SCI_TM ^ MODE1<<4))
    if ki.check_checksum(frame):
        ki._logger.info("FAIL: Checksum not correct")
        errors += 1
    else:
        ki._logger.info("OK: Checksum correct")
    if len(frame) != 92:
        ki._logger.info("FAIL: Length of frame should be 92, is: " + str(len(frame)) +"\n")
        errors += 1
    else:
        ki._logger.info("OK: Length of frame is 92\n")

    ki._logger.info("TEST checklist #12 Send babbling idiot frame\n________________________________________________________________________________")
    tests += 1
    frame = 10*[0xC0, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    while not ki.frame_queue.empty():
        frame = ki.get_frame()
        ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != NACK:
            ki._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            ki._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

    frame = 100*[0xC0, 0xC0]
    tests += 1
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    sleep(2)
    while not ki.frame_queue.empty():
        frame = ki.get_frame()
        ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
        frame = decode_kiss_frame(frame)
        if frame[0] != NACK:
            ki._logger.info("FAIL: expected " + hex(NACK) + ", received: " + hex(frame[0])+'\n')
            errors += 1
        else:
            ki._logger.info("OK: received NACK: " + hex(frame[0])+'\n')

    # Send ‘TIME’ command 
    # Check if ACK is received.
    # Send ‘STATUS’ to check if time was updated.
    # Note: time will be updated only after next SCI_TM packet was sent.
    ki._logger.info("TEST checklist #13 Send 'TIME' command and Send 'STATUS' to check if time was updated.\n________________________________________________________________________________")
    tests += 1
    ki.send_time(0x12345678, verbose=True)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != ACK:
        ki._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0]))
        errors += 1
    else:
        ki._logger.info("OK: received ACK: " + hex(ACK))
    ki._logger.info("Waiting for next SCI_TM frame..")
    ki.wait_for_frame()
    ki.get_frame()
    ki.request_sci_tm(verbose=True)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED:\n"+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    new_time = int.from_bytes(frame[1:5],  byteorder="big")
    if new_time < 0x12345678:
        ki._logger.info("FAIL: expected new time > " + hex(0x12345678) + ", is: " + frame[1:5].hex()+'\n')
        errors += 1
    else:
        ki._logger.info("OK: new time is " + frame[1:5].hex()+'\n')

    ki._logger.info("TEST checklist #14 Check that all memories have been tested and that none of them have errors.\n________________________________________________________________________________")
    tests += 1
    tmp_errors = errors
    for i in range(12):
        sub_frame = frame[(i+1)*7:(i+1)*7+7]
        if sub_frame[0] < 1:
          ki._logger.info("FAIL: "+str(sub_frame[0])+" cycles on memory u" + str(i+1))
          errors += 1
        if sub_frame[1] != 0:
          ki._logger.info("FAIL: "+str(sub_frame[1])+"SEUs in memory u" + str(i+1))
          errors += 1
        if sub_frame[2] != 0:
          ki._logger.info("FAIL: "+str(sub_frame[2])+" MBUs in memory u" + str(i+1))
          errors += 1
        if sub_frame[3] != 0: 
          ki._logger.info("FAIL: "+str(sub_frame[3])+" LUs in memory u" + str(i+1))
          errors += 1
        if sub_frame[4] != 0:
          ki._logger.info("FAIL: "+str(sub_frame[4])+" timeout SEFIs in memory u" + str(i+1))
          errors += 1
        if sub_frame[5] != 0:
          ki._logger.info("FAIL: "+str(sub_frame[5])+" RW SEFIs in memory u" + str(i+1))
          errors += 1
    if tmp_errors == errors:
        ki._logger.info("OK: All memories functioning as expected\n")
    ki._logger.info("TEST checklist #16 Set MODE 2 to test only 6 SRAMs\n________________________________________________________________________________")
    tests += 1
    frame = [0xC0, 0x07, 0x02, 0x0f, 0xDB, 0xDC, 0x39, 0xC0]
    frame = bytes(frame)
    ki._logger.info("SENDING: " + frame.hex())
    ki.interface.write(frame)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED: "+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != ACK:
        ki._logger.info("FAIL: expected " + hex(ACK) + ", received: " + hex(frame[0])+'\n')
        errors += 1
    else:
        ki._logger.info("OK: received ACK: " + hex(frame[0])+'\n')
    ki._logger.info("Waiting for next SCI_TM frame to check if mode changed correctly...")
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki.request_sci_tm(verbose=True)
    ki.wait_for_frame()
    frame = ki.get_frame()
    ki._logger.info("RECEIVED:\n"+ ki.frame_to_string(frame))
    frame = decode_kiss_frame(frame)
    if frame[0] != SCI_TM ^ MODE2<<4:
        ki._logger.info("FAIL: First byte of SCI_TM frame should be " + hex(SCI_TM ^ MODE1<<4) + ", is: " + hex(frame[0]))
        errors += 1
    else:
        ki._logger.info("OK: First byte of SCI_TM frame is " + hex(SCI_TM ^ MODE1<<4))
    if ki.check_checksum(frame):
        ki._logger.info("FAIL: Checksum not correct")
        errors += 1
    else:
        ki._logger.info("OK: Checksum correct")
    if frame[5] != 0x0f and frame[6] != 0xc0:
        ki._logger.info("FAIL: Wrong number of memories activated")
        errors += 1
    else:
        ki._logger.info("OK: Only SRAMS activated")
    if frame[0]>>4 != MODE2:
        ki._logger.info("FAIL: MODE did not change to 0x02"+"\n")
        errors += 1
    else:
        ki._logger.info("OK: MODE Changed to " +hex(frame[0]>>4) +"\n")


    ki._logger.info("TEST Summary:\n________________________________________________________________________________")
    ki._logger.info("Performed " + str(tests) + " tests")
    ki._logger.info("Found " + str(errors) + " errors")


if __name__ == '__main__':
    try:
        main()
    except FatalError as e:
        print ('\nA fatal error occurred: %s' % e)
        sys.exit(2)
