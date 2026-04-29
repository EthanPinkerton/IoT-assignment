#run server with:
#flask --app server.py run --host=0.0.0.0

from flask import Flask
from flask import request

def get_html_file(filename):
  with open(filename, "r") as f:
    return f.read()

def write_temp(temp):
  f = open("tempurature.txt", "w")
  f.write(temp)
  f.close()

def read_temp():
  with open("tempurature.txt", "r") as f:
    return f.readlines()[0].strip()

def write_mode():
  open('LEDs.txt', 'w')

index = get_html_file("index.html")

app = Flask(__name__)

@app.route("/")
def hello_world():
    return get_html_file("index.html")

@app.route("/tempurature")
def get_temp():
  return read_temp()

@app.get("/send_temp")
def store_temp():
  temp = request.args.get('temp', "None")
  mode = request.args.get('mode', "None")
  leds = request.args.get('LEDs', "None")
  write_temp(temp + "\n" + mode + "\n" + leds + "\n")
  return ""

@app.post("/change_mode")
def store_mode():
  write_mode()
  return ""

@app.get("/get_data")
def change_mode():
  return
