import numpy as np

class CHI_STATUS: 

    def __init__(self):
        self.SOFTWARE_VERSION = 0
        self.reset_type = 0
        self.device_mode = 0
        self.no_cycles = 0

class CHI_SCI_TM:
    def __init__(self):
        self.local_time = 0
        self.mem_to_test = 0
        
        self.prev_data = 12*[b'\x00\x00\x00\x00\x00\x00\x00']
        # current_data contains no_cycles, no_SEU, no_MBU, no_LU, no_SEFI_timeout, no_SEFI_rw_error, no_SEFI_current
        self.curr_data= np.zeros([12,7],dtype=int) #12*[7*[0]]

