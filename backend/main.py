from flask import Flask
from flask_socketio import SocketIO, send


from landpage import settings 
from landpage import landpage
import json
import os

# Get the current working directory
cwd = os.getcwd()
url_path = os.path.dirname(cwd.__str__()+"/static/dash/").__str__()
static = os.path.dirname(cwd.__str__()).__str__()
template_dir = os.path.dirname(cwd.__str__()+"/static/dash/").__str__()

STATIC_URL_PATH = url_path
STATIC_FOLDER = static
TEMPLATE_PATH = template_dir

app = Flask(__name__, static_folder=STATIC_FOLDER,
            static_url_path=STATIC_URL_PATH,
            template_folder=TEMPLATE_PATH)
app.config['SECRET_KEY'] = 'some_cool_secret'



@app.route('/')
def land():
    return json.load("/conboard/boards/Dj4Mix.json")
