
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

        self.no_SEU = [ ]
        self.no_LU = [ ]
        self.no_SEFI_timeout  = [ ]
        self.no_SEFI_rw_error = [ ]
        self.no_SEFI_current1 = [ ]
        
        for i in range(12):
            self.no_SEU.append(0)
            self.no_LU.append(0)
            self.no_SEFI_timeout.append(0)
            self.no_SEFI_rw_error.append(0)
            self.no_SEFI_current1.append(0)
	
