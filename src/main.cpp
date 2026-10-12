#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SENSOR_PIN 34   // MQ-135 analog out
#define FAN_PIN 26      // filter fan relay
#define FAN_ON_AT 1500  // raw ADC threshold, calibrate for your sensor

const char *AP_SSID = "AirQuality";
const char *AP_PASS = "12345678";  // min 8 chars

LiquidCrystal_I2C lcd(0x27, 16, 2);  // try 0x3F if blank
WebServer server(80);

int air = 0;
bool fanOn = false;
bool autoMode = true;

void setFan(bool on) {
  fanOn = on;
  digitalWrite(FAN_PIN, on);
}

void handleApi() {
  if (server.hasArg("mode")) autoMode = server.arg("mode") == "auto";
  if (server.hasArg("fan")) { autoMode = false; setFan(server.arg("fan") == "1"); }
  server.send(200, "application/json",
              "{\"air\":" + String(air) + ",\"fan\":" + fanOn + ",\"auto\":" + autoMode + "}");
}

void setup() {
  Serial.begin(115200);
  pinMode(FAN_PIN, OUTPUT);
  setFan(false);

  lcd.init();
  lcd.backlight();
  lcd.print("Starting...");

  if (!LittleFS.begin(true)) Serial.println("LittleFS mount failed");

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println(WiFi.softAPIP());  // 192.168.4.1

  server.on("/api", handleApi);
  server.serveStatic("/", LittleFS, "/index.html");
  server.begin();
}

void loop() {
  server.handleClient();

  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();

  air = analogRead(SENSOR_PIN);
  if (autoMode) setFan(air > FAN_ON_AT);

  lcd.setCursor(0, 0);
  lcd.printf("Air:%-5d %s   ", air, air > FAN_ON_AT ? "BAD" : "OK");
  lcd.setCursor(0, 1);
  lcd.printf("Fan:%-3s %-4s    ", fanOn ? "ON" : "OFF", autoMode ? "AUTO" : "MAN");
}
