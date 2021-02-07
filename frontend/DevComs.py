from xml.dom import minidom
import xml.etree.ElementTree as ElementTree
from landpage.xmlParser import XmlDictConfig
import landpage.settings as settings
import threading
import json
import zmq

from flask_socketio import send

class DevComs(threading.Thread):
    def __init__(self, send):
        self.DEVICE = []
        self.header = {}
        self.body = {}
        self.io_context = zmq.Context()
        self.coms_context = zmq.Context()

        self.io_socket = self.io_context.socket(zmq.PULL)
        self.io_socket.connect("tcp://localhost:5555")
        self.poller = zmq.Poller()
        self.poller.register(self.io_socket, zmq.POLLIN)

        self.coms_socket = self.coms_context.socket(zmq.REQ)
        self.coms_socket.connect("tcp://localhost:5550")

        self.alive = True
        self.send = send




    def run(self):
        #  Do 10 requests, waiting each time for a response
        while self.alive:
            socks = dict(self.poller.poll(100))
            if socks:
                print("received something")
                if socks.get(self.io_socket) == zmq.POLLIN:
                    #print "got message ",
                    rec_msg = self.io_socket.recv(zmq.NOBLOCK)
                    print(rec_msg)
                    self.send(rec_msg,broadcast=True)
            else:
                pass

    def SendReaload(self,dev):
        msg = "{} reload".format(dev)
        self.coms_socket.send(bytearray(msg))
        message =  self.coms_socket.recv()
        print("Received reply {}".format(message))

    def SendOutStop(self,dev):
        msg = "{} outstop".format(dev)
        self.coms_socket.send(bytearray(msg))
        message =  self.coms_socket.recv()
        print("Received reply {}".format(message))

    def SendFile(self,dev, filename):
        msg = "{} file {}".format(dev,filename)
        self.coms_socket.send(bytearray(msg))
        message =  self.coms_socket.recv()
        print("Received reply {}".format(message))