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
    QFrame, QApplication, QLabel, QTextEdit, QGridLayout, QHBoxLayout, QVBoxLayout, QCheckBox, QComboBox)
from PyQt5.QtGui import QColor
from PyQt5 import uic
from pirate import KISS, NACK, STATUS, SCI_TM
from kiss_constants import *
from kiss_tnc import *

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

        mainLayout = QHBoxLayout(self)

        mainLayout.addLayout(button_box)
        mainLayout.addLayout(middle_box)
        mainLayout.addWidget(self.lastFrame)

        self.setGeometry(300, 300, 600, 200)
        self.setWindowTitle('Toggle button')
        self.setLayout(mainLayout)
        self.show()

    def update_frame_view(self):
        while 1:
            if not ki.frame_queue.empty():
                frame = ki.get_frame()
                self.lastFrame.insertPlainText(''.join(["0x%02x "% byte for byte in frame])+'\n\n')
                if frame[0] == CHI_COMM_ID_SCI_TM:
                    self.handle_SCI_TM_frame(frame[1:])
                elif frame[0] == CHI_COMM_ID_STATUS:
                    self.handle_status_frame(frame[1:])
            sleep (0.01)

    def handle_status_frame(self,frame):
        self.chi_status.SOFTWARE_VERSION = frame[0]
        self.chi_status.reset_type = frame[1]
        self.chi_status.device_mode = frame[2]
        self.chi_status.no_cycles = int.from_bytes(frame[3:5],byteorder='big')    

        self.SOFTWARE_VERSION.setText("CHIMERA Software: V."+str(self.chi_status.SOFTWARE_VERSION))
        self.reset_type.setText("Reset type: "+str(self.chi_status.reset_type))
        self.device_mode.setText("Device Mode: " + str(self.chi_status.device_mode)) 
        self.no_cycles.setText("Number of Cycles: " + str(self.chi_status.no_cycles))

    def handle_SCI_TM_frame(self, frame):
        self.chi_sci_tm.local_time = int.from_bytes(frame[:4],byteorder='little')    
        self.chi_sci_tm.mem_to_test = int.from_bytes(frame[4:6],byteorder='little')    
        #for i in range(1,13):
        #    self.chi_sci_tm.no_SEU[i-1] = frame[5*i]

        self.local_time.setText("CHIMERA time: " + str(self.chi_sci_tm.local_time)) 
        ki._logger.debug(frame[4])
        memories_lower = frame[4]
        memories_upper = frame[5]
        for i in range(0,6):
            if (memories_lower>>i & 1):
                self.leds[i].setStyleSheet("QWidget { background-color: %s }" %  
                    self.green.name())
            else:
                self.leds[i].setStyleSheet("QWidget { background-color: %s }" %  
                    self.red.name())
        for i in range(6,12):
            if (memories_upper>>(i-6) & 1):
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
        for i in range(0,6):
            if (self.checkboxes[i].isChecked()):
                memories_lower |= (1<<i)
        for i in range(6,12):
            if (self.checkboxes[i].isChecked()):
                memories_upper |= (1<<(i-6))
        frame += bytes([memories_lower, memories_upper])
        ki.write(frame)

    def get_sci_tcm(self):
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
    
    ki = KISS(port='com7', speed='115200', pirate=True)
    ki.start()
    sr_read_thread = threading.Thread(target=ki.simpleread)
    sr_read_thread.daemon = True # stop when main thread stops
    sr_read_thread.start()

    app = QApplication(sys.argv)
    ex = GroundSoftware(ki)

    frame_thread = threading.Thread(target=ex.update_frame_view)
    frame_thread.daemon = True # stop when main thread stops
    frame_thread.start()

    #read frames


    sys.exit(app.exec_())
