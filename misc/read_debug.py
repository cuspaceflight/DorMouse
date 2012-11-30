#!/usr/bin/python

import sys
import serial

sp = serial.Serial(port="/dev/ttyUSB0", baudrate=115200, bytesize=8,
   parity=serial.PARITY_NONE, stopbits=1, xonxoff=0, rtscts=0, timeout=0.1)

while True:
    try:
        sys.stdout.write(sp.read(1))
        sys.stdout.flush()
    except KeyboardInterrupt:
        raise
    except:
        pass
