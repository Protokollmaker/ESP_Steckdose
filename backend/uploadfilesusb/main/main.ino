// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "password.h"
#include "config.h"
#include <ArduinoJson.h>
#include "LittleFS.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

// Replace with your network credentials
const char* ssid = SSID_WLAN;
const char* password = PASSWORD_WLAN;

//int timerRelay[TIMER_NUMBER];
int timer[TIMER_NUMBER];
bool timerTurn[TIMER_NUMBER];
bool timerRun[TIMER_NUMBER];

bool ledState = 0;
const int ledPin = 2;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


int relayState = 0b0;
bool timerState = 0;
int blink_pins[] = BLINK_LEDS; 

void setRelayState(int t_relay, bool t_stata){
    if (t_stata)
        relayState |= (1 << t_relay);
    else
        relayState &= ~(1 << t_relay);
    Serial.print("[I/O] shiftout: ");
    Serial.println(relayState);
    // update Relay State on I/O PINS
    shiftoutData();
}

void shiftoutData() {
    digitalWrite(SHIFT_SHIFT, LOW);
    shiftOut(SHIFT_OUT, SHIFT_SHIFT, MSBFIRST, relayState);
    digitalWrite(SHIFT_OUTPUT_ENABLE, LOW);
    digitalWrite(SHIFT_OUTPUT_ENABLE, HIGH);
}

bool deleteFile(const char* filename) {
    Serial.printf("[LittleFS] delete file");
    Serial.println(filename);
    if (!LittleFS.remove(filename)) {
        Serial.println("[LittleFS]Failed to delete file");
        return 1;
    }
    return 0;
}

bool getRelayState(int t_relay){
    return relayState & (1 << t_relay);
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
      timerRun[doc["timerID"].as<int>()] = doc["state"].as<bool>();
      timerTurn[doc["timerID"].as<int>()] = doc["turn"].as<bool>();
      //timerRelay[doc["timerID"].as<int>()] = doc["relay"].as<int>();
    }
    if (!strcmp(eventtype, "setRelayState")){
      setRelayState(doc["relay"].as<int>(), doc["state"].as<bool>());
    }
    if (!strcmp(eventtype, "turnTimer")){
      timerTurn[doc["timerID"].as<int>()] = doc["turn"].as<bool>();
    }
    if (!strcmp(eventtype, "stopTimer")){
      timerRun[doc["timerID"].as<int>()] = doc["state"].as<bool>();
      digitalWrite(blink_pins[doc["timerID"].as<int>()], 0);
    }
    if   (!strcmp(eventtype, "UpdateFrontend")) {
        if (!strcmp(doc["file"], FILE_INDEX_HTML)) {
            deleteFile(FILE_INDEX_HTML);
        }
        if (!strcmp(doc["file"], FILE_INDEX_CSS)) {
            deleteFile(FILE_INDEX_CSS);
        }
        if (!strcmp(doc["file"], FILE_INDEX_JS)) {
            deleteFile(FILE_INDEX_JS);
        }
        return;
    }
    if (!strcmp(eventtype, "restat")) {
      ESP.restart();
      return;
    }
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
  if(var == "STATE0"){
    return String(getRelayState(0) ? "ON" : "OFF");
  }
  if(var == "STATE1"){
    return String(getRelayState(1) ? "ON" : "OFF");
  }
  if(var == "STATE2"){
    return String(getRelayState(2) ? "ON" : "OFF");
  }
  if(var == "STATE3"){
    return String(getRelayState(3) ? "ON" : "OFF");
  }
  if(var == "STATE4"){
    return String(getRelayState(4) ? "ON" : "OFF");
  }
  if (var == "Timer0")      {return String(timer[0]);}
  if (var == "Timer1")      {return String(timer[1]);}
  if (var == "Timer2")      {return String(timer[2]);}
  if (var == "Timer3")      {return String(timer[3]);}
  if (var == "Timer4")      {return String(timer[4]);}
  if (var == "Run0")        {return String(timerRun[0]? "true": "false");}
  if (var == "Run1")        {return String(timerRun[1]? "true": "false");}
  if (var == "Run2")        {return String(timerRun[2]? "true": "false");}
  if (var == "Run3")        {return String(timerRun[3]? "true": "false");}
  if (var == "Run4")        {return String(timerRun[4]? "true": "false");}
  if (var == "TimerTurn0")  {return String(timerTurn[0]? "true": "false");}
  if (var == "TimerTurn1")  {return String(timerTurn[1]? "true": "false");}
  if (var == "TimerTurn2")  {return String(timerTurn[2]? "true": "false");}
  if (var == "TimerTurn3")  {return String(timerTurn[3]? "true": "false");}
  if (var == "TimerTurn4")  {return String(timerTurn[4]? "true": "false");}
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  for (int i = 0; i < TIMER_NUMBER; i++){
    timer[i] = -1;
    timerRun[i] = 1;
  }

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("[Wifi] WiFi Failed!\n");
    return;
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
  Serial.println("\n[LittleFS] TRY to open filesystem");
  if(!LittleFS.begin()){
      Serial.println("[LittleFS] An Error has occurred while mounting LittleFS");
      return;
  }
  initWebSocket();
  pinMode(SHIFT_OUT, OUTPUT);
  pinMode(SHIFT_SHIFT, OUTPUT);
  pinMode(SHIFT_OUTPUT_ENABLE, OUTPUT);

  for (int i = 0; i < RELAY_NUMBER; i++){
      pinMode(blink_pins[i], OUTPUT);
      digitalWrite(blink_pins[i], 0);
  }
  // Route for root / web page
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){ 
      request->send(LittleFS, FILE_INDEX_CSS, "text/css"); 
  });
  server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request){ 
      request->send(LittleFS, FILE_INDEX_JS, "text/js"); 
  });
  // Start server
  server.begin();
}

void loop() {
  timerState = !timerState;
  ws.cleanupClients();
  for (int i = 0; i < TIMER_NUMBER; i++){
    if (timer[i] == -1){continue;}
    if (!timerRun[i])  {continue;}
    if (timer[i] == 0)  {
      StaticJsonDocument<96> doc;
      doc["eventtype"] = "setRelayState";
      doc["relay"] = i;
      doc["state"] = timerTurn[i];
      String output;
      serializeJson(doc, output);
      ws.textAll(output);
      digitalWrite(blink_pins[i],0);
      timer[i] = timer[i] -1;
      setRelayState(i, timerTurn[i]);
      return;
    }
    Serial.print("Timer: ");
    Serial.print(i);
    Serial.print(": Timer: ");
    Serial.println(timer[i]);
    digitalWrite(blink_pins[i],timerState);
    timer[i] = timer[i] -1;
  }
  ws.textAll("{\"eventtype\":\"Tick\"}");
  delay(1000);
}