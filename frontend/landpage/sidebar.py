import glob
import os
import landpage.settings as settings

class sidebar():
    def __init__(self):
        self.ctx = {}
    def get(self):
        json_files = glob.glob(settings.XML_LOCATION + "/conboard/boards/*.json")
        midis = []
        keyboards = []
        dmxs = []
        mice = []
        joysticks = []
        none_type = []
        for json in json_files:
            tree = json.load(json)
            if tree['DEVICE'] is not None:
                device = tree['DEVICE']
                if 'type' in device:
                    device['FileName'] = os.path.basename(json)
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


        