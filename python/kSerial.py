#        __            ____
#       / /__ _  __   / __/                      __  
#      / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
#     / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
#    /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
#                      /_/   github.com/KitSprout    
#   
#   @file    kSerial.py
#   @author  KitSprout
#   @date    Dec-2019
#   @brief   kSerial packet format :
#            byte 1   : header 'K' (75)       [HK]
#            byte 2   : header 'S' (83)       [HS]
#            byte 3   : data bytes (12-bit)   [L ]
#            byte 4   : data type             [T ]
#            byte 5   : parameter 1           [P1]
#            byte 6   : parameter 2           [P2]
#            byte 7   : checksum              [CK]
#             ...
#            byte L-1 : data                  [DN]
#            byte L   : finish '\r' (13)      [ER]

import struct
import serial
import numpy as np

class kSerial:

    typesize = {
        'uint8'  :  1, 'uint16' :  2, 'uint32' :  4, 'uint64' :  8,
        'int8'   :  1, 'int16'  :  2, 'int32'  :  4, 'int64'  :  8,
        'half'   :  2, 'float'  :  4, 'double' :  8,
        'R0'     :  1, 'R1'     :  1, 'R2'     :  1, 'R3'     :  1, 'R4'     :  1,
        0 : 1,  1 : 2,  2 : 4,  3 : 8,
        4 : 1,  5 : 2,  6 : 4,  7 : 8,
        9 : 2, 10 : 4, 11 : 8,
        8 : 1, 12 : 1, 13 : 1, 14 : 1,  15: 1,
    }
    typeconv = {
        'uint8'  :  0, 'uint16' :  1, 'uint32' :  2, 'uint64' :  3,
        'int8'   :  4, 'int16'  :  5, 'int32'  :  6, 'int64'  :  7,
        'half'   :  9, 'float'  : 10, 'double' : 11,
        'R0'     :  8, 'R1'     : 12, 'R2'     : 13, 'R3'     : 14, 'R4'     : 15,
         0: 'uint8',  1: 'uint16', 2: 'uint32', 3: 'uint64',
         4: 'int8',   5: 'int16',  6: 'int32',  7: 'int64',
         9: 'half',  10: 'float', 11: 'double',
         8: 'R0',    12: 'R1',    13: 'R2',    14: 'R3',     15: 'R4'
    }
    typestructconv = {
        'uint8'  : 'B', 'uint16' : 'H', 'uint32' : 'I', 'uint64' : 'Q',
        'int8'   : 'b', 'int16'  : 'h', 'int32'  : 'i', 'int64'  : 'q',
        'half'   : 'e', 'float'  : 'f', 'double' : 'd',
        'R0'     : 'B', 'R1'     : 'B', 'R2'     : 'B', 'R3'     : 'B', 'R4'     : 'B',
        0 : 'B',  1 : 'H',  2 : 'I',  3 : 'Q',
        4 : 'b',  5 : 'h',  6 : 'i',  7 : 'q',
        9 : 'e', 10 : 'f', 11 : 'd',
        8 : 'B', 12 : 'B', 13 : 'B', 14 : 'B',  15: 'B',
    }

    def __init__(self, port="COM3", baudrate=115200):
        self.s = serial.Serial()
        self.port = port
        self.baudrate = baudrate

    def open(self):
        self.s.port = self.port
        self.s.baudrate = self.baudrate
        self.s.open()

    def close(self):
        self.s.close()

    def is_open(self):
        return self.s.is_open

    def get_bytes_available(self):
        return self.s.in_waiting

    def clear(self):
        self.s.reset_input_buffer()

    def write(self, data):
        self.s.write(data)

    def read(self, lens):
        return self.s.read(lens)

    def find(self, packet, val):
        return [i for i, v in enumerate(packet) if v == val]

    # param = bytes(2), ktype = string, data = int or list
    def pack(self, param = [], ktype = 'R0', data = []):

        # get data lens
        if data != []:
            if type(data) is int:
                lens = 1
                pdata = bytearray(struct.pack(self.typestructconv[ktype], data))
            else:
                lens = len(data)
                pdata = bytearray(struct.pack(self.typestructconv[ktype], data[0]))
                for i in range(1, lens):
                    pdata += bytearray(struct.pack(self.typestructconv[ktype], data[i]))
        else:
            lens = 0

        # pack
        packet_bytes = self.typesize[ktype] * lens
        packet_type = self.typeconv[ktype] + int(packet_bytes / 256) * 16

        packet = bytes(b'KS')               # header 'KS'
        packet += bytes([packet_bytes])     # data bytes
        packet += bytes([packet_type])      # data type
        if param != []:
            packet += bytes(param)          # parameter 1, 2
        else:
            packet += bytes(2)
        checksum = (packet[2] + packet[3] + packet[4] + packet[5]) % 256
        packet += bytes([checksum])         # checksum
        if packet_bytes != 0:               # data ...
            packet += pdata
        packet += bytes(b'\r')              # finish '\r'

        return packet

    def unpack(self, packet = []):
        # empty data
        if packet == []:
            return [], []

        # conv to array
        packet = np.array(list(packet))
        packet_bytes = len(packet)

        # check 'KS'
        Ki = np.where(packet == 75)[0]
        Si = np.where(packet == 83)[0]
        Ki = np.array(sorted(list(set(Ki) & set(Si - 1))))
        if Ki == []:
            return [], []

        # check last packet
        if Ki[-1] + 8 > packet_bytes:
            np.delete(Ki, -1)
            if Ki == []:
                return [], []

        # get packet info and checksum
        Ld  = packet[Ki + 2]
        Td  = packet[Ki + 3]
        P1d = packet[Ki + 4]
        P2d = packet[Ki + 5]
        CKd = packet[Ki + 6]

        # check checksum
        checksum = (Ld + Td + P1d + P2d) % 256
        rm = np.where((CKd == checksum) != True)[0]
        if rm != []:
            Ki = np.delete(Ki, rm)
            if Ki != []:
                Ld  = packet[Ki + 2]
                Td  = packet[Ki + 3]
                P1d = packet[Ki + 4]
                P2d = packet[Ki + 5]
                CKd = packet[Ki + 6]
            else:
                return [], []

        # get lens, type, parameter
        packet_lens = Ld + np.trunc(Td / 16).astype(int) * 256
        packet_type = Td % 16

        # check last packet
        if Ki[-1] + packet_lens[-1] + 8 > packet_bytes:
            np.delete(Ki, -1)
            if Ki != []:
                Ld  = packet[Ki + 2]
                Td  = packet[Ki + 3]
                P1d = packet[Ki + 4]
                P2d = packet[Ki + 5]
                CKd = packet[Ki + 6]
                packet_lens = Ld + np.trunc(Td / 16).astype(int) * 256
                packet_type = Td % 16
            else:
                return [], []

        # check '/r'
        Ed = packet[Ki + packet_lens + 7]
        rm = np.where((Ed == 13) != True)[0]
        if rm != []:
            Ki = np.delete(Ki, rm)
            if Ki != []:
                Ld  = packet[Ki + 2]
                Td  = packet[Ki + 3]
                P1d = packet[Ki + 4]
                P2d = packet[Ki + 5]
                CKd = packet[Ki + 6]
                packet_lens = Ld + np.trunc(Td / 16).astype(int) * 256
                packet_type = Td % 16
            else:
                return [], []

        # get packet data
        Ki = np.array(Ki)
        cnt = 0
        data = ()
        for i in Ki + 7:
            Dd = packet[i:i+packet_lens[cnt]]
            cnt = cnt + 1
            data += (list(Dd),)

        # info(1) : data length (bytes)
        # info(2) : data type
        # info(3) : parameter 1
        # info(4) : parameter 2
        # info(5) : checksum
        info = list(packet_lens), list(packet_type), list(P1d), list(P2d), list(CKd)

        return info, data

    def send(self, param = [], ktype = 'R0', data = []):
        pkdata = self.pack(param, ktype, data)
        pkcount = len(pkdata)
        if pkcount:
            self.write(pkdata)
        return pkdata, pkcount

    def recv(self):
        rd = self.read(self.get_bytes_available())
        pkinfo, pkdata = self.unpack(rd)
        pkcount = len(pkdata)
        pkdatalens = []
        pk = ()
        for i in range(0, pkcount):
            if pkdata[i] != []:
                typesize = self.typesize[pkinfo[1][i]]
                lens = int(pkinfo[0][i] / typesize)
                pkdatalens.append(lens)
                pd = []
                for k in range(0, lens):
                    pd.append(struct.unpack(self.typestructconv[pkinfo[1][i]], bytes(pkdata[i][0:typesize]))[0])
                    del pkdata[i][0:typesize]
            else:
                pd = []
            pkinfo[1][i] = self.typeconv[pkinfo[1][i]]
            pk += (pd,)

        # add packet data lens to info
        pkinfo += (pkdatalens,)
        pkdata = pk

        return pkinfo, pkdata, pkcount
