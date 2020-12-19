from flask import Flask
from landpage import landpage


app = Flask(__name__)

@app.route('/')
def here():
    return landpage.hello()

if __name__ == '__main__':
    app.run()