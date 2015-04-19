#! /usr/bin/python
import os
from datetime import datetime
import json
import logging
import logging.config
import requests

from xbee import ZigBee
import serial
import struct

# Environment variable that indicates linked Docker container for data logging
DOCKER_WS = 'WEATHER_LOGGER_PORT_5000_TCP'

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

# Load the friendly name lookups from json
friendly = {"appIDs":{}, "msgTypes":{}, "xbees":{}}
def getFriendly(default_path="friendly.json"):
    global friendly
    path = default_path
    if os.path.exists(path):
        with open(path, 'rt') as f:
            friendly = json.load(f)
        logging.info("Loaded friendly names: " + str(friendly))

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
        self.msg["source"] = "0x%0.16X" % struct.unpack(">Q",self.frame.get("source_addr_long"))
        # convert to friendly name if we have defined it
        if self.msg["source"] in friendly["xbees"]:
            self.msg["source"] = friendly["xbees"][self.msg["source"]]
        self.appLog.debug("Got message")
        self.msg["logtime"] = datetime.isoformat(datetime.now())

        decodeHdr = struct.unpack("HHHH", self.rfdata[0:8])
        self.msg["appID"]= "0x%0.4X" % decodeHdr[0]
        self.msg["msgType"] = "0x%0.4X" % decodeHdr[1]
        # convert to friendly names if we have defined them
        if self.msg["appID"] in friendly["appIDs"]:
            self.msg["appID"] = friendly["appIDs"][self.msg["appID"]]
            if self.msg["appID"] in friendly["msgTypes"]:
                if self.msg["msgType"] in friendly["msgTypes"][self.msg["appID"]]:
                    self.msg["msgType"] = friendly["msgTypes"][self.msg["appID"]][self.msg["msgType"]]
        self.msg["reserved"] = decodeHdr[2]
        self.msg["length"] = decodeHdr[3]
        self.msg["data"] = self.rfdata[8:]
        if self.msg["length"] != len(self.msg["data"]):
            self.appLog.error("Incorrect data length in received packet. Rx: %s, Expected: %s" % (len(self.msg["data"]), self.header["length"]))
        else:
            if self.msg["appID"] in self.appHandlers:
                self.appLog.debug("Handling application ID: %s" % self.msg["appID"])
                return self.appHandlers[self.msg["appID"]].decode(self.msg)
            else:
                self.appLog.warn("No handler registered for appID %s, dropping message..." % self.msg["appID"])
        return []

    def register(self, appID, handler):
        self.appHandlers[appID] = handler
        self.appLog.info("Registered handler for appID: %s" % appID)


# a handler class to allow indirection of message handling
class appHandler:
    def __init__(self, parent, appID, appLog=None):
        self.appLog = appLog or logging.getLogger(__name__)
        self.parent = parent
        # Convert appID to friendly name if we have defined one
        if appID[0:2] == "0x":
            if appID in friendly["appIDs"]:
                self.appID = friendly["appIDs"][appID]
                self.appLog.info("Converted appID %s to friendly name: %s" % (appID, self.appID))
            else:
                self.appID = appID
                self.appLog.info("Unable to convert appID %s to a friendly name" % appID)
        self.msgHandlers = {}
        self.parent.register(self.appID, self)

    def register(self, msgType, handler):
        msgName = msgType
        # Convert to friendly name if we have defined one
        if msgType[0:2] == "0x":
            if self.appID in friendly["msgTypes"]:
                self.appLog.debug("Looking up msgType %s in appID %s", (msgType, self.appID))
                if msgType in friendly["msgTypes"][self.appID]:
                    msgName = friendly["msgTypes"][self.appID][msgType]
                    self.appLog.info("Converted msgType %s to friendly name %s" % (msgType, msgName))
                else:
                    self.appLog.info("Unable to convert msgType %s to friendly name", msgType)
        self.msgHandlers[msgName] = handler
        self.appLog.info("Registered handler for msgType: %s" % msgName)

    def decode(self, msg, log=True):
        if self.appID != msg["appID"]:
            self.appLog.error("Passed a message that I didn't register for.  My appID: %s, message appID: %s" % (self.appID, msg["appID"]))
        else:
            if msg["msgType"] in self.msgHandlers:
                self.appLog.debug("Handling message type: %s" % msg["msgType"])
                msg = self.msgHandlers[msg["msgType"]].decode(msg)
                if log:
                    self.logCSV(msg)
                return msg
            else:
                self.appLog.warn("No handler registered for msgType %s, dropping message..." % msg["msgType"])
        return None

    def logCSV(self, msg):
        logger_name = "%s.%s.%s" % (msg["appID"], msg["msgType"], msg["source"])
        msg["logger_name"] = logger_name
        logger = logging.getLogger(logger_name)
        msg = self.msgHandlers[msg["msgType"]].decode(msg)
        if "csv" in msg:
            logger.info(msg["csv"])
        else:
            self.appLog.error("No CSV data has been decoded.  Have you specified the fields to be listed in the CSV?")
        return msg

# The msgHandler class is a prototype class that should be subclassed to do something useful
class msgHandler:
    def __init__(self, parent, msgTypes=[], appLog=None):
        self.appLog = appLog or logging.getLogger(__name__)
        self.msgTypes = msgTypes
        self.parent = parent
        self.CSVFields = None
        self.JSONFields = None
        for msgType in msgTypes:
            self.appLog.debug ("Registering as handler for msgType %s" % msgType)
            self.parent.register(msgType, self)


    def decode(self, msg):
        # this is where the final message decoding happens - return an object containing the message items
        # do the work here and add values into the msg Dictionary
        return msg

    def createFields(self, msg):
        if self.CSVFields != None:
            csv = []
            self.appLog.debug("Creating CSV logline using the following fields: " + str(self.CSVFields))
            for ref in self.CSVFields:
                if ref in msg:
                    csv.append(str(msg[ref]))
                else:
                    self.appLog.error("Error creating CSV, requested item not available: %s" % ref)
            msg["csv"] = ','.join(csv)

        if self.JSONFields != None:
            csv = {}
            self.appLog.debug("Creating JSON data using the following fields: " + str(self.CSVFields))
            for ref in self.JSONFields:
                if ref in msg:
                    csv[ref] = msg[ref]
                else:
                    self.appLog.error("Error creating JSON, requested item not available: %s" % ref)
            msg["json"] = json.dumps(csv)

        return msg

    def setCSVFields(self, fields=None):
        self.appLog.info("Setting CSV Fields to: " + str(fields))
        self.CSVFields = fields

    def getCSVFields(self):
        return self.CSVFields

    def setJSONFields(self, fields = None):
        self.appLog.info("Setting JSON Fields to: " + str(fields))
        self.JSONFields = fields

    def getJSONFields(self):
        return self.JSONFields


# a concrete class that unpacks the data for a protocol and implements a msgHandler
class weatherHandler(msgHandler):
    def __init__(self, parent):
        msgHandler.__init__(self, parent, ["0x0001"])
        self.setCSVFields(["logtime", "millis", "inT1", "inT2", "inT3", "outT", "pressure", "humidity", "light", "battery", "solar"])
        self.setJSONFields(["logtime", "millis", "inT1", "inT2", "inT3", "outT", "pressure", "humidity", "light", "battery", "solar"])

    def decode(self, msg):
        values = struct.unpack("Iihhhhhhhh", msg["data"])
        msg["millis"] = values[0]
        msg["pressure"] = values[1]/100.0
        msg["inT1"] = values[2]/100.0
        msg["inT2"] = values[3]/100.0
        msg["inT3"] = values[4]/100.0
        msg["outT"] = values[5]/100.0
        msg["humidity"] = values[6]/100.0
        msg["light"] = values[7]
        msg["battery"] = values[8]/155.0
        msg["solar"] = values[9]/155.0

        return self.createFields(msg)

class wsPostData():
    def __init__(self, ws=None, env=None, appLog=None):
        self.appLog = appLog or logging.getLogger(__name__)
        self.ws = ws
        if env is not None:
            self.appLog.info("Attempting to configure linked Web Service from: $" + env)
            # Use Docker Linked Container environment variables for setup
            self.env = os.environ.get(env, None)
            if self.env is not None:
                self.ws = "http" + self.env[3:] + "/data"
            else:
                self.appLog.warning("Unable to configure web service from environment variable: $" + env)
        self.appLog.info("Configured web service: %s" % str(self.ws))

    def postData(self, data):
        if self.ws is not None:
            self.appLog.debug("Attempting to post data to: " + self.ws)
            try:
                r = requests.post(self.ws, data=data, headers={'content-type': 'application/json'})
                if r.json['status'] == 'OK':
                    return True
                else:
                    return False
            except:
                return False

if __name__ == '__main__':
    # Configure the logs
    setup_logging(default_level=logging.DEBUG)
    getFriendly()
    datalog = logging.getLogger("data")
    datalog.info("Starting logging...")
    zbdl = zbDataLogger()
    weatherApp = appHandler(zbdl, "0x10A1")
    weatherMsg = weatherHandler(weatherApp)
    log2ws = wsPostData(env=DOCKER_WS)
    while True:
        data = zbdl.getMsg()
        datalog.info(data["csv"])
#        log2ws.postData(data["json"])

