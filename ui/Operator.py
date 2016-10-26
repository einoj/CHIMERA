#!/usr/bin/python3
# -*- coding: utf-8 -*-

"""
ZetCode PyQt5 tutorial 

In this example, we create three toggle buttons.
They will control the background colour of a 
QFrame. 

author: Jan Bodnar
website: zetcode.com 
last edited: January 2015
"""

import sys
from PyQt5.QtWidgets import (QWidget, QPushButton, 
    QFrame, QApplication, QLabel)
from PyQt5.QtGui import QColor


class Example(QWidget):
    
    def __init__(self):
        super().__init__()
        
        self.initUI()
        
        
    def initUI(self):      

        self.red = QColor(255, 0, 0)       
        self.green = QColor(0, 255, 0)

        statusb = QPushButton('Get Status', self)
        statusb.move(10, 10)

        statusb.clicked[bool].connect(self.getStatus)

        self.statusLayout()
       # redb = QPushButton('Green', self)
       # redb.setCheckable(True)
       # redb.move(10, 60)

       # redb.clicked[bool].connect(self.setColor)

       # blueb = QPushButton('Blue', self)
       # blueb.setCheckable(True)
       # blueb.move(10, 110)

       # blueb.clicked[bool].connect(self.setColor)

        
        
    def getStatus(self, pressed):
        
        self.led1.setStyleSheet("QFrame { background-color: %s }" %
            self.red.name())  

        self.led2.setStyleSheet("QFrame { background-color: %s }" %
            self.green.name())  
       
    def statusLayout(self):
        self.labels = []
        for i in range(12):
            label = QLabel(str(i+1),self)
            self.labels.append(label)
            #self.ledlabel1 = QLabel('1',self) 
            #self.ledlabel1.move(150, 5)
        xloc = 150
        for label in self.labels:
            label.move(xloc,5)
            xloc += 20

        self.led1 = QFrame(self)
        self.led1.setGeometry(150, 20, 10, 10)
        self.led1.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led2 = QFrame(self)
        self.led2.setGeometry(170, 20, 10, 10)
        self.led2.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led3 = QFrame(self)
        self.led3.setGeometry(190, 20, 10, 10)
        self.led3.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led4 = QFrame(self)
        self.led4.setGeometry(210, 20, 10, 10)
        self.led4.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led5 = QFrame(self)
        self.led5.setGeometry(230, 20, 10, 10)
        self.led5.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led6 = QFrame(self)
        self.led6.setGeometry(250, 20, 10, 10)
        self.led6.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led7 = QFrame(self)
        self.led7.setGeometry(270, 20, 10, 10)
        self.led7.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led8 = QFrame(self)
        self.led8.setGeometry(290, 20, 10, 10)
        self.led8.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led9 = QFrame(self)
        self.led9.setGeometry(310, 20, 10, 10)
        self.led9.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led10 = QFrame(self)
        self.led10.setGeometry(330, 20, 10, 10)
        self.led10.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led11 = QFrame(self)
        self.led11.setGeometry(350, 20, 10, 10)
        self.led11.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())

        self.led12 = QFrame(self)
        self.led12.setGeometry(370, 20, 10, 10)
        self.led12.setStyleSheet("QWidget { background-color: %s }" %  
            self.red.name())
        
        self.setGeometry(300, 300, 400, 170)
        self.setWindowTitle('Toggle button')
        self.show()
       
if __name__ == '__main__':
    
    app = QApplication(sys.argv)
    ex = Example()
    sys.exit(app.exec_())
