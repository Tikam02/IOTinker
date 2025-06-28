void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Started!");
}


void loop(){
  Serial.print("Uptime");
  Serial.print(millis());
  Serial.println("ms");
  delay(2000);
}