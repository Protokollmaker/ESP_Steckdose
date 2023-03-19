#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <BearSSLHelpers.h>

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* url = "https://example.com/large_file.zip";

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
}

void loop() {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, url);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();
    while (stream->connected() || stream->available()) {
      if (stream->available()) {
        Serial.write(stream->read());
      }
    }
  }

  http.end();
  delay(1000);
}





