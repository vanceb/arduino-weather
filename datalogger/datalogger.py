#! /usr/bin/python

from xbee import ZigBee
import serial
import struct

serial_port = serial.Serial('/dev/ttyUSB0', 9600)

def print_data(data):
    print
    print len(data)
    values = struct.unpack("Iihhhh", data[2:18])
    print values
#    print "Msg Type " + values[0]
#    print "Length " + str(values[1])
    print "Millis " + str(values[0])
    print "pressure " + str(values[1]/100.0)
    print "inT1 " + str(values[2]/100.0)
    print "inT2 " + str(values[3]/100.0)
    print "outT " + str(values[4]/100.0)
    print "light " + str(values[5])

xbee = ZigBee(serial_port, escaped=True)

while True:
    try:
        frame=xbee.wait_read_frame()
        data = frame.get("rf_data")
        print_data(data)
    except KeyboardInterrupt:
        break

xbee.halt()
serial_port.close()
