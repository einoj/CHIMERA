#!/usr/bin/python3
# -*- coding: utf-8 -*-

"""
Ground Software for CHIMERA

author: Eino Oltedal
"""

import sys
import threading
from time import sleep
from PyQt5.QtWidgets import (QMainWindow, QWidget, QPushButton, 
    QFrame, QApplication, QLabel, QTextEdit, QGridLayout, QHBoxLayout,
    QVBoxLayout, QCheckBox, QComboBox, QTableWidget, QTableWidgetItem, 
    QMenuBar, QAction, qApp, QFileDialog)
from PyQt5 import QtGui 
from PyQt5.QtCore import (QThread, pyqtSignal, QObject)
from PyQt5 import uic
from kiss import *
from kiss_uart import *
from kiss_constants import *
from kiss_tnc import *
LOG_FORMAT_FILE = logging.Formatter('%(message)s')

class Communicate(QObject):
    gui_signal = pyqtSignal(list)

def frame_thread(callbackFunc):
    mySrc = Communicate()
    mySrc.gui_signal.connect(callbackFunc)

    while True:
        if not ki.frame_queue.empty():
            frame = ki.get_frame()
            mySrc.gui_signal.emit(frame)
        sleep(0.05)

class GroundSoftware(QMainWindow):
    
    def __init__(self, kiss_serial=None):
        super().__init__()
        self.initUI()
        
    def clear_gui_data(self):
        self.lastFrame.clear()
        self.chi_status = CHI_STATUS()
        self.chi_sci_tm = CHI_SCI_TM()
        self.chi_events = CHI_EVENTS()

    def loadFile(self):
        self.clear_gui_data()
        openfile = QFileDialog.getOpenFileName(self)
        # We only need to load the SCI_TM frames
        print(openfile)
        frame = []
        frame_open = False
        with open(openfile[0], 'rb') as datafile:
            data = datafile.read()
            for byte in data:
                # the frame handling method expects bytes
                # therefore the int needs to be converted to byte
                byte = bytes([byte])
                if byte == FEND:
                    if frame_open:
                        retval = self.handle_file_frame(frame)
                        if retval != 0:
                            frame = [ ]
                        frame_open = False # close frame
                    else:
                        frame = [ ]
                        frame_open = True  # open frame
                else:
                    frame.append(byte)
        self.update_data_table()
        print(self.chi_events.seus)
        print(self.chi_events.rw_sefi)
        print("SEUs extracted from EVENT data: " + str(self.chi_events.real_seus))
        print("MBUs extracted from EVENT data: " + str(self.chi_events.real_mbus))
        print("Zero to one transitions: " + str(self.chi_events.zero_to_one))
        print("One to zero transitions: " + str(self.chi_events.one_to_zero))

    def handle_file_frame(self, frame):
        if len(frame) > 2:
            self.lastFrame.insertPlainText(ki.frame_to_string(frame) + '\n\n')
            byte_frame = decode_kiss_frame(frame)
            checksum = ki.check_checksum(byte_frame)
            if checksum != 0:
                #bad checksum, request packet again
                ki._logger.debug("Bad packet, drop frame")
                return -1
            if byte_frame[0]&0x0F == CHI_COMM_ID_SCI_TM:
                self.handle_SCI_TM_frame(byte_frame)
            elif byte_frame[0] == CHI_COMM_ID_STATUS:
                self.handle_status_frame(byte_frame)
            elif byte_frame[0] == CHI_COMM_ID_EVENT:
                self.handle_EVENT_frame(byte_frame)

    def handle_EVENT_frame(self, byte_frame):
        pattern = [0x55, 0xAA]
        #strip frametype, timestamp and checksum
        byte_frame = byte_frame[5:-1] 
        num_events = len(byte_frame)//5
        if num_events == 15:
            # SEFI
            print(byte_frame)
            self.chi_events.rw_sefi+=1
            for i in range(num_events):
                event = byte_frame[0:5]
                if event[1]%2 == 0:
                    print("SEFI 0x55 : " + str(hex(event[-1])+" upsets: " +str(bin(0x55^event[-1]).count('1'))))
                else:
                    print("SEFI 0xAA : " + str(hex(event[-1])+" upsets: " +str(bin(0x55^event[-1]).count('1'))))
        else:
            for i in range(num_events):
                # event data is structured as 
                # memid, addr1, addr2, addr3, upset
                event = byte_frame[0:5]
                # if addr1 is even the pattern should be 0x55
                seus = 0
                xormask = 0
                memID = event[0]
                # the number of upsets is counted by xoring the original value with the value afte upset
                # and counting the 1s. 
                if event[1]%2 == 0:
                    xormask = 0x55^event[-1]
                    seus = bin(xormask).count('1')
                    print("SEU 0x55 : " + str(hex(event[-1]))+" uspets: " +str(seus))
                else:
                    xormask = 0xAA^event[-1]
                    seus = bin(xormask).count('1')
                    print("SEU 0xAA : " + str(hex(event[-1]))+" uspets: " +str(seus))
                byte_frame = byte_frame[5:]
                self.chi_events.seus[memID] += 1
                # The xor mask is also used to find the number of one to zero and zero to one transitions
                self.chi_events.one_to_zero[memID] += bin(xormask&event[-1]).count('1')
                self.chi_events.zero_to_one[memID] += bin(xormask&~event[-1]).count('1')
                if seus > 1:
                    self.chi_events.real_mbus[memID] += 1
                else:
                    self.chi_events.real_seus[memID] += seus


    def initUI(self, kiss_serial=None):      
        self.setWindowTitle('CHIMERA GUI')
        widget = QWidget(self)
        self.setCentralWidget(widget)
        menubar = self.menuBar()

        # load data file function
        loadAction = QAction("&Open...", self)
        loadAction.setShortcut('Ctrl+O')
        loadAction.setStatusTip('Load previous frame file')
        loadAction.triggered.connect(self.loadFile)

        fileMenu = menubar.addMenu('&File')
        fileMenu.addAction(loadAction)

        self.labels = [ ]
        self.leds = [ ]
        self.checkboxes = [ ]

        #CHIMERA INFO
        self.chi_status = CHI_STATUS()
        self.chi_sci_tm = CHI_SCI_TM()
        self.chi_events = CHI_EVENTS()
        self.SOFTWARE_VERSION = QLabel("CHIMERA Software: V."+str(self.chi_status.SOFTWARE_VERSION))
        self.reset_type =QLabel("Reset type: "+str(self.chi_status.reset_type))
        self.device_mode = QLabel("Device Mode: " + str(self.chi_status.device_mode)) 
        self.no_cycles =  QLabel("Number of Cycles: " + str(self.chi_status.no_cycles))
        self.local_time = QLabel("CHIMERA time: " + str(self.chi_sci_tm.local_time))

        self.ki = kiss_serial
        self.red = QtGui.QColor(255, 0, 0)       
        self.green = QtGui.QColor(0, 255, 0)

        # Buttons for controlling CHIMERA
        button_box = QVBoxLayout()
        status_b = QPushButton('Get Status', self)
        status_b.clicked[bool].connect(self.getStatus)

        prevpacket_b = QPushButton('Get Previous', self)
        prevpacket_b.clicked[bool].connect(self.get_prev)
        sci_tm_b = QPushButton('Get Telemetry', self)
        sci_tm_b.clicked[bool].connect(self.get_sci_tcm)

        mode_box = QHBoxLayout()
        mode_label = QLabel("Select Mode")
        mode_button = QPushButton("Set Mode")
        mode_button.clicked[bool].connect(self.set_mode)
        self.mode_selector = QComboBox()
        self.mode_selector.addItem("Read Mode")
        self.mode_selector.addItem("R/W/E Mode")
        mode_box.addWidget(mode_label)
        mode_box.addWidget(self.mode_selector)

        button_box.addWidget(mode_button)
        button_box.addWidget(status_b)
        button_box.addWidget(prevpacket_b)
        button_box.addWidget(sci_tm_b)

        self.lastFrame = QTextEdit(self)
        self.lastFrame.setReadOnly(True)
        frame_box = QHBoxLayout()
        frame_box.addWidget(self.lastFrame)
        
        led_grid = QGridLayout()
        led_grid.columnStretch(1)
        self.set_led_layout(led_grid)

        middle_box = QVBoxLayout()
        sub_middle_box = QVBoxLayout()
        sub_middle_box.addWidget(self.SOFTWARE_VERSION)
        sub_middle_box.addWidget(self.reset_type)
        sub_middle_box.addWidget(self.device_mode)
        sub_middle_box.addWidget(self.no_cycles)
        sub_middle_box.addWidget(self.local_time)
        middle_box.addLayout(mode_box)
        middle_box.addLayout(sub_middle_box)
        middle_box.addLayout(led_grid)

        self.dataTable = QTableWidget()
        self.dataTable.setRowCount(12)
        self.dataTable.setColumnCount(7)
        table_labels = ["Cycles", "SEUs", "MBUs", "SELs", "R/W SEFIs", "Timeout SEFIs", "Current"]
        for i, label in enumerate(table_labels):
            self.dataTable.setHorizontalHeaderItem(i,QTableWidgetItem(label))
        for i in range(0,12):
            self.dataTable.setVerticalHeaderItem(i,QTableWidgetItem("mem"+str(i+1)))

        self.update_data_table()

        self.infoBoxes = QHBoxLayout()
        self.infoBoxes.addLayout(button_box)
        self.infoBoxes.addLayout(middle_box)
        self.infoBoxes.addWidget(self.lastFrame)

        self.mainLayout = QVBoxLayout()
        self.mainLayout.addLayout(self.infoBoxes)
        self.mainLayout.addWidget(self.dataTable)

        self.setGeometry(100, 100, 800, 850)
        widget.setLayout(self.mainLayout)
        self.show()
        self.start_frame_thread()

    def handle_frame(self, frame):
        if len(frame) > 2:
            #log frame
            with open("Binary_data.bin", "ab") as bin_file:
                # Add kiss framing that was removed
                bin_file.write(FEND)
                # write all bin values in list to file
                for byte in frame:
                    bin_file.write(byte)
                # Add kiss framing that was removed
                bin_file.write(FEND)
            self.lastFrame.insertPlainText(ki.frame_to_string(frame) + '\n\n')
            ki._logger.critical("c0 " + ki.frame_to_string(frame) + " c0")
            byte_frame = decode_kiss_frame(frame)
            checksum = ki.check_checksum(byte_frame)
            if checksum != 0:
                #bad checksum, request packet again
                ki._logger.debug("Bad packet, Request previous")
                self.get_prev()
                return -1
            if byte_frame[0]&0x0F == CHI_COMM_ID_SCI_TM:
                self.handle_SCI_TM_frame(byte_frame)
            elif byte_frame[0] == CHI_COMM_ID_STATUS:
                self.handle_status_frame(byte_frame)

    def start_frame_thread(self):
        ## Oops! should use signals and slots, not interact directly
        t = threading.Thread(name = 'frame_thread', target=frame_thread,daemon=True, args = (self.handle_frame,))
        ##t.daemon = True # stop when main thread stops
        t.start()

    def handle_status_frame(self,frame):
        self.chi_status.SOFTWARE_VERSION = frame[1]
        self.chi_status.reset_type = frame[2]
        self.chi_status.device_mode = frame[3]
        self.chi_status.no_cycles = int.from_bytes(frame[4:6],byteorder='big')    

        self.SOFTWARE_VERSION.setText("CHIMERA Software: V."+str(self.chi_status.SOFTWARE_VERSION))
        self.reset_type.setText("Reset type: "+str(self.chi_status.reset_type))
        self.device_mode.setText("Device Mode: " + str(self.chi_status.device_mode)) 
        self.no_cycles.setText("Number of Cycles: " + str(self.chi_status.no_cycles))

    def handle_SCI_TM_frame(self, frame):
        self.chi_sci_tm.local_time = int.from_bytes(frame[1:5],byteorder='big')    
        self.chi_sci_tm.mem_to_test = int.from_bytes(frame[5:7],byteorder='big')    
        #for i in range(1,13):
        #    self.chi_sci_tm.no_SEU[i-1] = frame[5*i]
        self.local_time.setText("CHIMERA time: " + str(self.chi_sci_tm.local_time) + " ms") 
        memories_upper = frame[5]
        memories_lower = frame[6]
        for i in range(0,8):
            if (memories_lower>>i & 1):
                self.leds[i].setStyleSheet("QWidget { background-color: %s }" %  
                    self.green.name())
            else:
                self.leds[i].setStyleSheet("QWidget { background-color: %s }" %  
                    self.red.name())
        for i in range(8,12):
            if (memories_upper>>(i-8) & 1):
                self.leds[i].setStyleSheet("QWidget { background-color: %s }" %  
                    self.green.name())
            else:
                self.leds[i].setStyleSheet("QWidget { background-color: %s }" %  
                    self.red.name())
        memory_data = frame[7:-1]
         
        #iterates over 12 memories
        #old versions of kiss_tnc.c sends 6 bytes for every memory while from 
        #commit 00984806d 7 bytes are sent because the SEU counter is extended
        #from 8-bit to 16-bit, and later turned into an MBU counter, 80 bytes vs 92 bytes.
        #Therefore the length of the sci_tm_frame is measured before it is read.
        #If there are only 6 bytes available the 7th byte will be set to 0
        #and the data will be formated like this:
        #cycles|SEU|LU|SEFI time| SEFI WR| current
        if len(frame) == 92:
            sci_bytes = 7
        elif len(frame) == 80:
            sci_bytes = 6
        else:
            print("Unsupported SCI_TM_frame format!")
            return -1
        for i in range(0,sci_bytes*12,sci_bytes):
            new_data = memory_data[i:i+sci_bytes]
            idx = i//sci_bytes
            for j in range(sci_bytes-1):
                delta = 0
                if new_data[j] < self.chi_sci_tm.prev_data[idx][j]:
                    delta = 255-self.chi_sci_tm.prev_data[idx][j]+new_data[j]+1
                else:
                    delta = new_data[j] - self.chi_sci_tm.prev_data[idx][j]
                self.chi_sci_tm.curr_data[idx][j] += delta
            self.chi_sci_tm.curr_data[idx][6] = new_data[sci_bytes-1]
            self.chi_sci_tm.prev_data[idx] = new_data

        self.update_data_table()


    def update_data_table(self):
        for i, row in enumerate(self.chi_sci_tm.curr_data):
            for j, col in enumerate(row):
                self.dataTable.setItem(i,j,QTableWidgetItem(str(col)))

    def set_mode(self):
        frame = b'\x07'
        if (self.mode_selector.currentIndex() == 0):
            # read mode
            frame += b'\x01'
        elif (self.mode_selector.currentIndex() == 1):
            # erase program read mode
            frame += b'\x02'
        else:
            frame = b'\x00'
        memories_lower = 0
        memories_upper = 0
        for i in range(0,8):
            if (self.checkboxes[i].isChecked()):
                memories_lower |= (1<<i)
        for i in range(8,12):
            if (self.checkboxes[i].isChecked()):
                memories_upper |= (1<<(i-8))
        frame += bytes([memories_upper, memories_lower])
        ki._logger.debug(frame)
        ki.write(frame)

    def get_sci_tcm(self, pressed):
        ki.request_sci_tm()

    def getStatus(self, pressed):
        ki.request_status()

    def get_prev(self):
        ki.send_nack()
       
    def set_led_layout(self, gridlayout):
        for i in range(12):
            if i < 9:
                label = QLabel("0"+str(i+1),self)
            else:
                label = QLabel(str(i+1),self)
            self.labels.append(label)
            gridlayout.addWidget(self.labels[i],0,i,1,1)

            led = QFrame(self)
            led.setStyleSheet("QWidget { background-color: %s }" %  
                self.red.name())
            self.leds.append (led)
            gridlayout.addWidget(self.leds[i],1,i,1,1)

            checkbox = QCheckBox(self)
            self.checkboxes.append(checkbox)
            gridlayout.addWidget(self.checkboxes[i],2,i,1,1)

    def exit_application(self):
        sys.exit(app.exec_())

if __name__ == '__main__':
    #exit_event = threading.Event() Consider using events instead of deamon=true
    # Remember to also check for event in GroundSoft
    ki = KISS(port='com8', speed='38400', pirate=False)
    ki.start()

    filename="Frames.txt"
    _file_handler = logging.FileHandler(filename)
    _file_handler.setLevel(LOG_LEVEL_CRI)
    _file_handler.setFormatter(LOG_FORMAT_FILE)
    ki._logger.addHandler(_file_handler)
    ki._logger.propagate = False

    sr_read_thread = threading.Thread(target=ki.simpleread,daemon=True)
    sr_read_thread.start()

    app = QApplication(sys.argv)
    ex = GroundSoftware()

    sys.exit(app.exec_())
