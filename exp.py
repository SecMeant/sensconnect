#!/usr/bin/python3

import socket
import struct

def p16(i: int):
    i &= 0xffff


s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(('localhost', 34756))

s.sendall(b'\x00' + b'\x05\x00' + b'ASDFG' + b'\x02\x00' + b'\x04' + b'zxcv' + b'\x05' + b'qwer')
print(s.recv(4096))
