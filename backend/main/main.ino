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
#define RELAY_NUMBER 5
int timer[TIMER_NUMBER];

bool ledState = 0;
const int ledPin = 2;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
#define BLINK_LEDS {0,2,14,12,13}
#define SHIFT_OUT 16           // GPIO DO
#define SHIFT_SHIFT 5          // GPIO D1
#define SHIFT_OUTPUT_ENABLE 4  //GPIO D2

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
      <h2>Relais 0</h2>
      <p class="state">state: <span id="state">%Timer0%</span></p>
      <p class="state">state: <span id="state">%STATE0%</span></p>
      <p><button id="button" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Relais 1</h2>
      <p class="state">state: <span id="state">%Timer1%</span></p>
      <p class="state">state: <span id="state">%STATE1%</span></p>
      <p><button id="button" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Relais 2</h2>
      <p class="state">state: <span id="state">%Timer2%</span></p>
      <p class="state">state: <span id="state">%STATE2%</span></p>
      <p><button id="button" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Relais 3</h2>
      <p class="state">state: <span id="state">%Timer3%</span></p>
      <p class="state">state: <span id="state">%STATE3%</span></p>
      <p><button id="button" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Relais 4</h2>
      <p class="state">state: <span id="state">%Timer4%</span></p>
      <p class="state">state: <span id="state">%STATE4%</span></p>
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
    }
    if (!strcmp(eventtype, "setRelayState")){
      setRelayState(doc["relay"].as<int>(), doc["state"].as<bool>());
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
  if(var == "STATE0"){
    if (getRelayState(0)){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  if(var == "STATE1"){
    if (getRelayState(1)){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  if(var == "STATE2"){
    if (getRelayState(2)){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  if(var == "STATE3"){
    if (getRelayState(3)){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  if(var == "STATE4"){
    if (getRelayState(4)){
      return "ON";
    }
    else{
      return "OFF";
    }
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

  initWebSocket();
  pinMode(SHIFT_OUT, OUTPUT);
  pinMode(SHIFT_SHIFT, OUTPUT);
  pinMode(SHIFT_OUTPUT_ENABLE, OUTPUT);

  for (int i = 0; i < RELAY_NUMBER; i++){
      pinMode(blink_pins[i], OUTPUT);
  }
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Start server
  server.begin();
}

void loop() {
  timerState = !timerState;
  ws.cleanupClients();
  for (int i = 0; i < TIMER_NUMBER; i++){
    if (timer[i] == -1){
      continue;
    }
    if (timer[i] == 0)  {
      StaticJsonDocument<96> doc;

      doc["eventtype"] = "setRelayState";
      doc["relay"] = i;
      doc["state"] = false;
      String output;
      serializeJson(doc, output);
      ws.textAll(output);
      digitalWrite(blink_pins[i],0);
      timer[i] = timer[i] -1;
      setRelayState(i, false);
      return;
    }
    Serial.print("Timer: ");
    Serial.print(i);
    Serial.print(": Timer: ");
    Serial.println(timer[i]);
    digitalWrite(blink_pins[i],timerState);
    timer[i] = timer[i] -1;
  }
  ws.textAll("Tick");
  delay(1000);
}