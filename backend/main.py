from flask import Flask
from flask_socketio import SocketIO, send

from DevComs import DevComs
from landpage import settings 
from landpage.landpage import landPage, midiPage, keyboardPage, joystickPage, mousePage, dmxPage, nonamePage


import os

# Get the current working directory
cwd = os.getcwd()
url_path = os.path.dirname(cwd.__str__()+"/../frontend/static").__str__()

STATIC_URL_PATH = url_path
STATIC_FOLDER = url_path
print(url_path)

app = Flask(__name__, static_folder=STATIC_FOLDER,
            static_url_path=STATIC_URL_PATH)
app.config['SECRET_KEY'] = 'some_cool_secret'
socketio = SocketIO(app)

coms = DevComs(send=socketio.send)

@socketio.on('message')
def handle_message(data):
    print('received message: ' + data)

@app.route('/')
def land():
    return landPage()

@app.route("/midi/<string:name>/<string:file>")
def midi(name, file):
    return midiPage(name = name, filename=file)

@app.route("/keyboard/<string:name>/<string:file>")
def keyboard(name,file):
    return keyboardPage()

@app.route("/joystick/<string:name>/<string:file>")
def joystick(name,file):
    return joystickPage(name=name, filename=file)

@app.route("/mouse/<string:name>/<string:file>")
def mouse(name,file):
    return mousePage(name=name, filename=file)

@app.route("/dmx/<string:name>/<string:file>")
def dmx(name,file):
    return dmxPage(name=name, filename=file)

@app.route("/noname/<string:file>")
def noname(file):
    return nonamePage(filename = file)

if __name__ == '__main__':
    socketio.run(app)
