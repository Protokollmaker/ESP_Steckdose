// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "password.h"
#include "Ticker.h"
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = SSID_WLAN;
const char* password = PASSWORD_WLAN;

#define TIMER_NUMBER 5
int timer[TIMER_NUMBER];

bool ledState = 0;
const int ledPin = 2;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>ESP WebSocket Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Output - GPIO 2</h2>
      <p class="state">state: <span id="state">Timer</span></p>
      <p class="state">state: <span id="state">%STATE%</span></p>
      <p><button id="button" class="button">Toggle</button></p>
    </div>
  </div>
</body>
</html>
)rawliteral";

void printMessage(){
    Serial.printf("test");
}

//Ticker timer1(printMessage, 500);
//int tiemr = 0;

void notifyClients() {
  ws.textAll(String(ledState));
}



void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    ws.textAll((char*)data);
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, (char*)data, len);
    if (err) {
      Serial.print(F("[JSON] deserializeJson() failed: "));
      Serial.println(err.c_str());
      return;
    }
    const char* eventtype = doc["eventtype"];
    if   (!strcmp(eventtype, "setTimer")) {
      if (doc["timerID"].as<int>() < TIMER_NUMBER && doc["timerID"].as<int>() >= 0) //check in bounse
      {
        timer[doc["timerID"].as<int>()] = doc["time"].as<int>();
      }
    }
    if (!strcmp(eventtype, "setRelay")){

    }
    /*if (strcmp((char*)data, "toggle") == 0) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      notifyClients();
    }*/
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}

/*String processor1(const String& var){
  return "02139"
}*/

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  for (int i = 0; i < TIMER_NUMBER; i++){
    timer[i] = 0;
  }

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();
  for (int i = 0; i < TIMER_NUMBER; i++){
    if (timer[i] == -1){
      continue;
    }
    if (timer[i] == 0)  {
      ws.textAll((char*)"data");
    }
    
    Serial.print("Timer: ");
    Serial.print(i);
    Serial.print(": Timer: ");
    Serial.println(timer[i]);

    timer[i] = timer[i] -1;
  }
  ws.textAll("Tick");
  delay(1000);
}