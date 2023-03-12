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

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP Web Server</title>
  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    //var gateway = "ws://192.168.178.123/ws"
    webSocket = new WebSocket(gateway, "protocolOne");
    webSocket.onopen = function(event) {};
    webSocket.onmessage = function(event) {
      let data = JSON.parse(event.data);
      console.log("data: ", data);
      switch (data.eventtype) {
      case "Tick":
        const timers = document.getElementsByClassName("tick");
        for (let i = 0; i < timers.length; i++) {
          if (timers[i].innerHTML > 0)
            timers[i].innerHTML = parseInt(timers[i].innerHTML) - 1;
        }
        return;
      case "setRelayState":
        document.getElementById("state".concat(data.relay)).innerHTML = data.state ? "ON":"OFF";
        return;
      case "setTimer":
        document.getElementById("timer".concat(data.timerID)).innerHTML = data.time;
        return;
      }
    }

    function ToggleRelay(relay, relayClass){
      webSocket.send(JSON.stringify({
                  eventtype: "setRelayState",
                  relay: relay,
                  state: document.getElementById(relayClass).innerHTML == "ON" ? 0 : 1
                  }));
    }
  </script>
</head>
<body>
  <div class="topnav">
    <h1>ESP WebSocket Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Relais 0</h2>
      <p class="timer">time: <span id="timer0" class="tick">%Timer0%</span></p>
      <p class="state">state: <span id="state0">%STATE0%</span></p>
      <p><button id="toggle0" onclick="ToggleRelay(0,'state0')" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Relais 1</h2>
      <p class="timer">time: <span id="timer1" class="tick">%Timer1%</span></p>
      <p class="state">state: <span id="state1">%STATE1%</span></p>
      <p><button id="toggle1" onclick="ToggleRelay(1,'state1')" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Relais 2</h2>
      <p class="timer">time: <span id="timer2" class="tick">%Timer2%</span></p>
      <p class="state">state: <span id="state2">%STATE2%</span></p>
      <p><button id="toggle2" onclick="ToggleRelay(2,'state2')" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Relais 3</h2>
      <p class="timer">time: <span id="timer3" class="tick">%Timer3%</span></p>
      <p class="state">state: <span id="state3">%STATE3%</span></p>
      <p><button id="toggle3" onclick="ToggleRelay(3,'state3')" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Relais 4</h2>
      <p class="timer">time: <span id="timer4" class="tick">%Timer4%</span></p>
      <p class="state">state: <span id="state4">%STATE4%</span></p>
      <p><button id="toggle4" onclick="ToggleRelay(4,'state4')" class="button">Toggle</button></p>
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