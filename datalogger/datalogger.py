#! /usr/bin/python
import os
from datetime import datetime
import json
import logging
import logging.config

from xbee import ZigBee
import serial
import struct


# Load logging config from logging.json
def setup_logging(default_path='logging.json', default_level=logging.INFO, env_key='LOG_CFG'):
    path = default_path
    value = os.getenv(env_key, None)
    if value:
        path = value
    if os.path.exists(path):
        with open(path, 'rt') as f:
            config = json.load(f)
        logging.config.dictConfig(config)
        logging.info("Configured logging from json")
    else:
        logging.basicConfig(level=default_level)
        logging.info("Configured logging basic")


class zbDataLogger:
    def __init__(self, port='/dev/ttyUSB0', baud=9600, escaped=True, appLog=None):
        self.appLog = appLog or logging.getLogger(__name__)
        self.port = port
        self.baud = baud
        self.escaped = escaped
        try:
            self.serial_port = serial.Serial(self.port, self.baud)
            self.xbee = ZigBee(self.serial_port, escaped=self.escaped)
            self.appLog.info("Successfully initialised ZigBee on " + self.port + " at " + str(self.baud) + " baud")
        except:
            self.appLog.error("Unable to initialise Zigbee on " + self.port + " at " + str(self.baud) + " baud")
            raise
        self.frame = ""
        self.msg = {}
        self.data = ""
        self.appHandlers = {}

    def getMsg(self):
        self.frame = self.xbee.wait_read_frame() # blocking
        self.rfdata = self.frame.get("rf_data")
        self.appLog.debug("Got message")
        self.msg["logtime"] = datetime.isoformat(datetime.now())

        decodeHdr = struct.unpack("HHHH", self.rfdata[0:8])
        self.msg["appID"] = decodeHdr[0]
        self.msg["msgType"] = decodeHdr[1]
        self.msg["reserved"] = decodeHdr[2]
        self.msg["length"] = decodeHdr[3]
        self.msg["data"] = self.rfdata[8:]
        if self.msg["length"] != len(self.msg["data"]):
            self.appLog.error("Incorrect data length in received packet. Rx: %s, Expected: %s" % (len(self.msg["data"]), self.header["length"]))
        else:
            if self.msg["appID"] in self.appHandlers:
                self.appLog.debug("Handling application ID: 0x%0.4X" % self.msg["appID"])
                return self.appHandlers[self.msg["appID"]].decode(self.msg)
            else:
                self.appLog.warn("No handler registered for appID 0x%0.4X, dropping message..." % self.msg["appID"])
        return []

    def register(self, appID, handler):
        self.appHandlers[appID] = handler
        self.appLog.info("Registered handler for appID: 0x%0.4X" % appID)


# a handler class to allow indirection of message handling
class appHandler:
    def __init__(self, parent, appID, appLog=None):
        self.appLog = appLog or logging.getLogger(__name__)
        self.parent = parent
        self.appID = appID
        self.msgHandlers = {}
        self.parent.register(self.appID, self)

    def register(self, msgType, handler):
        self.msgHandlers[msgType] = handler
        self.appLog.info("Registered handler for msgType: 0x%0.4X" % msgType)

    def decode(self, msg):
        if self.appID != msg["appID"]:
            self.appLog.error("Passed a message that I didn't register for.  My appID: 0x%0.4X, message appID: 0x%0.4X" % (self.appID, msg["appID"]))
        else:
            if msg["msgType"] in self.msgHandlers:
                self.appLog.debug("Handling message type: 0x%0.4X" % msg["msgType"])
                return self.msgHandlers[msg["msgType"]].decode(msg)
            else:
                self.appLog.warn("No handler registered for msgType 0x%0.4X, dropping message..." % msg["msgType"])
        return []


class msgHandler:
    def __init__(self, parent, msgTypes=[], appLog=None):
        self.appLog = appLog or logging.getLogger(__name__)
        self.msgTypes = msgTypes
        for msgType in msgTypes:
            self.appLog.debug ("Registering as handler for msgType 0x%0.4X" % msgType)
            parent.register(msgType, self)


    def decode(self, msg):
        # this is where the final message decoding happens - return an object containing the message items
        # do the work here and add values into the msg Dictionary
        return msg

    def createCSV(self, msg, listOfRefs):
        # you MUST also provide a "csv" member containing a CSV of the data to be logged
        csv = []
        self.appLog.debug("Creating csv logline using the following fields: " + str(listOfRefs))
        for ref in listOfRefs:
            if ref in msg:
                csv.append(str(msg[ref]))
            else:
                self.appLog.error("Error creating CSV, requested item not available: %s" % ref)
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
    # Configure the logs
    setup_logging(default_level=logging.DEBUG)
    datalog = logging.getLogger("data")
    datalog.info("Starting logging...")
    zbdl = zbDataLogger()
    weatherApp = appHandler(zbdl, 0x10a1)
    weatherMsg = weatherHandler(weatherApp)
    while True:
        data = zbdl.getMsg()
        datalog.info(data["csv"])

