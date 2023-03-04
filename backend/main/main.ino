#include "config.h"
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = SSID_WLAN;
const char* password = PASSWORD_WLAN;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); 

struct Timer {
    int timerLeft = 0;
    int relay = 0;
    bool state = 0;
    bool run = 0;
};

Timer timerlist[NUMBER_OF_TIMERS];

bool timerIsSet(int timerID){
    if (timerID < 0 || timerID > NUMBER_OF_TIMERS) {Serial.println("[Timer] Out of index"); return 0;}
    if(timerlist[timerID].timerLeft);
        return 1;
    return 0;
}

Timer getTimer(int timerID, bool &error){
    if (timerID < 0 || timerID > NUMBER_OF_TIMERS) {
        Serial.println("[Timer] Out of index");
        error = 1;
        return timerlist[0];
    }
    return timerlist[timerID];
}

bool setTimer(Timer timer, int timerID){
    if (timerID < 0 || timerID > NUMBER_OF_TIMERS) {Serial.println("[Timer] Out of index"); return 1;}
    timerlist[timerID] = timer;
    return 0;
}

bool setNewTimer(Timer timer, int &timerIndex){
    timerIndex = -1;
    for (int i = 0; i < NUMBER_OF_TIMERS; i++){
        if (!timerIsSet(i)){continue;}
        timerIndex = i;
        break;
    }
    if (timerIndex == -1){
        Serial.print("[Timer] no Timer Free");
        return 1;
    }
    return setTimer(timer, timerIndex);
}

bool delTimer(int timerID) {
    if (timerID < 0 || timerID > NUMBER_OF_TIMERS) {Serial.println("[Timer] Out of index"); return 1;}
    timerlist[timerID].timerLeft = 0;
    return 0;
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){return;}
    if(type == WS_EVT_DISCONNECT){ return;}
    if(type == WS_EVT_ERROR){ return;}
    if(type == WS_EVT_PONG){return;}
    if(type == WS_EVT_DATA){
        for(size_t i=0; i<len; i++){
            Serial.write(data[i]);
        }
        Serial.println();
        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, (char*)data, len);
        if (err) {
            Serial.print(F("[JSON] deserializeJson() failed: "));
            Serial.println(err.c_str());
            return;
        }
        const char* eventtype = doc["eventtype"];
        if   (!strcmp(eventtype, "setTimer")) {
            Timer timer = {doc["time"], doc["Relay"], doc["turn"], doc["run"]};
            int newTimerIndex;
            if (setNewTimer(timer, newTimerIndex))
                return; // Error 
            return;
        }
        if   (!strcmp(eventtype, "getTimers")) {
            for (int i = 0; i < NUMBER_OF_TIMERS; i++){
                // ! implement
            }
        }
        doc.clear();
        return;
    }
}

void setup(){
    // Serial port for debugging purposes
    Serial.begin(115200);
    // Connect to Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID_WLAN, PASSWORD_WLAN);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("[Wifi] WiFi Failed!\n");
        return;
    }
    Serial.println("[Wifi] IP address: ");
    Serial.println(WiFi.localIP());


    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Test");
    });
    //server.onNotFound(onRequest);
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
        }
    });
    server.begin();
}
bool timerState = 1;

void loop() {
    if (timerState) { timerState = 0; } 
    else            { timerState = 1; }
    
    for (int i = 0; i < NUMBER_OF_TIMERS; i++){
        if (timerlist[i].timerLeft <= 0)
            continue;
        if (!timerlist[i].run)
            continue;
        if (!timerlist[i].timerLeft){
            delTimer(i);
            //digitalWrite(blink_pins[timerlist[i].relay],LOW);
        }
        timerlist[i].timerLeft = timerlist[i].timerLeft - 1;
        //digitalWrite(blink_pins[timerlist[i].relay],timerState);

        Serial.print("[TIMER] Timer : ");
        Serial.print(i);
        Serial.print(" : TimerLeft : ");
        Serial.println(timerlist[i].timerLeft);
    }
    
    StaticJsonDocument<32> event;
    event["eventtype"] = "Tick";
    String output;
    serializeJson(event, output);
    ws.textAll(output);

    ws.cleanupClients();
    delay(1000);
}