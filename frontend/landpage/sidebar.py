import glob
import os
import landpage.settings as settings
from xml.dom import minidom
import xml.etree.ElementTree as ElementTree
from landpage.xmlParser import XmlDictConfig
from landpage.model import DevModel
class sidebar():
    def __init__(self):
        self.ctx = {}
    def get(self):
        xml_files = glob.glob(settings.XML_LOCATION + "/../boards/*.xml")
        context = {}
        midis = []
        keyboards = []
        dmxs = []
        mice = []
        joysticks = []
        none_type = []
        for xml in xml_files:
            tree = ElementTree.parse(xml)
            root = tree.getroot()
            device = XmlDictConfig(root)
            if device is not None:
                if 'type' in device:
                    device['FileName'] = os.path.basename(xml)
                    if device['type'] == 'midi':
                        midis.append(device)
                    elif device['type'] == 'keyboard':
                        keyboards.append(device)
                    elif device['type'] == 'joystick':
                        joysticks.append(device)
                    elif device['type'] == 'DMX':
                        dmxs.append(device)
                    elif device['type'] == 'mouse':
                        mice.append(device)
                    else:
                        none_type.append(device)
        self.ctx = {
            'midis':midis,
            'keyboards':keyboards,
            'dmxs':dmxs,
            'joysticks':joysticks,
            'mice':mice,
            'none_type':none_type
        }
        return self.ctx

    def getFile(self, filename):
        dev = DevModel()
        return dev.GetHeader(filename)