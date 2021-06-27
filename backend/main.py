from flask import Flask
from flask_socketio import SocketIO, send

from DevComs import DevComs
from landpage import settings 
from landpage.landpage import landPage, midiPage, keyboardPage, joystickPage, mousePage, dmxPage, nonamePage


import os

# Get the current working directory
cwd = os.getcwd()
url_path = os.path.dirname(cwd.__str__()+"/static/dash/").__str__()
static = os.path.dirname(cwd.__str__()).__str__()
template_dir = os.path.dirname(cwd.__str__()+"/static/dash/").__str__()

STATIC_URL_PATH = url_path
STATIC_FOLDER = static
TEMPLATE_PATH = template_dir
print(url_path)
print(static)
print(template_dir)

app = Flask(__name__, static_folder=STATIC_FOLDER,
            static_url_path=STATIC_URL_PATH,
            template_folder=TEMPLATE_PATH)
app.config['SECRET_KEY'] = 'some_cool_secret'
socketio = SocketIO(app)

coms = DevComs(send=socketio.send)

@socketio.on('message')
def handle_message(data):
    print('received message: ' + data)

@app.route('/')
def land():
    return {
            "id": 1,
            "name": "Leanne Graham",
            "username": "Bret",
            "email": "Sincere@april.biz",
            "address": {
            "street": "Kulas Light",
            "suite": "Apt. 556",
            "city": "Gwenborough",
            "zipcode": "92998-3874",
            "geo": {
            "lat": "-37.3159",
            "lng": "81.1496"
            }
            },
            "phone": "1-770-736-8031 x56442",
            "website": "hildegard.org",
            "company": {
            "name": "Romaguera-Crona",
            "catchPhrase": "Multi-layered client-server neural-net",
            "bs": "harness real-time e-markets"
            }
            }

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

