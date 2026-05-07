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

def write_mode(mode):
  f = open("tempurature.txt", "w")
  f.write(read_temp() + "\n" + mode + "\n" + read_LEDs() + "\n")
  f.close()

def write_leds(leds):
  f = open("tempurature.txt", "w")
  f.write(read_temp() + "\n" + leds)
  f.close()

def read_temp():
  with open("tempurature.txt", "r") as f:
    return f.readlines()[0].strip()

def read_mode():
  with open("tempurature.txt", "r") as f:
    return f.readlines()[1].strip()

def read_LEDs():
  with open("tempurature.txt", "r") as f:
    return f.readlines()[2].strip()

index = get_html_file("index.html")

app = Flask(__name__)

@app.route("/")
def hello_world():
    return get_html_file("index.html")

@app.route("/tempurature")
def get_temp():
  return read_temp()

@app.route("/get_values")
def get_values():
  return {"temp": read_temp(), "mode": read_mode(), "leds": read_LEDs()}

@app.get("/send_temp")
def store_temp():
  temp = request.args.get('temp', "None")
  mode = request.args.get('mode', "None")
  leds = request.args.get('LEDs', "None")
  write_temp(temp + "\n" + mode + "\n" + leds + "\n")
  return ""

@app.post("/change_mode")
def store_mode():
  mode = request.args.get('mode', "None")
  write_mode(mode)
  return ""

@app.post("/change_led")
def change_led():
  mode = request.args.get('mode', "None")
  change = request.args.get('change', "None")
  leds = read_LEDs()
  if leds[change] == "0":
    leds = leds[:change] + "1" + leds[change+1:]
  else:
    leds[change] = "0"
  write_leds(mode + "\n" + leds + "\n")
  return ""

app.run(host="0.0.0.0")
