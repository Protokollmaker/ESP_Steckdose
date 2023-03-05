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
namespace Name {
  #include <LinkedList.h>
}

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
    return !!timerlist[timerID].timerLeft;
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
        Serial.println(timerIsSet(i));
        if (timerIsSet(i)){continue;}
        Serial.println("passed");
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

Name::LinkedList<String> eventList = Name::LinkedList<String>();

void sendTimerUpdate(String &string, int timerID){
    bool error = 0;
    Timer timer = getTimer(timerID, error);
    StaticJsonDocument<300> event;
    if (error) {
        event["eventtype"] = "error";
        event["msg"] = "Out of index";
        serializeJson(event, string);
        return;
    }
    event["eventtype"] = "Timer";
    event["timerID"] = timerID;
    event["time"] = timer.timerLeft;
    event["Relay"] = timer.relay;
    event["turn"] = timer.state;
    event["run"] = timer.run;
    serializeJson(event, string);
    return;
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){return;}
    if(type == WS_EVT_DISCONNECT){ return;}
    if(type == WS_EVT_ERROR){ return;}
    if(type == WS_EVT_PONG){return;}
    if(type == WS_EVT_DATA){
        /*for(size_t i=0; i<len; i++){
            Serial.write(data[i]);
        }
        Serial.println();*/
        StaticJsonDocument<96> doc;
        DeserializationError error = deserializeJson(doc, (char*)data, len);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            doc.clear();
            return;
        }
        const char* eventtype = doc["eventtype"];
        Serial.println(eventtype);
        if   (!strcmp(eventtype, "setTimer")) {
            Timer timer = {doc["time"], doc["Relay"], doc["turn"], doc["run"]};
            int newTimerIndex;
            if (setNewTimer(timer, newTimerIndex))
                return; // ! Error 
            String event;
            sendTimerUpdate(event, newTimerIndex);
            eventList.add(event);
            //ws.textAll(event);
            //client->text(data, len);
        }
        if   (!strcmp(eventtype, "getTimers")) {
            for (int i = 0; i < NUMBER_OF_TIMERS; i++){
                if (!timerIsSet(i)) {continue;} 
                String event;
                sendTimerUpdate(event, i);
                eventList.add(event);
                //client->text(event);
            }
        }
        if   (!strcmp(eventtype, "getTimer")) {
            if (!timerIsSet(doc["timerID"])) {return;}
            String event;
            sendTimerUpdate(event, doc["timerID"]);
            eventList.add(event);
            //client->text(event);
        }
        doc.clear();
        return;
    }
}
String tickString;

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
    StaticJsonDocument<32> tickEvent;
    tickEvent["eventtype"] = "Tick";
    serializeJson(tickEvent, tickString);
    server.begin();
}
bool timerState = 1;

void loop() {
    int theSize = eventList.size();
    for (int i = 0; i < theSize; i++) {
        String event = eventList.pop();
        ws.textAll(event);
    }
    String event;
    sendTimerUpdate(event, 1);
    ws.textAll(event);
    String event1;
    sendTimerUpdate(event1, 2);
    ws.textAll(event1);
    
    if (timerState) { timerState = 0; } 
    else            { timerState = 1; }
    
    for (int i = 0; i < NUMBER_OF_TIMERS; i++){
        if (timerlist[i].timerLeft <= 0)
            continue;
        timerlist[i].timerLeft = timerlist[i].timerLeft - 1;
        if (!timerlist[i].run)
            continue;
        if (!timerlist[i].timerLeft){
            delTimer(i);
            //digitalWrite(blink_pins[timerlist[i].relay],LOW);
        }
        
        //digitalWrite(blink_pins[timerlist[i].relay],timerState);

        Serial.print("[TIMER] Timer : ");
        Serial.print(i);
        Serial.print(" : TimerLeft : ");
        Serial.println(timerlist[i].timerLeft);
    }
    
    ws.textAll(tickString);

    ws.cleanupClients();
    delay(1000);
}