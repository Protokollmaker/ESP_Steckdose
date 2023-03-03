#include "config.h"
#include <ArduinoJson.h>
//#include <ArduinoSTL.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h> 

#include "LittleFS.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

struct Timers {
    int time = 0;
    int Relay = 0;
    bool state = 0;
    bool run = 0;
};

int timer_pins[] = BLINK_LEDS;

constexpr int getBitNumberTimerUsed(int mumberOfTimer, int typesize = 8){
    int numberOfBits = NUMBER_OF_TIMERS/typesize;
    if (NUMBER_OF_TIMERS % typesize)
        numberOfBits += 1;
    return numberOfBits;
}

uint8_t TimerUsed[getBitNumberTimerUsed(NUMBER_OF_TIMERS - 1)];
Timers Timerlist[NUMBER_OF_TIMERS];

uint8_t relayState = RELAY_STARTUP;
uint8_t timerState = 0b0;

int blink_pins[] = BLINK_LEDS;

bool promtWlanLogin = false;

bool getTimerInUse(int timerID, int typesize = 8){
    return TimerUsed[timerID/typesize] & (1 << timerID % typesize);
}

void setTimerInUse(int timerID,int state, int typesize = 8){
    if (state)
        TimerUsed[timerID/typesize] |= (1 << timerID % typesize);
    else
        TimerUsed[timerID/typesize] &= ~(1 << timerID % typesize);
}

int newTimerIndex = 0;
bool setNewTimer(Timers timer){
    int indexTimerNotInUse = -1;
    for (int i = 0; i < NUMBER_OF_TIMERS; i++){
        if (getTimerInUse(i)) {continue;} // if timer in use skip
        indexTimerNotInUse = i;
        break;
    }
    if (indexTimerNotInUse == -1){
        Serial.print("[Timer] no Timer Free");
        return 1;
    }
    newTimerIndex = indexTimerNotInUse;
    setTimer(timer, indexTimerNotInUse);
    return 0;
}

bool setTimer(Timers timer, int timerID){
    if (timerID >= NUMBER_OF_TIMERS){ Serial.print("[Timer] over index setTimer");return 1;}
    setTimerInUse(timerID, 1);
    Timers timer_obj = timer;
    Timerlist[timerID] = timer_obj;
    return 0;
}

Timers getTimer(int timerID){
    return Timerlist[timerID];
}

bool delTimer(int timerID){
    if (timerID >= NUMBER_OF_TIMERS){ Serial.print("[Timer] over index delTimer");return 1;}
    setTimerInUse(timerID, 0);
    return 0;
}

AsyncWebServer server(WSERVER_PORT);
AsyncWebSocket ws(WSOCKET_ACCSESS); 
AsyncEventSource events(WEBSOCKET_EVENTS);

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

void onRequest(AsyncWebServerRequest *request) { //Handle Unknown Request
  request->send(404, "text/plain", "Not found");
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {}

void shiftoutData() {
    digitalWrite(SHIFT_SHIFT, LOW);
    shiftOut(SHIFT_OUT, SHIFT_SHIFT, MSBFIRST, relayState);
    digitalWrite(SHIFT_OUTPUT_ENABLE, LOW);
    digitalWrite(SHIFT_OUTPUT_ENABLE, HIGH);
}

String eventGetRelayState(int relay){
    StaticJsonDocument<96> event;
    event["eventtype"] = "setRelayState";
    event["Relay"] = relay;
    event["turn"] = getRelayState(relay);
    String output;
    serializeJson(event, output);
    return output;
}

String TimerRequest(int timerID) {
    Timers timer_obj = getTimer(timerID);
    StaticJsonDocument<128> event;
    event["eventtype"] = "Timer";
    event["timerID"] = timerID;
    event["time"] = timer_obj.time;
    event["Relay"] = timer_obj.Relay;
    event["turn"] = timer_obj.state;
    event["run"] = timer_obj.run;
    String output;
    serializeJson(event, output);
    return output;
}

void sentTimerdelete(int t_timer) {
    StaticJsonDocument<64> event;
    event["eventtype"] = "endTimer";
    event["timerID"] = t_timer;
    String output;
    serializeJson(event, output);
    ws.textAll(output);
}

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

void eventSetRelayState(int t_relay, bool t_stata){
    setRelayState(t_relay, t_stata);
    ws.textAll(eventGetRelayState(t_relay));
}

bool getRelayState(int t_relay){
    return relayState & (1 << t_relay);
}

void onMassageEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, StaticJsonDocument<LAGES_SOCKET_EVENT_FOR_ARDURINO_JSON>& doc) {
    const char* eventtype = doc["eventtype"];
    if (!strcmp(eventtype, "setRelayState")) {
        eventSetRelayState(doc["Relay"], doc["turn"]);
        return;
    }
    if (!strcmp(eventtype, "getRelayState")) { // Wandle in einzehlene events
        client->text(eventGetRelayState(doc["Relay"]));
        return;
    } 
    if (!strcmp(eventtype, "getRelaysState")) { // Wandle in einzehlene events
        for (int i = 0; i < NUMBER_OF_RELAY; i++) {
            client->text(eventGetRelayState(i));
        }
        return;
    } 
    if   (!strcmp(eventtype, "setTimer")) {
        Timers timer = {doc["time"], doc["Relay"], doc["turn"], doc["run"]};
        setNewTimer(timer); // ! Implement error
        ws.textAll(TimerRequest(newTimerIndex));
        return;
    }
    if   (!strcmp(eventtype, "getTimers")) {
        for (int i = 0; i < NUMBER_OF_TIMERS; i++){
            if (!getTimerInUse(i)) {continue;} 
            client->text(TimerRequest(i)); 
        }
        return;
    }
    if   (!strcmp(eventtype, "getTimer")) {
        client->text(TimerRequest(doc["timerID"])); 
        return;
    }
    if   (!strcmp(eventtype, "updateTimer")) {
        Timers timer_obj = {doc["time"], doc["Relay"], doc["turn"], doc["run"]};
        timer_obj.run = doc["run"];
        setTimer(timer_obj, doc["timerID"]); // ! Implement error
        ws.textAll(TimerRequest(doc["timerID"]));
        return;
    }
    if   (!strcmp(eventtype, "delTimer")) {
        delTimer(doc["timerID"]); // ! Implement error
        sentTimerdelete(doc["timerID"]);
        return;
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
        ESP.restart();
        return;
    }
    if   (!strcmp(eventtype, "delWlanConfig")) {
        #ifndef SSID_WLAN
            deleteFile(FILE_WLAN_PASSWORD);
            ESP.restart();
        #else 
            StaticJsonDocument<96> event;
            event["eventtype"] = "error";
            event["msg"] = "Password was defined by #definined";
            String output;
            serializeJson(event, output);
            client->text(output);
        #endif
        return;
    }
    if   (!strcmp(eventtype, "setWlanConfig")) {
        #ifndef SSID_WLAN
            StaticJsonDocument<96> serialize;
            serialize["SSID"] = doc["SSID"];
            serialize["Password"] = doc["Password"];
            String output;
            serializeJson(doc, output);
            File file = LittleFS.open(FILE_WLAN_PASSWORD, "w");
            if (!file) {
                Serial.println("[LittleFS] Failed to open file for writing");
                return;
            }
            auto bytesWritten = file.write(output.c_str(), output.length());
            Serial.printf("[LittleFS] bytesWritten: %d\n", bytesWritten);
            Serial.printf("should be %d\n", output.length());
            ESP.restart();
        #else 
            StaticJsonDocument<96> event;
            event["eventtype"] = "error";
            event["msg"] = "Password was defined by #definined";
            String output;
            serializeJson(event, output);
            client->text(output);
        #endif
        return;
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    if (type == WS_EVT_CONNECT){ // on Join
        return;
    }
    if (type == WS_EVT_DISCONNECT) {return;}
    if (type == WS_EVT_PONG) {return;}
    if (type == WS_EVT_ERROR) {return;}
    if (type == WS_EVT_DATA) {
        for(size_t i=0; i<len; i++){
            Serial.write(data[i]);
        }
        StaticJsonDocument<LAGES_SOCKET_EVENT_FOR_ARDURINO_JSON> doc;
        DeserializationError err = deserializeJson(doc, (char*)data, len);
        if (err) {
            Serial.print(F("[JSON] deserializeJson() failed: "));
            Serial.println(err.c_str());
            return;
        }
        onMassageEvent(server, client, doc);
    }
}

void setup() {
    Serial.begin(SERIAL_SPEED);
    //Serial.println("\n[Config] set wdt");
    //ESP.wdtDisable();
    //ESP.wdtEnable(1500); // max loop time 1,5sec
    Serial.println("\n[LittleFS] TRY to open filesystem");
    if(!LittleFS.begin()){
        Serial.println("[LittleFS] An Error has occurred while mounting LittleFS");
        return;
    }
    #ifdef SSID_WLAN
        WiFi.mode(WIFI_STA);
        WiFi.begin(SSID_WLAN, PASSWORD_WLAN);
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.printf("[Wifi] WiFi Failed!\n");
            return;
        }
    #else
        // TODO IMPLEMENT Can't connect and open file get login
    #endif
    Serial.println("[I/O] Define I/O Modes");
    pinMode(SHIFT_OUT, OUTPUT);
    pinMode(SHIFT_SHIFT, OUTPUT);
    pinMode(SHIFT_OUTPUT_ENABLE, OUTPUT);

    for (int i = 0; i < NUMBER_OF_RELAY; i++){
        pinMode(blink_pins[i], OUTPUT);
    }
    Serial.println("[I/O] Define state");
    shiftoutData();
    for (int i = 0; i < NUMBER_OF_RELAY; i++){
        digitalWrite(blink_pins[i], LOW);
    }
    Serial.println("[Timer] Init timer");
    for (int i = 0; i < NUMBER_OF_TIMERS;i++){
        delTimer(i);
    }
    Serial.println("[Wifi] IP address: ");
    Serial.println(WiFi.localIP());

    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.addHandler(&events);
    Serial.println("[Webserver] Init Sides");
    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/text", HTTP_ANY, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Test"); // TODO Redidirect to webseite.com?ipaddress
    });
    server.onNotFound(onRequest);
    server.begin();

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
    Serial.println("[Webserver] Begin");
    
}

void loop() { 
    bool oneTimerAktiv = 0;
    
    if (timerState) { timerState = 0; } 
    else            { timerState = 1; }

    // TODO use memory pointers to optimice programm
    for (int i = 0; i < NUMBER_OF_TIMERS; i++){
        if (!getTimerInUse(i)) { continue; }
        oneTimerAktiv = 1;
        Timers timer = getTimer(i);
        if (!timer.run) { continue; }
        if (!timer.time){
            delTimer(i);
            digitalWrite(blink_pins[timer.Relay],LOW);
            eventSetRelayState(timer.Relay, timer.state);
            sentTimerdelete(i);
            continue;
        }
        timer.time = timer.time - 1;
        setTimer(timer, i);
        digitalWrite(blink_pins[timer.Relay],timerState);
        Serial.print("[TIMER] Timerleft : ");
        Serial.print(timer.time);
        Serial.print(" : Timer : ");
        Serial.println(i);
        
    }

    if (oneTimerAktiv){
        StaticJsonDocument<32> event;
        event["eventtype"] = "Tick";
        String output;
        serializeJson(event, output);
        ws.textAll(output);
    }
    delay(1000);
}