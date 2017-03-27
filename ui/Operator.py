#!/usr/bin/python3
# -*- coding: utf-8 -*-

"""
Ground Software for CHIMERA

author: Eino Oltedal
"""

import sys
import threading
from time import sleep
from PyQt5.QtWidgets import (QWidget, QPushButton, 
    QFrame, QApplication, QLabel, QTextEdit, QGridLayout, QHBoxLayout, QVBoxLayout, QCheckBox, QComboBox, QTableWidget)
from PyQt5.QtGui import QColor 
from PyQt5.QtCore import (QThread, pyqtSignal, QObject)
from PyQt5 import uic
from kiss import *
from kiss_uart import KISS, NACK, STATUS, SCI_TM
from kiss_constants import *
from kiss_tnc import *

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

class GroundSoftware(QWidget):
    
    def __init__(self, kiss_serial=None):
        super().__init__()
        
        self.initUI()
        
    def initUI(self, kiss_serial=None):      
        self.labels = [ ]
        self.leds = [ ]
        self.checkboxes = [ ]

        #CHIMERA INFO
        self.chi_status = CHI_STATUS()
        self.chi_sci_tm = CHI_SCI_TM()
        self.SOFTWARE_VERSION = QLabel("CHIMERA Software: V."+str(self.chi_status.SOFTWARE_VERSION))
        self.reset_type =QLabel("Reset type: "+str(self.chi_status.reset_type))
        self.device_mode = QLabel("Device Mode: " + str(self.chi_status.device_mode)) 
        self.no_cycles =  QLabel("Number of Cycles: " + str(self.chi_status.no_cycles))
        self.local_time = QLabel("CHIMERA time: " + str(self.chi_sci_tm.local_time))

        self.ki = kiss_serial
        self.red = QColor(255, 0, 0)       
        self.green = QColor(0, 255, 0)

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

        dataTable = QTableWidget()
        dataTable.setRowCount(13)
        dataTable.setColumnCount(5)

        infoBoxes = QHBoxLayout()
        infoBoxes.addLayout(button_box)
        infoBoxes.addLayout(middle_box)
        infoBoxes.addWidget(self.lastFrame)

        mainLayout = QVBoxLayout(self)

        mainLayout.addLayout(infoBoxes)
        mainLayout.addWidget(dataTable)

        self.setGeometry(300, 300, 600, 200)
        self.setWindowTitle('Toggle button')
        self.setLayout(mainLayout)
        self.show()
        self.start_frame_thread()

    def handle_frame(self, frame):
        if len(frame) > 2:
            self.lastFrame.insertPlainText(ki.frame_to_string(frame) + '\n\n')
            ki._logger.debug(ki.frame_to_string(frame))
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
        t = threading.Thread(name = 'frame_thread', target=frame_thread, args = (self.handle_frame,))
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
        ki._logger.debug(len(frame))
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

if __name__ == '__main__':
    ki = KISS(port='com8', speed='38400', pirate=False)
    ki.start()

    sr_read_thread = threading.Thread(target=ki.simpleread)
    sr_read_thread.daemon = True # stop when main thread stops
    sr_read_thread.start()

    app = QApplication(sys.argv)
    ex = GroundSoftware(ki)

    sys.exit(app.exec_())
