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
};

int timer_pins[] = BLINK_LEDS;

Name::LinkedList<Timers> TimerList = Name::LinkedList<Timers>();

uint8_t relayState = RELAY_STARTUP;
uint8_t timerState = 0b0;

bool promtWlanLogin = false;



AsyncWebServer server(WSERVER_PORT);
AsyncWebSocket ws(WSOCKET_ACCSESS); 
AsyncEventSource events(WEBSOCKET_EVENTS);

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
    Serial.printf("bytesWritten: %d\n", bytesWritten);
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

void eventSetRelayState(int t_relay, bool t_stata){
    if (t_stata)
        relayState |= (1 << t_relay);
    else
        relayState &= ~(1 << t_relay);
    Serial.print("shiftout: ");
    Serial.print(relayState);
    Serial.printf("\n");
    // update Relay State on I/O PINS
    digitalWrite(SHIFT_SHIFT, LOW);
    shiftOut(SHIFT_OUT, SHIFT_SHIFT, MSBFIRST, relayState);
    digitalWrite(SHIFT_OUTPUT_ENABLE, LOW);
    digitalWrite(SHIFT_OUTPUT_ENABLE, HIGH);
}

bool eventGetRelayState(int t_relay){
    return relayState & (1 << t_relay);
}
void onMassageEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, StaticJsonDocument<LAGES_SOCKET_EVENT_FOR_ARDURINO_JSON>& doc) {
    const char* eventtype = doc["eventtype"];
    if (!strcmp(eventtype, "setRelayState")) {
        eventSetRelayState(doc["Relay"], doc["turn"]);
        StaticJsonDocument<96> event;
        event["eventtype"] = "setRelayState";
        event["Relay"] = doc["Relay"];
        event["turn"] = doc["turn"];
        String output;
        serializeJson(event, output);
        ws.textAll(output);
        return;
    }
    if (!strcmp(eventtype, "getRelayState")) {
        StaticJsonDocument<SEND_RELAIS_STATE_FOR_8_RELAIS_ARDUINO_JSON> event;
        event["eventtype"] = "getRelayState";
        JsonArray data = event.createNestedArray("data");
        for (int i = 0; i < NUMBER_OF_RELAY; i++) {
            data[i]["state"] = eventGetRelayState(i);
        }
        String output;
        serializeJson(event, output);
        client->text(output);
        return;
    } 
    if   (!strcmp(eventtype, "setTimer")) {
        Timers timer = {doc["time"], doc["Relais"], doc["turn"]};
        TimerList.add(timer);
    }
    if   (!strcmp(eventtype, "UpdateFrontend")) {
        client->text("NOT IMPLEMENTED"); // TODO IMPLEMENT
        return;
    }
    if   (!strcmp(eventtype, "deleteWlanConfig")) {
        client->text("NOT IMPLEMENTED"); // TODO IMPLEMENT
        return;
    }
    if   (!strcmp(eventtype, "setPassword")) {
        client->text("NOT IMPLEMENTED"); // TODO IMPLEMENT
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
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(err.c_str());
            return;
        }
        onMassageEvent(server, client, doc);
    }
}

void setup() {
    Serial.begin(SERIAL_SPEED);
    Serial.println("\nTRY to open filesystem");
    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    #ifdef SSID_WLAN
        WiFi.mode(WIFI_STA);
        WiFi.begin(SSID_WLAN, PASSWORD_WLAN);
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.printf("WiFi Failed!\n");
            return;
        }
    #else
        // TODO IMPLEMENT Can't connect and open file get login
    #endif
    Serial.println("Define I/O Modes");
    pinMode(SHIFT_OUT, OUTPUT);
    pinMode(SHIFT_SHIFT, OUTPUT);
    pinMode(SHIFT_OUTPUT_ENABLE, OUTPUT);
    Serial.println("IP address: ");
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
    server.on("/html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, FILE_INDEX_HTML, "text/html"); });
    
}

void loop() { 
    int listSize = TimerList.size();

   //Serial.print("There are ");
   //Serial.print(listSize);
   //Serial.print(" integers in the list. The negative ones are: ");

   // Print Negative numbers
   for (int h = 0; h < listSize; h++) {

      // Get value from list
      Timers val = TimerList.get(h);

      // If the value is negative, print it
      Serial.print(" ");
      Serial.print(val.time);
      Serial.print(" ");
      Serial.print(val.Relay);
      Serial.print(" ");
      Serial.print(val.state);
      Serial.print("\n");  
   }
    delay(1000);
}