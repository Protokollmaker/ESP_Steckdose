#include <ESP8266WiFi.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid     = "ESP8226-Access-Point";
const char* password = "123456789";
AsyncWebServer server(80);

IPAddress local_IP(192,168,4,22); // TODO check if 192.168.178.1 Defalte router works
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(ssid,password) ? "Ready" : "Failed!");
  //WiFi.softAP(ssid);
  //WiFi.softAP(ssid, password, channel, hidden, max_connection)
  
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Test"); // TODO server Website with websocket for password and username 
    // ! webside must be in string from
  });
  server.begin();  
  
}

void loop() {

  delay(1000);
}