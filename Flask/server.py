from flask import Flask
from flask_socketio import SocketIO, send, emit, join_room, leave_room
from flask import request
from flask import make_response

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
#app.config['SECRET_KEY'] = 'your-secret-key-here'
socketio = SocketIO(app)


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
  resp = make_response("OK", 200)
  resp.headers['Content-Type'] = "text/plain"
  resp.headers['Connection'] = "keep-alive"
  resp.headers['Keep-Alive'] = "timeout=60, max=1000"
  return resp

# Handle new user joining
@socketio.on('arduino')
def handle_join():
    print(f'Client connected: {request.sid}')
    emit("hello", {data: "hello"})

# Handle user messages
@socketio.on('message')
def handle_message(data):
    try:
        message = data.get('message', '').strip()
        if not message:
            emit('my response', {'data': 'Message cannot be empty'})
            return
        print(f'Received message: {message}')
        # Echo the message back to the sender
        emit('my response', {'data': f'Echo: {message}'})
    except Exception as e:
        print(f'Error processing message: {e}')
        emit('my response', {'data': 'Error processing your message'})

# Handle disconnects
@socketio.on('disconnect')
def handle_disconnect():
    print(f'Client disconnected: {request.sid}')

@app.post("/change_mode")
def store_mode():
  write_mode()
  return ""

@app.get("/get_data")
def change_mode():
  return

socketio.run(app, host="0.0.0.0", debug=True)
