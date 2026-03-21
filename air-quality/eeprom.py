#!/usr/bin/python

import sys
import socket
import subprocess

if len(sys.argv) != 4:
    print(sys.argv[0] + " mac ip name")
    sys.exit(1)

mac = bytes.fromhex(sys.argv[1].replace(':', ''))
ip = socket.inet_aton(sys.argv[2])
topic = "nh/air".ljust(40, '\0').encode('ascii')
name = sys.argv[3].ljust(20, '\0').encode('ascii')
host = socket.inet_aton("192.168.0.1")

with open('config.bin', 'wb') as file:
    file.write(mac)
    file.write(ip)
    file.write(topic)
    file.write(name)
    file.write(host)

subprocess.run(["/usr/bin/avrdude", "-B125K", "-V", "-pm328p", "-D", "-catmelice_isp", "-Ueeprom:w:config.bin"])

