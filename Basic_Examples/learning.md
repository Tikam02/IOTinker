# ESP32 Arduino Learning Guide - Basic to Intermediate

## Prerequisites & Setup

### 1. Install Arduino IDE
- Download Arduino IDE 2.x from arduino.cc
- Install the ESP32 board package:
  - Go to File → Preferences
  - Add this URL to "Additional Board Manager URLs": 
    `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
  - Go to Tools → Board → Board Manager
  - Search "ESP32" and install "esp32 by Espressif Systems"

### 2. Hardware Setup
- **ESP32 WROOM-32** (you have this!)
- USB cable (usually micro-USB or USB-C)
- Breadboard and jumper wires
- LEDs, resistors (220Ω), buttons, sensors (optional for later projects)

### 3. Board Configuration
- Select: Tools → Board → ESP32 Arduino → "ESP32 Dev Module"
- Select your COM port: Tools → Port

---

## Phase 1: Basic Programming (Week 1-2)

### Example 1: Blink LED (Built-in)
```cpp
// Basic blink using built-in LED
// Most ESP32 boards have built-in LED on GPIO2
const int ledPin = 2;  // Define LED pin manually

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  delay(1000);
}
```

**Alternative version if GPIO2 doesn't work:**
```cpp
// If GPIO2 doesn't work, try these common LED pins:
// GPIO2, GPIO5, GPIO18, GPIO19, or GPIO21
const int ledPin = 5;  // Try different pins if needed

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  delay(1000);
}
```

### Example 2: Serial Communication
```cpp
void setup() {
  Serial.begin(115200);  // ESP32 works well at high baud rates
  delay(1000);
  Serial.println("ESP32 Started!");
}

void loop() {
  Serial.print("Uptime: ");
  Serial.print(millis());
  Serial.println(" ms");
  delay(2000);
}
```

### Example 3: Digital Input/Output
```cpp
// Connect LED to GPIO18, button to GPIO19 (with pull-up)
const int ledPin = 18;
const int buttonPin = 19;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Internal pull-up resistor
}

void loop() {
  int buttonState = digitalRead(buttonPin);
  
  if (buttonState == LOW) {  // Button pressed (pulled to ground)
    digitalWrite(ledPin, HIGH);
    Serial.println("Button pressed - LED ON");
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("Button released - LED OFF");
  }
  
  delay(100);
}
```

### Example 4: Analog Reading
```cpp
// Read analog value from a potentiometer on GPIO36
const int potPin = 36;  // ADC1_CH0
const int ledPin = 18;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  int sensorValue = analogRead(potPin);  // 0-4095 (12-bit ADC)
  int ledBrightness = map(sensorValue, 0, 4095, 0, 255);
  
  analogWrite(ledPin, ledBrightness);  // PWM output
  
  Serial.print("Sensor: ");
  Serial.print(sensorValue);
  Serial.print(" | LED: ");
  Serial.println(ledBrightness);
  
  delay(100);
}
```

---

## Phase 2: WiFi and Communication (Week 3-4)

### Example 5: WiFi Connection
```cpp
#include <WiFi.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
  } else {
    Serial.println("WiFi disconnected");
  }
  delay(5000);
}
```

### Example 6: Simple Web Server
```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);
const int ledPin = 18;

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP32 Web Server</h1>";
  html += "<p><a href='/led/on'>Turn LED ON</a></p>";
  html += "<p><a href='/led/off'>Turn LED OFF</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleLedOn() {
  digitalWrite(ledPin, HIGH);
  server.send(200, "text/plain", "LED is ON");
}

void handleLedOff() {
  digitalWrite(ledPin, LOW);
  server.send(200, "text/plain", "LED is OFF");
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.print("Connected! Visit: http://");
  Serial.println(WiFi.localIP());
  
  server.on("/", handleRoot);
  server.on("/led/on", handleLedOn);
  server.on("/led/off", handleLedOff);
  
  server.begin();
}

void loop() {
  server.handleClient();
}
```

### Example 7: HTTP Client (Getting Data)
```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://worldtimeapi.org/api/timezone/UTC");
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("Response:");
      Serial.println(payload);
      
      // Parse JSON (install ArduinoJson library)
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      
      const char* datetime = doc["datetime"];
      Serial.print("Current UTC time: ");
      Serial.println(datetime);
    }
    
    http.end();
  }
  
  delay(10000);  // Request every 10 seconds
}
```

---

## Phase 3: Sensors and Advanced Features (Week 5-6)

### Example 8: Temperature Sensor (DHT22)
```cpp
#include <DHT.h>

#define DHT_PIN 4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("DHT22 sensor initialized");
}

void loop() {
  delay(2000);  // DHT22 needs time between readings
  
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%  Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
}
```

### Example 9: OLED Display
```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 OLED Test");
  display.display();
}

void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("ESP32 Running");
  display.print("Uptime: ");
  display.print(millis() / 1000);
  display.println(" sec");
  display.display();
  delay(1000);
}
```

### Example 10: Bluetooth Classic
```cpp
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;
const int ledPin = 18;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  
  SerialBT.begin("ESP32-BT"); // Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}

void loop() {
  if (SerialBT.available()) {
    String command = SerialBT.readString();
    command.trim();
    
    Serial.print("Received: ");
    Serial.println(command);
    
    if (command == "ON") {
      digitalWrite(ledPin, HIGH);
      SerialBT.println("LED turned ON");
    } else if (command == "OFF") {
      digitalWrite(ledPin, LOW);
      SerialBT.println("LED turned OFF");
    } else {
      SerialBT.println("Unknown command. Use ON or OFF");
    }
  }
  
  delay(20);
}
```

---

## Phase 4: Intermediate Projects (Week 7-8)

### Example 11: WiFi Weather Station
```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey = "YOUR_OPENWEATHER_API_KEY";
const char* city = "London";

#define DHT_PIN 4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void getWeatherData() {
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "&appid=" + String(apiKey) + "&units=metric";
  
  http.begin(url);
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    float temp = doc["main"]["temp"];
    int humidity = doc["main"]["humidity"];
    const char* description = doc["weather"][0]["description"];
    
    Serial.println("=== Weather Data ===");
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Description: ");
    Serial.println(description);
  }
  
  http.end();
}

void getLocalSensorData() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  if (!isnan(temp) && !isnan(humidity)) {
    Serial.println("=== Local Sensor ===");
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(humidity);
  }
}

void loop() {
  getWeatherData();
  getLocalSensorData();
  
  Serial.println("===================");
  delay(60000);  // Update every minute
}
```

### Example 12: IoT Data Logger
```cpp
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

struct SensorData {
  unsigned long timestamp;
  float temperature;
  float humidity;
  int lightLevel;
};

SensorData readings[100];  // Store last 100 readings
int readingIndex = 0;

void setup() {
  Serial.begin(115200);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  setupWebServer();
  Serial.print("Web server started: http://");
  Serial.println(WiFi.localIP());
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/api/readings", handleApiReadings);
  server.begin();
}

void handleRoot() {
  String html = R"(
    <!DOCTYPE html>
    <html>
    <head><title>ESP32 Data Logger</title></head>
    <body>
      <h1>ESP32 Sensor Data Logger</h1>
      <div id="data"></div>
      <script>
        function updateData() {
          fetch('/api/readings')
            .then(response => response.json())
            .then(data => {
              document.getElementById('data').innerHTML = 
                '<h2>Latest Reading:</h2>' +
                '<p>Temperature: ' + data.temperature + '°C</p>' +
                '<p>Humidity: ' + data.humidity + '%</p>' +
                '<p>Light: ' + data.light + '</p>';
            });
        }
        setInterval(updateData, 5000);
        updateData();
      </script>
    </body>
    </html>
  )";
  
  server.send(200, "text/html", html);
}

void handleApiReadings() {
  DynamicJsonDocument doc(1024);
  
  if (readingIndex > 0) {
    int latest = (readingIndex - 1) % 100;
    doc["temperature"] = readings[latest].temperature;
    doc["humidity"] = readings[latest].humidity;
    doc["light"] = readings[latest].lightLevel;
    doc["timestamp"] = readings[latest].timestamp;
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void collectSensorData() {
  // Simulate sensor readings (replace with actual sensors)
  SensorData newReading;
  newReading.timestamp = millis();
  newReading.temperature = random(200, 300) / 10.0;  // 20-30°C
  newReading.humidity = random(300, 800) / 10.0;     // 30-80%
  newReading.lightLevel = analogRead(36);             // Light sensor on GPIO36
  
  readings[readingIndex % 100] = newReading;
  readingIndex++;
  
  Serial.print("Data collected: T=");
  Serial.print(newReading.temperature);
  Serial.print("°C, H=");
  Serial.print(newReading.humidity);
  Serial.print("%, L=");
  Serial.println(newReading.lightLevel);
}

void loop() {
  server.handleClient();
  
  static unsigned long lastReading = 0;
  if (millis() - lastReading > 10000) {  // Collect data every 10 seconds
    collectSensorData();
    lastReading = millis();
  }
}
```

---

## Learning Path & Next Steps

### Week 1-2: Master the Basics
- Get comfortable with digital I/O, analog reading, serial communication
- Understand ESP32 pin mapping and capabilities
- Practice with delays, timing, and basic control structures

### Week 3-4: Connectivity
- WiFi connection and web servers
- HTTP requests and JSON parsing
- Bluetooth communication

### Week 5-6: Sensors & Peripherals
- Interface with various sensors (temperature, humidity, light)
- Display data on OLED/LCD screens
- Store and retrieve data

### Week 7-8: Integration Projects
- Combine multiple concepts into practical projects
- Create web interfaces for your devices
- Implement data logging and visualization

### Advanced Topics to Explore Next:
- **FreeRTOS Tasks**: Multi-threading on ESP32
- **Deep Sleep**: Power management for battery projects
- **OTA Updates**: Update firmware over WiFi
- **MQTT**: IoT messaging protocol
- **BLE**: Bluetooth Low Energy
- **Touch Sensors**: Capacitive touch capabilities
- **Camera Module**: ESP32-CAM integration
- **Security**: WiFi encryption, secure communication

### Required Libraries:
Install these through Arduino IDE Library Manager:
- DHT sensor library (for temperature/humidity)
- Adafruit SSD1306 & Adafruit GFX (for OLED)
- ArduinoJson (for JSON parsing)
- PubSubClient (for MQTT, future projects)

### Hardware Shopping List for Extended Learning:
- DHT22 temperature/humidity sensor
- 0.96" OLED display (SSD1306)
- Photoresistor (light sensor)
- Push buttons and LEDs
- Breadboard and jumper wires
- 10kΩ and 220Ω resistors

Start with Example 1 and work your way through each example. Each builds on previous concepts while introducing new capabilities. Test each example thoroughly before moving to the next!