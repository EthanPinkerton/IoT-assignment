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
int LED_states[] = {0,0,0,0,0,0};
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

void blink(int pin) {
  if (blinkMode == 0) {
    digitalWrite(LEDs[pin], HIGH);
    blinkMode = 1;
  } else {
    digitalWrite(LEDs[pin], LOW);
    blinkMode = 0;
  }
}

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

void web_control() {

}

void clearLEDs() {
  for (int LED : LEDs) {
    digitalWrite(LED, LOW);
  }
}

void changeMode() {
  clearLEDs();
  
  if (mode == 0) {
    display_binary_temp();
  } else if (mode == 1){
    bounce_pos = 0;
  } else if (mode == 2){
    LEDstates = {0,0,0,0,0,0};
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

String readLEDstates() {
  String state = "";
  for (int LED : LEDs) {
    state += digitalRead(LED);
  }
  return state;
}

void send_temp() {
  // Construct request URL
  if (connectServer()) {
    String url = "/send_temp?temp=";
    url += readTemperature();
    url += "&mode=";
    url += mode;
    url += "&LEDs=";
    url += readLEDstates();
    
    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    // Send HTTP GET request
    client.print(String("GET ") + url + " HTTP/1.1\n" +
               "Host: " + serverHost + "\n" +
               "Connection: close\n\n");
    
    client.stop();
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

// Modes: 0 -> binary temp display, 1 -> LED Chase (bounce), 2 -> web interface controlled
void conduct_current_mode(){
  if (mode == 0) {
    if (timer%50 == 0) {
      display_binary_temp();
    }
  } else if (mode == 1){
    if (timer%25 == 0) {
      bounce();
    }
  } else if (mode == 2){
    web_control();
  }
}

void loop() {
  int time = millis();
  if (digitalRead(button) == LOW && buttonState == 0) {
    mode++;
    mode = mode%2;
    changeMode();
    buttonState = 1;
  } else if (digitalRead(button) == HIGH && buttonState == 1) {
    buttonState = 0;
  }
  
  conduct_current_mode();
  
  if (timer >= 200) {
    send_temp();
    timer = 0;
  }
  
  timer++;
  
  if (millis() - time < 10) {
    delay(millis() - time);
  } else {
    timer += (millis() - time)/10;
  }
}