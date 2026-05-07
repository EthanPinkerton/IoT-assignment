# Arduino ESP32 Refactor: Stable WebSocket Connection

```cpp
#include <Arduino.h>
#include <bitset>
#include <cmath>
#include <string>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* ssid = "Xperia_6145";
const char* password = "12345678321";

const char* serverHost = "10.49.108.58";
const uint16_t serverPort = 5000;
const char* websocketPath = "/";

WebSocketsClient webSocket;

// LEDs and modes
int LEDs[] = {6, 9, 10, 11, 12, 13};
int LEDstates[6] = {0, 0, 0, 0, 0, 0};
int button = 5;
int buttonState = 0;
int mode = 0;
int blinkMode = 0;
int bounce_pos = 0;
int bounce_dir = 1;
const int len_LEDs = sizeof(LEDs) / sizeof(LEDs[0]);

int timer = 0;

// Temperature monitor
int TEMP_PIN = 8;
const double ADC_MAX = 4095.0;
const double VREF = 3.3;
float temp;

float readTemperature() {
    int adcValue = analogRead(TEMP_PIN);
    float voltage = adcValue * VREF / ADC_MAX;
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

void bounce() {
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
    std::string binary = std::bitset<6>(t).to_string();

    for (int i = 0; i < len_LEDs; i++) {
        digitalWrite(LEDs[i], binary[i] == '1' ? HIGH : LOW);
    }
}

void web_control() {
    for (int i = 0; i < len_LEDs; i++) {
        digitalWrite(LEDs[i], LEDstates[i] == 1 ? HIGH : LOW);
    }
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
    } else if (mode == 1) {
        bounce_pos = 0;
    }
}

String readLEDstates() {
    String state = "";
    for (int LED : LEDs) {
        state += digitalRead(LED);
    }
    return state;
}

void sendDeviceData() {
    StaticJsonDocument<200> doc;
    doc["temp"] = readTemperature();
    doc["mode"] = mode;
    doc["LEDs"] = readLEDstates();

    String payload;
    serializeJson(doc, payload);

    webSocket.sendTXT(payload);

    Serial.print("Sent: ");
    Serial.println(payload);
}

void processWebSocketMessage(uint8_t* payload, size_t length) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
        Serial.println("Invalid JSON received");
        return;
    }

    if (doc.containsKey("LEDs")) {
        JsonArray leds = doc["LEDs"].as<JsonArray>();

        for (int i = 0; i < len_LEDs && i < leds.size(); i++) {
            LEDstates[i] = leds[i];
        }
    }
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("WebSocket Disconnected");
            break;

        case WStype_CONNECTED:
            Serial.println("WebSocket Connected");
            break;

        case WStype_TEXT:
            Serial.print("Received: ");
            Serial.println((char*)payload);
            processWebSocketMessage(payload, length);
            break;

        default:
            break;
    }
}

void connectWifi() {
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        blink(0);
    }

    clearLEDs();

    Serial.println("\nConnected! IP: ");
    Serial.println(WiFi.localIP());
}

void setupWebSocket() {
    webSocket.begin(serverHost, serverPort, websocketPath);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    webSocket.enableHeartbeat(15000, 3000, 2);
}

void button_press() {
    if (digitalRead(button) == LOW && buttonState == 0) {
        mode++;
        mode = mode % 3;
        changeMode();
        buttonState = 1;
    } else if (digitalRead(button) == HIGH && buttonState == 1) {
        buttonState = 0;
    }
}

void conduct_current_mode() {
    if (mode == 0) {
        if (timer % 500 == 0) {
            display_binary_temp();
        }
    } else if (mode == 1) {
        if (timer % 250 == 0) {
            bounce();
        }
    } else if (mode == 2) {
        web_control();
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
    setupWebSocket();
}

void loop() {
    webSocket.loop();

    button_press();
    conduct_current_mode();

    if (timer >= 1000) {
        if (WiFi.status() == WL_CONNECTED) {
            sendDeviceData();
        }
        timer = 0;
    }

    timer++;
    delay(1);
}
```

## Required Libraries

Install these from Arduino Library Manager:

* **WebSockets by Markus Sattler**
* **ArduinoJson by Benoit Blanchon**

## Benefits of WebSockets

* Persistent real-time connection
* Lower latency
* No repeated HTTP reconnections
* Instant server-to-device LED updates
* Automatic reconnect + heartbeat
* Better scalability for live dashboards

## Example Server Messages

### Device → Server:

```json
{"temp":24.3,"mode":2,"LEDs":"101010"}
```

### Server → Device:

```json
{"LEDs":[1,0,1,0,1,0]}
```
