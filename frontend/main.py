from flask import Flask
from landpage import landpage


app = Flask(__name__)

@app.route('/')
def landpage():
    return landpage.landpage()

@app.route("midi/")
def midi():
    return landpage.midi()

@app.route("keyboard/")
def keyboard():
    return landpage.keyboard()

@app.route("joystick/")
def joystick():
    return landpage.joystick()

@app.route("mouse/")
def mouse():
    return landpage.mouse()

@app.route("dmx/")
def dmx():
    return landpage.dmx()

@app.route("noname/")
def noname():
    return landpage.noname()

if __name__ == '__main__':
    app.run()