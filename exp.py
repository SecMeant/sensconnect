#!/usr/bin/python3

import socket
import struct

DEVREG = 0x00
SENSDATA = 0x01
GETSUM = 0x02

def p8(i: int):
    i &= 0xff
    return struct.pack('<B', i)

def p16(i: int):
    i &= 0xffff
    return struct.pack('<H', i)

def p32(i: int):
    i &= 0xffffffff
    return struct.pack('<i', i)

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(('localhost', 34756))

def devreg(sock, name, fields):
    msg = p8(DEVREG) + p16(len(name)) + name + p8(len(fields))

    for field in fields:
        msg += p8(len(field)) + field

    sock.sendall(msg)
    return s.recv(4096)

def sensdata(sock, name, field, value):
    msg = p8(SENSDATA) + p16(len(name)) + name + p8(len(field)) + field + p32(value)
    sock.sendall(msg)
    return s.recv(4096)

#s.sendall(b'\x00' + b'\x05\x00' + b'ASDFG' + b'\x02\x00' + b'\x04' + b'zxcv' + b'\x05' + b'qwer')
#print(s.recv(4096))

print(devreg(sock = s, name = b'ASDFGH\x00QWER', fields = (b'zxcv', b'qwer')))
print(sensdata(sock = s, name = b'ASDFGH', field = b'zxcv', value = 1337))

