
def encode_kiss_frame(frame):
    """
    Append CRC8 to frame, then preform KISS encoding

    """
    checksum = 0

    for byte in frame:
        checksum = calculateCRC8(checksum,byte)
    frame = b''.join([frame, bytes([checksum])])
    return frame.replace(FESC, FESC_TFESC).replace(FEND, FESC_TFEND)

def decode_kiss_frame(frame):
    # "FEND is sent as FESC, TFEND"
    FESC_TFEND = b''.join([FESC, TFEND])

    # "FESC is sent as FESC, TFESC"
    FESC_TFESC = b''.join([FESC, TFESC])
    """
    Recover special codes, per KISS spec.
    "If the FESC_TFESC or FESC_TFEND escaped codes appear in the data received, they
    need to be recovered to the original codes. The FESC_TFESC code is replaced by
    FESC code and FESC_TFEND is replaced by FEND code."
    - http://en.wikipedia.org/wiki/KISS_(TNC)#Description
    """ 
    frame = b''.join(frame)
    return frame.replace( FESC_TFESC, FESC).replace( FESC_TFEND, FEND)
