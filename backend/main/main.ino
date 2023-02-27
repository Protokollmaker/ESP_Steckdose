#include "config.h"
#include <ArduinoJson.h>
//#include <ArduinoSTL.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h> 
namespace Name {
  #include <LinkedList.h>
}
#include "LittleFS.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

struct Timers {
    unsigned long time = 0;
    int Relay = 0;
    bool state = 0;
    bool run = 0;
};

int timer_pins[] = BLINK_LEDS;

Name::LinkedList<Timers> TimerList = Name::LinkedList<Timers>();

uint8_t relayState = RELAY_STARTUP;
uint8_t timerState = 0b0;

int blink_pins[] = BLINK_LEDS;

bool promtWlanLogin = false;



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
    //Serial.println(payload.c_str());
    file.close();
    return 0;
}

void onRequest(AsyncWebServerRequest *request) {
  //Handle Unknown Request
  request->send(404, "text/plain", "Not found");
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  //Handle body
}

void shiftoutData() {
    digitalWrite(SHIFT_SHIFT, LOW);
    shiftOut(SHIFT_OUT, SHIFT_SHIFT, MSBFIRST, relayState);
    digitalWrite(SHIFT_OUTPUT_ENABLE, LOW);
    digitalWrite(SHIFT_OUTPUT_ENABLE, HIGH);
}

void sentTimerNew() {
    Timers timer_obj = TimerList.get(TimerList.size() - 1);
    StaticJsonDocument<128> event;
    event["eventtype"] = "newTimer";
    event["time"] = timer_obj.time;
    event["Relay"] = timer_obj.Relay;
    event["turn"] = timer_obj.state;
    event["run"] = timer_obj.run;
    String output;
    serializeJson(event, output);
    ws.textAll(output);
}

void sentTimerUpdate(int t_timer) {
    Timers timer_obj = TimerList.get(t_timer);
    StaticJsonDocument<128> event;
    event["eventtype"] = "updateTimer";
    event["time"] = timer_obj.time;
    event["Relay"] = timer_obj.Relay;
    event["turn"] = timer_obj.state;
    event["run"] = timer_obj.run;
    String output;
    serializeJson(event, output);
    ws.textAll(output);
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
    Serial.print(relayState);
    Serial.printf("\n");
    // update Relay State on I/O PINS
    shiftoutData();
}

void eventSetRelayState(int t_relay, bool t_stata){
    setRelayState(t_relay, t_stata);
    StaticJsonDocument<96> event;
    event["eventtype"] = "setRelayState";
    event["Relay"] = t_relay;
    event["turn"] = t_stata;
    String output;
    serializeJson(event, output);
    ws.textAll(output);
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
        for (int i = 0; i < NUMBER_OF_RELAY; i++) {
            StaticJsonDocument<96> event;
            event["eventtype"] = "setRelayState";
            event["Relay"] = i;
            event["turn"] = getRelayState(i);
            String output;
            serializeJson(event, output);
            client->text(output);
        }
        return;
    } 
    if   (!strcmp(eventtype, "setTimer")) {
        Timers timer = {doc["time"], doc["Relay"], doc["turn"], doc["run"]};
        TimerList.add(timer);
        sentTimerNew();
        return;
    }
    if   (!strcmp(eventtype, "stopTimer")) {
        Timers timer_obj = TimerList.get(doc["timerID"]);
        timer_obj.run = doc["run"];
        TimerList.set(doc["timerID"], timer_obj);
        sentTimerUpdate(doc["timerID"]);
        return;
    }
    if   (!strcmp(eventtype, "delTimer")) {
        TimerList.remove(doc["timerID"]);
        sentTimerdelete(doc["timerID"]);
        return;
    }
    if   (!strcmp(eventtype, "getTimers")) {
        int listSize = TimerList.size();
        for (int h = 0; h < listSize; h++) {
            Timers timer_obj = TimerList.get(h);
            StaticJsonDocument<128> event;
            event["eventtype"] = "updateTimer";
            event["time"] = timer_obj.time;
            event["Relay"] = timer_obj.Relay;
            event["turn"] = timer_obj.state;
            event["run"] = timer_obj.run;
            String output;
            serializeJson(event, output);
            client->text(output);
        }
        return;
    }
    if   (!strcmp(eventtype, "getTimer")) {
        Timers timer_obj = TimerList.get(doc["timerID"]);
        StaticJsonDocument<128> event;
        event["eventtype"] = "updateTimer";
        event["time"] = timer_obj.time;
        event["Relay"] = timer_obj.Relay;
        event["turn"] = timer_obj.state;
        event["run"] = timer_obj.run;
        String output;
        serializeJson(event, output);
        client->text(output);
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
    Serial.println("[Wifi] IP address: ");
    Serial.println(WiFi.localIP());

    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.addHandler(&events);

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    //request->send(LittleFS, "/index.html");
    request->send(200, "text/plain", "Test"); // TODO Redidirect to webseite.com?ipaddress
        // TODO Lidel webseite version
    });
    server.onNotFound(onRequest);
    server.begin();

    if (!LittleFS.exists(FILE_INDEX_HTML)) {
        Serial.print("[Downlade File] start Downladeing");
        Serial.print(FILE_INDEX_HTML);
        Serial.print(" from ");
        Serial.println(URL_INDEX_HTML);
        downloadToFile(FILE_INDEX_HTML, URL_INDEX_HTML);
    }
    server.on("/html", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->send(LittleFS, FILE_INDEX_HTML, "text/html"); 
    });
    
}

void loop() { 
    int listSize = TimerList.size();

   if (timerState) {
        timerState = 0;
   } else {
        timerState = 1;
   }

   for (int h = 0; h < listSize; h++) {
        Timers timer_obj = TimerList.get(h);
        if (!timer_obj.run)
          continue;
        timer_obj.time == timer_obj.time--;
        if (!timer_obj.time){
            eventSetRelayState(timer_obj.Relay, timer_obj.state);
            Serial.println("[TIMER] Timer endet");
            TimerList.remove(h);
            digitalWrite(blink_pins[timer_obj.Relay],LOW);
            sentTimerdelete(h);
            continue;            
        }
        TimerList.set(h, timer_obj);
        digitalWrite(blink_pins[timer_obj.Relay],timerState);
        // If the timer_objue is negative, print it
        Serial.print("[TIMER] Timerleft : ");
        Serial.print(timer_obj.time);
        Serial.print(" : Timer : ");
        Serial.println(h);
   }
    delay(1000);
}

// TODO Bratcast timers 
// TODO Bratcast timer endings
// TODO Timer Stop