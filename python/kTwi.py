#        __            ____
#       / /__ _  __   / __/                      __  
#      / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
#     / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
#    /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
#                      /_/   github.com/KitSprout    
#   
#   @file    kTwi.py
#   @author  KitSprout
#   @date    Dec-2019
#   @brief   

import time
from kSerial import kSerial

class kTwi(kSerial):

    s = []

    def __init__(self, port="COM3", baudrate=115200):
        self.s = kSerial(port, baudrate)

    def write(self, address, regsister, data):
        pkdata, pkcount = self.s.send([address * 2 + 1, regsister], 'R1', data)
        return pkcount

    def read(self, address, regsister, lenght):
        pkdata, pkcount = self.s.send([address * 2 + 1, regsister], 'R1', lenght)
        self.delay(0.5)
        pkinfo, pkdata, pkcount = self.s.recv()
        return pkdata[0]

    def scandevice(self, mode=[]):
        pkdata, pkcount = self.s.send([171, 0], 'R2', [])
        self.delay(0.1)
        pkinfo, pkdata, pkcount = self.s.recv()
        devaddress = pkdata[0]
        devcount = len(devaddress)
        if mode != [] and mode == 'printon':
            print('')
            print(' >> i2c device list (found %d device)' %(devcount))
            print('    ', end='')
            for i in range(0, devcount):
                print(' %02X' %(devaddress[i]), end='')
            print('\n')

        return devaddress, devcount

    def scanregister(self, address, mode=[]):
        pkdata, pkcount = self.s.send([203, address*2], 'R2', [])
        self.delay(0.5)
        pkinfo, pkdata, pkcount = self.s.recv()
        devregister = pkdata[0]
        if mode != [] and mode == 'printon':
            print('')
            print(' >> i2c device register (address %02X)\n' %(address));
            print('      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F');
            for i in range(0, 256, 16):
                print(' %02X:' %(i), end='')
                for j in range(0, 16):
                    print(' %02X' %(devregister[i + j]), end='')
                print('')
            print('')

        return devregister, address

    def delay(self, delaysecond):
        time.sleep(delaysecond)
