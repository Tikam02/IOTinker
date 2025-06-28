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