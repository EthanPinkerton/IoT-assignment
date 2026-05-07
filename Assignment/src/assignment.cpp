#include <Arduino.h>
#include <bitset>
#include <cmath>
#include <string>
#include <WiFi.h>
#include <WiFiClientSecure.h>

const char* ssid = "Xperia_6145";
const char* password = "12345678321";

const char* serverHost = "10.49.108.58";
const int serverPort = 5000;

// LEDs and modes
int LEDs[] = {6,9,10,11,12,13};
int LEDstates[6];
int button = 5;
int buttonState = 0;
int mode = 0;
int blinkMode = 0;
int bounce_pos = 0;
int bounce_dir = 1;
const int len_LEDs = sizeof(LEDs) / sizeof(LEDs[0]);

//timer to update modes
int timer = 0;

//tempurature monitor
int TEMP_PIN = 8;
const double ADC_MAX = 4095.0;
const double VREF = 3.3;
float temp;

WiFiClient client;

float readTemperature() {
  // Read the analog value from the sensor pin
  int adcValue = analogRead(TEMP_PIN);
  // Convert ADC value to voltage
  // Formula: voltage = (ADC value / ADC max) * reference voltage
  float voltage = adcValue * VREF / ADC_MAX;
  // Convert voltage to temperature using TMP36 formula
  // TMP36 output = 500 mV offset + 10 mV per °C
  // Temperature (°C) = (Voltage - 0.5) * 100
  float tempC = (voltage - 0.5) * 100.0;
  
  return tempC;
}

void handle_temp() {
  temp = readTemperature();
}

// Standby mode
void blink(int pin) {
  if (blinkMode == 0) {
    digitalWrite(LEDs[pin], HIGH);
    blinkMode = 1;
  } else {
    digitalWrite(LEDs[pin], LOW);
    blinkMode = 0;
  }
}

// Mode 1 - LED Animation Pattern - 1 turned on LED bounces from one end to the other
void bounce(){
  digitalWrite(LEDs[bounce_pos], HIGH);
  digitalWrite(LEDs[bounce_pos - bounce_dir], LOW);
  
  if (bounce_pos == len_LEDs - 1) {
    bounce_dir = -1;
  } else if (bounce_pos == 0) {
    bounce_dir = 1;
  }
  bounce_pos += bounce_dir;
}

// Mode 0 - Binary Temperature Display - Turns a temp int into a binary String
// Each character of the String accounts for an LED being on or off
void display_binary_temp() {
  handle_temp();
  int t = std::round(temp);
  std::string binary = std::bitset<len_LEDs>(t).to_string();
  for (int i = 0; i < len_LEDs; i++) {
    if (binary[i] == '1') {
      digitalWrite(LEDs[i], HIGH);
    } else {
      digitalWrite(LEDs[i], LOW);
    }
  }
}

// Recieves custom LED state input from the web server
//int* get_web_inputs(){
//  return {0,0,0,0,0,0};
//}

// All LEDs are set to OFF
void clearLEDs() {
  for (int LED : LEDs) {
    digitalWrite(LED, LOW);
  }
}

// Set the initial state of the new mode
void changeMode() {
  clearLEDs();
  
  if (mode == 0) {
    display_binary_temp();
  } else if (mode == 1){
    bounce_pos = 0;
  } else if (mode == 2){
//    LEDstates = [0,0,0,0,0,0];
  }
}

void connectWifi() {
  WiFi.begin(ssid,password);
  
  Serial.print("Connecting to WiFi");
  
  while(WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    //blink led while waiting
    blink(0);
  }
  clearLEDs();
  
  Serial.println("");
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());
}

bool connectServer() {
  Serial.print("Connecting to server ");
  Serial.print(serverHost);
  Serial.print(":");
  Serial.println(serverPort);
  
  // Attempt TCP connection
  if (!client.connect(serverHost, serverPort)){
    Serial.println("Connection failed!");
    return false;
  }
  
  Serial.println("Connected to server!");
  return true;
}

// Produces and returns a String of current LED states
String readLEDstates() {
  String state = "";
  for (int LED : LEDs) {
    state += digitalRead(LED);
  }
  return state;
}

void increment_mode() {
  mode++;
  mode = mode%3;
  changeMode();
}

void flip_LEDs(std::string binary_flip_LED){
  for(int i = 1; i < binary_flip_LED.length(); i++){
    if (binary_flip_LED.at(i) == '1') {
      mode =  2;
      if (digitalRead(LEDs[i - 1]) == HIGH) {
        digitalWrite(LEDs[i - 1], LOW);
      } else if (digitalRead(LEDs[i - 1]) == LOW) {
        digitalWrite(LEDs[i - 1], HIGH);
      }
    }
  }
}

void send_temp() {
  connectServer();
  // Construct request URL
  if (client.connected()) {
    String url = "/send_temp?temp=";
    url += readTemperature();
    url += "&mode=";
    url += mode;
    url += "&LEDs=";
    url += readLEDstates();
    
    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    // Send HTTP GET request
    client.print(String("GET ") + url + " HTTP/1.1" + "\r\n" +
                "Host: " + serverHost + "\r\n" +
                "Connection: keep-alive\r\n\r\n");
  }
}

void get_response() {
  std::string last = "";
  while (client.available()) {
      char c = client.read();
      last += c;
  }
  if (last != "") {
    std::string response = last.substr(last.length() - 7, 7);
    Serial.println(response.c_str());
    if (response.at(0) == '1') {
      increment_mode();
    }
    flip_LEDs(response);
  }
}

void setup() {
  Serial.begin(115200);
  
  for (int LED : LEDs) {
    pinMode(LED, OUTPUT);
  }
  
  pinMode(button, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(TEMP_PIN, ADC_11db);
  
  connectWifi();
  
  
  while(!connectServer()) {
    blink(1);
    delay(500);
  }
  clearLEDs();
}

// Changes the mode on a single button press - toggle button functionality
void button_press(){
  if (digitalRead(button) == LOW && buttonState == 0) {
    mode++;
    mode = mode%3;
    changeMode();
    buttonState = 1;
  } else if (digitalRead(button) == HIGH && buttonState == 1) {
    buttonState = 0;
  }
}

// Calls the current mode function depending on the mode state
// Modes: 0 -> binary temp display, 1 -> LED Chase (bounce), 2 -> web interface controlled
void conduct_current_mode(){
  if (mode == 0) {
    if (timer%500 == 0) {
      display_binary_temp();
    }
  } else if (mode == 1){
    if (timer%250 == 0) {
      bounce();
    }
  }
}

// Main loop responsible for run time operations
void loop() {

  button_press();
  conduct_current_mode();
  
  if (timer == 500) {
    get_response();
  }
  if (timer >= 1000) {
    send_temp();
    timer = 0;
  }
  timer++;
  
  delay(1);
}
