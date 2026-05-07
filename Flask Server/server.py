from flask import Flask
from flask import request

def get_html_file(filename):
  with open(filename, "r") as f:
    return f.read()

def write_temp(temp):
  f = open("data.txt", "w")
  f.write(temp)
  f.close()

def reset_change():
  f = open("change.txt", "w+")
  f.write("0\n000000\n")
  f.close()

def change_mode():
  leds = read_leds_change()
  f = open("change.txt", "w")
  f.write("1\n" + leds + "\n")
  f.close()

def change_leds(i):
  store = read_mode_change() + "\n" + read_leds_change()[:i] + "1" + read_leds_change()[i+1:] + "\n"
  f = open("change.txt", "w")
  f.write(store)
  f.close()

def read_mode_change():
  with open("change.txt", "r") as f:
    return f.readlines()[0].strip()

def read_leds_change():
  with open("change.txt", "r") as f:
   return f.readlines()[1].strip()

def read_temp():
  with open("data.txt", "r") as f:
    return f.readlines()[0].strip()

def read_mode():
  with open("data.txt", "r") as f:
    return f.readlines()[1].strip()

def read_LEDs():
  with open("data.txt", "r") as f:
    return f.readlines()[2].strip()

index = get_html_file("index.html")

app = Flask(__name__)

# display webpage
@app.route("/")
def hello_world():
    return get_html_file("index.html")

# provide ESP hardware values for webpage
@app.route("/get_values")
def get_values():
  return {"temp": read_temp(), "mode": read_mode(), "leds": read_LEDs()}

# revice data values from ESP
@app.get("/send_temp")
def store_temp():
  temp = request.args.get('temp', "None")
  mode = request.args.get('mode', "None")
  leds = request.args.get('LEDs', "None")
  write_temp(temp + "\n" + mode + "\n" + leds + "\n")
  r = read_mode_change() + read_leds_change()
  reset_change()
  return r

@app.get("/change_mode")
def store_mode():
  change_mode()
  return ""

@app.get("/change_led")
def change_led():
  change = int(request.args.get('change', "None"))
  change_leds(change)
  return ""

app.run(host="0.0.0.0")
