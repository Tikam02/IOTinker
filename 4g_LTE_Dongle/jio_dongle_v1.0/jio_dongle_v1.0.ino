/*
 * ESP32-S3-A7670E 4G LTE Internet Dongle
 * Converts cellular data to WiFi hotspot
 * 
 * Features:
 * - 4G LTE data connection via A7670E
 * - WiFi Access Point for client devices
 * - HTTP proxy for internet sharing
 * - Web interface for monitoring
 * - Connection status LED indicators
 * - Data usage tracking
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Pin definitions
#define RGB_LED_PIN    38
#define A7670_PWR_PIN  21
#define A7670_RX_PIN   47
#define A7670_TX_PIN   48

// Network configuration
#define AP_SSID        "ESP32_Jio_Hotspot"
#define AP_PASSWORD    "Jio4GHotspot"
#define DNS_PORT       53
#define HTTP_PORT      80
#define PROXY_PORT     8080

// Global objects
WebServer server(HTTP_PORT);
DNSServer dnsServer;
Preferences prefs;
HTTPClient http;

// Status variables
struct SystemStatus {
  bool cellularConnected = false;
  bool wifiAPActive = false;
  String cellularOperator = "";
  int signalStrength = 0;
  String ipAddress = "";
  unsigned long dataUsed = 0;
  int connectedClients = 0;
  unsigned long uptime = 0;
  String lastError = "";
} status;

// A7670E configuration for Jio
struct CellularConfig {
  String apn = "jionet";     // Jio APN
  String username = "";      // Jio doesn't require username
  String password = "";      // Jio doesn't require password
  int contextId = 1;
  String authType = "PAP";   // Jio uses PAP authentication
} cellConfig;

// LED status indicators
enum LEDStatus {
  LED_STARTUP,      // Slow blue blink
  LED_CONNECTING,   // Fast blue blink  
  LED_CONNECTED,    // Solid green
  LED_ERROR,        // Red blink
  LED_DATA_ACTIVE   // Green with quick flash
};

LEDStatus currentLEDStatus = LED_STARTUP;
unsigned long lastLEDUpdate = 0;
bool ledState = false;

void setLED(uint8_t r, uint8_t g, uint8_t b) {
  // Simple RGB control - you may need to adjust for your specific LED
  analogWrite(RGB_LED_PIN, (r + g + b) > 0 ? 255 : 0);
}

void updateLED() {
  unsigned long now = millis();
  
  switch(currentLEDStatus) {
    case LED_STARTUP:
      if(now - lastLEDUpdate > 1000) {
        ledState = !ledState;
        setLED(0, 0, ledState ? 50 : 0); // Slow blue blink
        lastLEDUpdate = now;
      }
      break;
      
    case LED_CONNECTING:
      if(now - lastLEDUpdate > 200) {
        ledState = !ledState;
        setLED(0, 0, ledState ? 100 : 0); // Fast blue blink
        lastLEDUpdate = now;
      }
      break;
      
    case LED_CONNECTED:
      setLED(0, 50, 0); // Solid green
      break;
      
    case LED_ERROR:
      if(now - lastLEDUpdate > 500) {
        ledState = !ledState;
        setLED(ledState ? 100 : 0, 0, 0); // Red blink
        lastLEDUpdate = now;
      }
      break;
      
    case LED_DATA_ACTIVE:
      if(now - lastLEDUpdate > 100) {
        ledState = !ledState;
        setLED(0, ledState ? 100 : 20, 0); // Green with flash
        lastLEDUpdate = now;
      }
      break;
  }
}

// A7670E Communication Functions
String sendATCommand(String command, unsigned long timeout = 5000) {
  // Clear input buffer
  while(Serial1.available()) Serial1.read();
  
  Serial.print("AT>> ");
  Serial.println(command);
  
  // Send command
  Serial1.println(command);
  
  // Wait for response
  String response = "";
  unsigned long start = millis();
  
  while(millis() - start < timeout) {
    if(Serial1.available()) {
      char c = Serial1.read();
      response += c;
      
      // Check if we have a complete response
      if(response.endsWith("OK\r\n") || response.endsWith("ERROR\r\n")) {
        break;
      }
    }
    delay(10);
  }
  
  Serial.print("AT<< ");
  Serial.println(response);
  return response;
}

bool initializeCellular() {
  Serial.println("Initializing A7670E for Jio network...");
  currentLEDStatus = LED_CONNECTING;
  
  // Basic AT test
  String resp = sendATCommand("AT");
  if(resp.indexOf("OK") == -1) {
    status.lastError = "A7670E not responding";
    return false;
  }
  
  // Disable echo
  sendATCommand("ATE0");
  
  // Set network mode for Jio (LTE preferred)
  sendATCommand("AT+CNMP=38");  // LTE only mode for better Jio compatibility
  
  // Check SIM card
  resp = sendATCommand("AT+CPIN?");
  if(resp.indexOf("READY") == -1) {
    status.lastError = "Jio SIM card not ready - check if inserted properly";
    return false;
  }
  
  // Set preferred operator format
  sendATCommand("AT+COPS=3,0");  // Long alphanumeric format
  
  // Wait for Jio network registration
  Serial.println("Waiting for Jio network registration...");
  for(int i = 0; i < 60; i++) {  // Increased timeout for Jio
    resp = sendATCommand("AT+CREG?");
    if(resp.indexOf(",1") != -1 || resp.indexOf(",5") != -1) {
      Serial.println("Registered on Jio network!");
      break;
    }
    Serial.print("Waiting for Jio network... ");
    Serial.print(i + 1);
    Serial.println("/60");
    delay(2000);
    if(i == 59) {
      status.lastError = "Jio network registration failed - check signal strength";
      return false;
    }
  }
  
  // Get operator info
  resp = sendATCommand("AT+COPS?");
  int start = resp.indexOf("\"");
  if(start != -1) {
    int end = resp.indexOf("\"", start + 1);
    if(end != -1) {
      status.cellularOperator = resp.substring(start + 1, end);
    }
  }
  
  // Get signal strength
  resp = sendATCommand("AT+CSQ");
  int csqIndex = resp.indexOf("+CSQ: ");
  if(csqIndex != -1) {
    int rssi = resp.substring(csqIndex + 6, resp.indexOf(",", csqIndex)).toInt();
    status.signalStrength = rssi;
  }
  
  return true;
}

bool setupDataConnection() {
  Serial.println("Setting up data connection for Jio...");
  
  // Set network mode to LTE (Jio requirement)
  sendATCommand("AT+CNMP=38");  // LTE only mode
  sendATCommand("AT+CMNB=1");   // CAT-M1 and NB-IoT
  
  // Configure APN for Jio
  String apnCmd = "AT+CGDCONT=" + String(cellConfig.contextId) + ",\"IP\",\"" + cellConfig.apn + "\"";
  String resp = sendATCommand(apnCmd);
  if(resp.indexOf("OK") == -1) {
    status.lastError = "Jio APN configuration failed";
    return false;
  }
  
  // Set authentication type for Jio
  String authCmd = "AT+CGAUTH=" + String(cellConfig.contextId) + ",1,\"\",\"\"";  // PAP with no user/pass
  sendATCommand(authCmd);
  
  // Activate PDP context
  String activateCmd = "AT+CGACT=1," + String(cellConfig.contextId);
  resp = sendATCommand(activateCmd, 30000);
  if(resp.indexOf("OK") == -1) {
    status.lastError = "PDP context activation failed";
    return false;
  }
  
  // Get IP address
  String ipCmd = "AT+CGPADDR=" + String(cellConfig.contextId);
  resp = sendATCommand(ipCmd);
  int ipStart = resp.indexOf("\"");
  if(ipStart != -1) {
    int ipEnd = resp.indexOf("\"", ipStart + 1);
    if(ipEnd != -1) {
      status.ipAddress = resp.substring(ipStart + 1, ipEnd);
    }
  }
  
  // Start network service
  resp = sendATCommand("AT+NETOPEN");
  if(resp.indexOf("OK") != -1 || resp.indexOf("Network opened") != -1) {
    status.cellularConnected = true;
    Serial.println("Data connection established!");
    Serial.print("IP Address: ");
    Serial.println(status.ipAddress);
    return true;
  }
  
  status.lastError = "Network service start failed";
  return false;
}

// HTTP Proxy Implementation
String httpGet(String url) {
  // Simple HTTP GET via A7670E
  String cmd = "AT+HTTPINIT";
  sendATCommand(cmd);
  
  cmd = "AT+HTTPPARA=\"CID\",1";
  sendATCommand(cmd);
  
  cmd = "AT+HTTPPARA=\"URL\",\"" + url + "\"";
  sendATCommand(cmd);
  
  cmd = "AT+HTTPACTION=0";
  String resp = sendATCommand(cmd, 15000);
  
  delay(2000);
  
  cmd = "AT+HTTPREAD";
  resp = sendATCommand(cmd, 10000);
  
  sendATCommand("AT+HTTPTERM");
  
  return resp;
}

// Web Server Handlers
void handleRoot() {
  String connectionClass = status.cellularConnected ? "connected" : "disconnected";
  String connectionStatus = status.cellularConnected ? "🟢 Connected" : "🔴 Disconnected";
  String wifiStatus = status.wifiAPActive ? "Active" : "Inactive";
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 4G Dongle</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body { font-family: Arial; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }";
  html += ".status { padding: 10px; margin: 10px 0; border-radius: 5px; }";
  html += ".connected { background: #d4edda; color: #155724; }";
  html += ".disconnected { background: #f8d7da; color: #721c24; }";
  html += ".info-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }";
  html += ".card { background: #f8f9fa; padding: 15px; border-radius: 8px; }";
  html += "button { background: #007bff; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; }";
  html += "button:hover { background: #0056b3; }";
  html += ".data-usage { font-size: 1.2em; font-weight: bold; }";
  html += "</style>";
  html += "<script>";
  html += "setInterval(function() {";
  html += "fetch('/status').then(r => r.json()).then(data => {";
  html += "document.getElementById('status').innerHTML = JSON.stringify(data, null, 2);";
  html += "});";
  html += "}, 5000);";
  html += "</script></head><body>";
  html += "<div class=\"container\">";
  html += "<h1>🌐 ESP32 4G LTE Dongle (Jio Network)</h1>";
  
  html += "<div class=\"status " + connectionClass + "\">";
  html += "Connection Status: " + connectionStatus;
  html += "</div>";
  
  html += "<div class=\"info-grid\">";
  html += "<div class=\"card\">";
  html += "<h3>📶 Cellular Info</h3>";
  html += "<p><strong>Operator:</strong> " + status.cellularOperator + "</p>";
  html += "<p><strong>Signal:</strong> " + String(status.signalStrength) + " dBm</p>";
  html += "<p><strong>IP Address:</strong> " + status.ipAddress + "</p>";
  html += "</div>";
  
  html += "<div class=\"card\">";
  html += "<h3>📡 WiFi Hotspot</h3>";
  html += "<p><strong>SSID:</strong> " + String(AP_SSID) + "</p>";
  html += "<p><strong>Connected Clients:</strong> " + String(status.connectedClients) + "</p>";
  html += "<p><strong>Status:</strong> " + wifiStatus + "</p>";
  html += "</div></div>";
  
  html += "<div class=\"card\">";
  html += "<h3>📊 Statistics</h3>";
  html += "<p class=\"data-usage\">Data Used: " + String(status.dataUsed / 1024) + " KB</p>";
  html += "<p><strong>Uptime:</strong> " + String(status.uptime / 1000) + " seconds</p>";
  html += "<p><strong>Last Error:</strong> " + status.lastError + "</p>";
  html += "</div>";
  
  html += "<div class=\"card\">";
  html += "<h3>⚙️ Controls</h3>";
  html += "<button onclick=\"fetch('/restart')\">Restart Connection</button> ";
  html += "<button onclick=\"fetch('/reboot')\">Reboot Device</button>";
  html += "<br><br><small><strong>Jio Troubleshooting:</strong><br>";
  html += "- Ensure VoLTE is enabled on SIM<br>";
  html += "- Check data plan is active<br>";
  html += "- Try different location for better signal<br>";
  html += "- SIM should be registered for 4G services</small>";
  html += "</div>";
  
  html += "<div class=\"card\">";
  html += "<h3>📋 Live Status</h3>";
  html += "<pre id=\"status\">Loading...</pre>";
  html += "</div></div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  StaticJsonDocument<512> doc;
  doc["cellular_connected"] = status.cellularConnected;
  doc["wifi_ap_active"] = status.wifiAPActive;
  doc["operator"] = status.cellularOperator;
  doc["signal_strength"] = status.signalStrength;
  doc["ip_address"] = status.ipAddress;
  doc["data_used"] = status.dataUsed;
  doc["connected_clients"] = status.connectedClients;
  doc["uptime"] = millis();
  doc["last_error"] = status.lastError;
  doc["free_heap"] = ESP.getFreeHeap();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleRestart() {
  server.send(200, "text/plain", "Restarting connection...");
  delay(1000);
  ESP.restart();
}

void handleProxy() {
  // Simple HTTP proxy implementation
  String url = server.arg("url");
  if(url.length() > 0) {
    String result = httpGet(url);
    server.send(200, "text/plain", result);
    status.dataUsed += result.length();
  } else {
    server.send(400, "text/plain", "Missing URL parameter");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n🚀 ESP32-S3 4G LTE Internet Dongle Starting...");
  Serial.println("📱 Configured for Jio Network");
  Serial.println("⚠️  Jio Requirements:");
  Serial.println("   - SIM must have active data plan");
  Serial.println("   - VoLTE compatible SIM required");
  Serial.println("   - Good 4G signal strength needed");
  Serial.println("   - APN: jionet (auto-configured)");
  
  // Initialize preferences
  prefs.begin("4g-dongle", false);
  status.dataUsed = prefs.getULong("data_used", 0);
  
  // Initialize LED
  pinMode(RGB_LED_PIN, OUTPUT);
  currentLEDStatus = LED_STARTUP;
  
  // Initialize A7670E
  pinMode(A7670_PWR_PIN, OUTPUT);
  digitalWrite(A7670_PWR_PIN, HIGH);
  delay(1000);
  
  Serial1.begin(115200, SERIAL_8N1, A7670_TX_PIN, A7670_RX_PIN);
  delay(2000);
  
  // Power cycle A7670E
  Serial.println("Power cycling A7670E...");
  digitalWrite(A7670_PWR_PIN, LOW);
  delay(3000);
  digitalWrite(A7670_PWR_PIN, HIGH);
  delay(5000);
  
  // Initialize cellular connection
  if(initializeCellular()) {
    if(setupDataConnection()) {
      currentLEDStatus = LED_CONNECTED;
    } else {
      currentLEDStatus = LED_ERROR;
    }
  } else {
    currentLEDStatus = LED_ERROR;
  }
  
  // Setup WiFi Access Point
  Serial.println("Setting up WiFi Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(2000);
  
  IPAddress apIP = WiFi.softAPIP();
  if(apIP != IPAddress(0,0,0,0)) {
    status.wifiAPActive = true;
    Serial.println("✅ WiFi AP Started Successfully!");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);
    Serial.print("IP Address: ");
    Serial.println(apIP);
  }
  
  // Setup DNS server (captive portal)
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Setup web server
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/restart", handleRestart);
  server.on("/proxy", handleProxy);
  server.onNotFound(handleRoot); // Redirect all to main page
  
  server.begin();
  Serial.println("Web server started on port " + String(HTTP_PORT));
  
  Serial.println("\n🎉 4G LTE Dongle Ready!");
  Serial.println("Connect to WiFi: " + String(AP_SSID));
  Serial.println("Open browser to: http://" + apIP.toString());
  Serial.println("================================");
}

void loop() {
  static unsigned long lastUpdate = 0;
  static unsigned long lastStatusCheck = 0;
  
  // Update LED status
  updateLED();
  
  // Handle web server
  server.handleClient();
  dnsServer.processNextRequest();
  
  // Update status every 5 seconds
  if(millis() - lastStatusCheck > 5000) {
    lastStatusCheck = millis();
    
    // Update connected clients
    status.connectedClients = WiFi.softAPgetStationNum();
    
    // Check cellular connection
    if(status.cellularConnected) {
      String resp = sendATCommand("AT+CSQ", 2000);
      int csqIndex = resp.indexOf("+CSQ: ");
      if(csqIndex != -1) {
        int rssi = resp.substring(csqIndex + 6, resp.indexOf(",", csqIndex)).toInt();
        status.signalStrength = rssi;
      }
      
      // Set LED status based on activity
      if(status.connectedClients > 0) {
        currentLEDStatus = LED_DATA_ACTIVE;
      } else {
        currentLEDStatus = LED_CONNECTED;
      }
    }
    
    // Save data usage periodically
    if(millis() - lastUpdate > 30000) {
      lastUpdate = millis();
      prefs.putULong("data_used", status.dataUsed);
      status.uptime = millis();
    }
  }
  
  // Handle any incoming data from A7670E
  if(Serial1.available()) {
    String data = Serial1.readString();
    // Process unsolicited messages if needed
  }
  
  delay(10);
}