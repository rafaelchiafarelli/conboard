from xml.dom import minidom
import xml.etree.ElementTree as ElementTree
from landpage.xmlParser import XmlDictConfig
import landpage.settings as settings
import json
class DevModel():
    def __init__(self):
        self.DEVICE = []
        self.header = {}
        self.body = {}

    def GetHeader(self, filename):
        xml = settings.XML_LOCATION + "/../boards/" + filename
        with open(xml, "r") as xml_file:
            ctx = json.load(xml_file)
        print(ctx)
        return ctx

    def GetName(self, filename):
        xml = settings.XML_LOCATION + "/../boards/" + filename
        tree = ElementTree.parse(xml)
        root = tree.getroot()
        name = ''
        for dev in root.iter('DEVICE'):
            name = dev.attrib['name']
        return name
            