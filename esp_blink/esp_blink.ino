// blink for ESP8266

// 4 pin = GPIO4
// please connect LED to GPIO4

void setup() {
  pinMode(4, OUTPUT);
  Serial.begin(115200);
  Serial.println("ESP start......");
  delay(1000);
}
 
void loop() {
  digitalWrite(4, HIGH);
  delay(500);
  digitalWrite(4, LOW);
  delay(500);
  Serial.println("blink");
}

