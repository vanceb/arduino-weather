#! /usr/bin/python

from xbee import ZigBee
import serial
import struct
from datetime import datetime
import logging

logging.basicConfig(filename="./datalogger.log", format='%(asctime)s %(levelname)s:%(message)s', level=logging.INFO)

serial_port = serial.Serial('/dev/ttyUSB0', 9600)

class zbDataLogger:
    def __init__(self, port='/dev/ttyUSB0', baud=9600, escaped=True):
        self.port = port
        self.baud = baud
        self.escaped = escaped
        try:
            self.serial_port = serial.Serial(self.port, self.baud)
            self.xbee = ZigBee(self.serial_port, escaped=self.escaped)
            logging.info("Successfully initialised ZigBee on " + self.port + " at " + str(self.baud) + " baud")
        except:
            logging.error("Unable to initialise Zigbee on " + self.port + " at " + str(self.baud) + " baud")
            raise
        self.frame = ""
        self.msg = {}
        self.data = ""
        self.appHandlers = {}

    def getMsg(self):
        self.frame = self.xbee.wait_read_frame() # blocking
        self.rfdata = self.frame.get("rf_data")
        logging.info("Got message")
        self.msg["logtime"] = datetime.isoformat(datetime.now())

        decodeHdr = struct.unpack("HHHH", self.rfdata[0:8])
        self.msg["appID"] = decodeHdr[0]
        self.msg["msgType"] = decodeHdr[1]
        self.msg["reserved"] = decodeHdr[2]
        self.msg["length"] = decodeHdr[3]
        self.msg["data"] = self.rfdata[8:]
        if self.msg["length"] != len(self.msg["data"]):
            logging.error("Incorrect data length in received packet. Rx: " + str(len(self.msg["data"])) + " Expected: " + str(self.header["length"]))
        else:
            if self.msg["appID"] in self.appHandlers:
                logging.info("Handling application ID: " + str(self.msg["appID"]))
                return self.appHandlers[self.msg["appID"]].decode(self.msg)
            else:
                logging.warn("No handler registered for appID " + str(self.msg["appID"]) + " dropping message...")
        return []

    def register(self, appID, handler):
        self.appHandlers[appID] = handler
        logging.info("Registered handler for appID: " + str(appID))

# a handler class to allow indirection of message handling
class appHandler:
    def __init__(self, parent, appID):
        self.parent = parent
        self.appID = appID
        self.msgHandlers = {}
        self.parent.register(self.appID, self)

    def register(self, msgType, handler):
        self.msgHandlers[msgType] = handler
        logging.info("Registered handler for msgType: " + str(msgType))

    def decode(self, msg):
        if self.appID != msg["appID"]:
            logging.error("Passed a message that I didn't register for.  My appID: " + str(self.appID) + ", message appID " + str(msg["appID"]))
        else:
            if msg["msgType"] in self.msgHandlers:
                logging.info("Handling message type: " + str(msg["msgType"]))
                return self.msgHandlers[msg["msgType"]].decode(msg)
            else:
                logging.warn("No handler registered for msgType " + str(msg["msgType"]) + " dropping message...")
        return []


class msgHandler:
    def __init__(self, parent, msgTypes=[]):
        self.msgTypes = msgTypes
        for msgType in msgTypes:
            parent.register(msgType, self)


    def decode(self, msg):
        # this is where the final message decoding happens - return an object containing the message items
        # do the work here and add values into the msg Dictionary
        # you MUST also provide a "csv" member containing a CSV of the data to be logged
        return msg

    def createCSV(self, msg, listOfRefs):
        csv = []
        for ref in listOfRefs:
            if ref in msg:
                csv.append(str(msg[ref]))
            else:
                logging.error("Error creating CSV, requested item not available: " + str(ref))
        return ','.join(csv)


class weatherHandler(msgHandler):
    def __init__(self, parent):
        msgHandler.__init__(self, parent, [0x0001])

    def decode(self, msg):
        values = struct.unpack("Iihhhhhh", msg["data"])
        msg["millis"] = values[0]
        msg["pressure"] = values[1]/100.0
        msg["inT1"] = values[2]/100.0
        msg["inT2"] = values[3]/100.0
        msg["inT3"] = values[4]/100.0
        msg["outT"] = values[5]/100.0
        msg["humidity"] = values[6]/100.0
        msg["light"] = values[7]

        msg["csv"]= self.createCSV(msg, ["logtime", "millis", "inT1", "inT2", "inT3", "outT", "pressure", "humidity", "light"])

        return msg;


if __name__ == '__main__':
    zbdl = zbDataLogger()
    weatherApp = appHandler(zbdl, 0x10a1)
    weatherMsg = weatherHandler(weatherApp)
    with open('data.log', 'a', 1) as dataLogfile:
        while True:
            data = zbdl.getMsg()
            dataLogfile.write(data["csv"] + '\n')

