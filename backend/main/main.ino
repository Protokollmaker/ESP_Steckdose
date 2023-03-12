// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "password.h"
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = SSID_WLAN;
const char* password = PASSWORD_WLAN;
#define RELAY_NUMBER 5
#define TIMER_NUMBER RELAY_NUMBER
#define BLINK_LEDS {0,2,14,12,13}
#define SHIFT_OUT 16           // GPIO DO
#define SHIFT_SHIFT 5          // GPIO D1
#define SHIFT_OUTPUT_ENABLE 4  //GPIO D2

#define URL_INDEX_HTML  "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backend/data/V2.0/index.html"
#define URL_INDEX_CSS   "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backend/data/V2.0/layout.css"
#define URL_INDEX_JS    "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backend/data/V2.0/layout.js"
#define FILE_INDEX_HTML "/index.html"
#define FILE_INDEX_CSS  "/style.css"
#define FILE_INDEX_JS   "/index.js"

#define LOG_DOWNLADE(URL, FILE) Serial.print("[Downlade File] start Downladeing html"); \
                                Serial.print(FILE); \
                                Serial.print(" from "); \
                                Serial.println(URL);

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

bool downloadToFile(const char* filename, const char* fileURL){
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    Serial.print("[HTTPS] begin...\n");
    if (!https.begin(*client, fileURL)) {
        Serial.printf("[HTTPS] Unable to connect\n");
        return 1;
    }
    Serial.print("[HTTPS] GET...\n");
    int httpCode = https.GET();
    if (!(httpCode > 0)) {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        return 1;
    }
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

    // file found at server
    if (!(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
        Serial.printf("[HTTPS] error \n");
        return 1;
    }
    String payload = https.getString();
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.println("[LittleFS] Failed to open file for writing");
        return 1;
    }

    auto bytesWritten = file.write(payload.c_str(), payload.length());
    Serial.printf("[LittleFS] bytesWritten: %d\n", bytesWritten);
    Serial.printf("should be %d\n", payload.length());
    file.close();
    return 0;
}

void printMessage(){
    Serial.printf("test");
}

//Ticker timer1(printMessage, 500);
//int tiemr = 0;

bool getRelayState(int t_relay){
    return relayState & (1 << t_relay);
}

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
    if (getRelayState(0)){return "ON";}
    else{return "OFF";}
  }
  if(var == "STATE1"){
    if (getRelayState(1)){return "ON";}
    else{return "OFF";}
  }
  if(var == "STATE2"){
    if (getRelayState(2)){return "ON";}
    else{return "OFF";}
  }
  if(var == "STATE3"){
    if (getRelayState(3)){return "ON";}
    else{return "OFF";}
  }
  if(var == "STATE4"){
    if (getRelayState(4)){return "ON";}
    else{return "OFF";}
  }
  if (var == "Timer0"){
    return String(timer[0]);
  }
  if (var == "Timer1"){
    return String(timer[1]);
  }
  if (var == "Timer2"){
    return String(timer[2]);
  }
  if (var == "Timer3"){
    return String(timer[3]);
  }
  if (var == "Timer4"){
    return String(timer[4]);
  }
  if (var == "Run0"){timerRun[0]}
  if (var == "Run1"){timerRun[1]}
  if (var == "Run2"){timerRun[2]}
  if (var == "Run3"){timerRun[3]}
  if (var == "Run4"){timerRun[4]}
  if (var == "TimerTurn0") {timerTurn[0]}
  if (var == "TimerTurn1") {timerTurn[1]}
  if (var == "TimerTurn2") {timerTurn[2]}
  if (var == "TimerTurn3") {timerTurn[3]}
  if (var == "TimerTurn4") {timerTurn[4]}
  return String();
}

/*String processor1(const String& var){
  return "02139"
}*/

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  for (int i = 0; i < TIMER_NUMBER; i++){
    timer[i] = -1;
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
  }
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send_P(200, "text/html", index_html, processor);

  });
  if (!LittleFS.exists(FILE_INDEX_HTML)) {
      LOG_DOWNLADE(FILE_INDEX_HTML, URL_INDEX_HTML);
      downloadToFile(FILE_INDEX_HTML, URL_INDEX_HTML);
  }
  if (!LittleFS.exists(FILE_INDEX_CSS)) {
      LOG_DOWNLADE(FILE_INDEX_CSS, URL_INDEX_CSS);
      downloadToFile(FILE_INDEX_CSS, URL_INDEX_CSS);
  }
  if (!LittleFS.exists(FILE_INDEX_JS)) {
      LOG_DOWNLADE(FILE_INDEX_JS, URL_INDEX_JS);
      downloadToFile(FILE_INDEX_JS, URL_INDEX_JS);
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
      request->send(LittleFS, FILE_INDEX_HTML, "text/html"); 
  });
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