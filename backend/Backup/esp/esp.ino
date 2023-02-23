#include <ESP8266WiFi.h>
#include "LittleFS.h"  // TODO write dokumentation to install this plugin

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>  // TODO write dokumentation custem install per zip https://github.com/me-no-dev/ESPAsyncWebServer

#include "password.h"

//#include <NTPClient.h>
//#include <WiFiUdp.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Defined ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define NUMBER_OF_RELAIS (uint8_t)5
// Beschreibt die anzahl der
#define CONNECTIONS_ATTEMPTS 20
// Wenn esp nicht zu den Netzwerk verbiden kann nutzt es diese daten zum erstellen eines netzwerkes
#define ACCESS_POINT_SSID "D1 Mini" // TODO IF netzwerk nicht vorhaden
#define ACCESS_POINT_PASSWORD "Pass"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define WS_PROT 1337
#define HTTP_PORT 80
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// unix time (64bits)
uint64_t unix_time;
typedef struct {
  uint64_t unix_time = 0;
  uint8_t state = 0;
} Turnstate;
Turnstate timers[NUMBER_OF_RELAIS];
// Pins
#define SHIFT_OUT 16           // GPIO DO
#define SHIFT_SHIFT 5          // GPIO D1
#define SHIFT_OUTPUT_ENABLE 4  //GPIO D2
// Timer pins
#define RELAIS_1_BLUE 0   //GPIO D3
#define RELAIS_2_BLUE 2   //GPIO D4
#define RELAIS_3_BLUE 14  //GPIO D5
#define RELAIS_4_BLUE 12  //GPIO D6
#define RELAIS_5_BLUE 13  //GPIO D7
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char *ssid = SSID;
const char *password = PASSWORD;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t relais = 0b11111;
uint8_t blinking = 0b11111;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");            // access at ws://[esp ip]/ws
AsyncEventSource events("/events");  // event source (Server-Sent events)
const char *http_username = "admin";
const char *http_password = "admin";
bool shouldReboot = false;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setRelay(uint8_t t_nummber, bool state) {
  if (state)
    relais |= (1 << t_nummber);
  else
    relais &= ~(1 << t_nummber);
  Serial.print("shiftout: ");
  Serial.print(relais);
  Serial.printf("\n");
  digitalWrite(SHIFT_SHIFT, LOW);
  shiftOut(SHIFT_OUT, SHIFT_SHIFT, MSBFIRST, relais);
  digitalWrite(SHIFT_OUTPUT_ENABLE, LOW);
  digitalWrite(SHIFT_OUTPUT_ENABLE, HIGH);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void onRequest(AsyncWebServerRequest *request) {
  //Handle Unknown Request
  request->send(404, "text/plain", "Not found");
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  //Handle body
}

void sendRelaisNummber(AsyncWebSocketClient *client){
  	char str[2];  // limit to 99 relais
    sprintf(str, "%d", NUMBER_OF_RELAIS);
    String event = "{ \"event\": \"relaisnumber\", \"relais\":";
    event += str;
    event += "}";
    client->text(event);
}
void broadcastedRelaisState(uint8_t relais1) {
  char str[2];
  sprintf(str, "%d", relais & (1 << relais1));
  String event = "{ \"event\": \"relaisstate\", \"relais\":";
  event += relais1;
  event += ", \"state\":";
  event += str;
  event += "}";
  Serial.print("test");
  Serial.print(relais & (1 << relais1));
  Serial.print(relais1);
  Serial.print("dgfsg");
  Serial.print(relais);
  ws.textAll(event);
}
void sendRelaisStates(AsyncWebSocketClient *client) {
  String event = "{ \"event\": \"relaisstates\",\"data\": [";
  for (int i = 0; i < NUMBER_OF_RELAIS; i++) {
    char str[2];
    sprintf(str, "%d", i);
    event += "{\"relais\": ";
    event += str;
    event += ", \"state\":";
    sprintf(str, "%d", relais & (1 << i));
    event += str;
    if (i+1 < NUMBER_OF_RELAIS)
      event += "},";
    else
      event += "}";
  }
  event += "]}";
 client->text(event);
}
bool command(char* command, const char* chackagenst, uint8_t command_len){
  if (strlen(command) < command_len) return 0;
  if (strlen(chackagenst) < command_len) return 0; 
  for (int i = 0; i < command_len; i++){
    if (command[0] != chackagenst [0])
      return 0;
  }
  return 1;
}
void setRelais(char* data){
  uint8_t relais = data[4] - '0';
  bool state = data[5] - '0';
  if (relais > NUMBER_OF_RELAIS || state > 1) {
    return;
  } else {
    setRelay(relais, state);
  }
  broadcastedRelaisState(relais);
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    sendRelaisNummber(client);
    sendRelaisStates(client);
    return;
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
    return;
  } else if (type == WS_EVT_DATA) {
    if (len > 0 && data[len - 1] == '\0') {
      // If the data is a null-terminated string, we can just print it directly
      Serial.printf("WebSocket client #%u sent text data: %s\n", client->id(), (char *)data);
    } else {
      // If the data is not null-terminated, we need to copy it to a new buffer and add a null terminator
      char *buffer = new char[len + 1];
      memcpy(buffer, data, len);
      buffer[len] = '\0';
      if (command((char*)data, "sRel", 4)) // set Relais
        setRelais((char*)data); // gets Relais uint_8t and status uint8_t from data
      if (command((char*)data, "gRel",4)) // get Relais status
        sendRelaisStates(client);
      if (command((char*)data, "sTim",4)) // set Timer
        client->text("Not implemented"); // TODO Set timer 
      if (command((char*)data, "gTim",4)) // get Timer
        client->text("Not implemented"); // TODO Get timer 
      Serial.printf("WebSocket client #%u sent text data: %s uno \n", client->id(), buffer);
      delete[] buffer;
    }
  }
  return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("TRY to open filesystem");
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  // file expsamel ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  File file = LittleFS.open("/test_example.txt", "r");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("File Content:");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();

  Serial.println("TRY to Connect to network");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // attach AsyncWebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // attach AsyncEventSource
  server.addHandler(&events);

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    //request->send(SPIFFS, "/index.html");
    request->send(200, "text/plain", "Test"); // TODO Redidirect to webseite.com?ipaddress
    // TODO Lidel webseite version
  });


  server.onNotFound(onRequest);
  server.begin();

  pinMode(SHIFT_OUT, OUTPUT);
  pinMode(SHIFT_SHIFT, OUTPUT);
  pinMode(SHIFT_OUTPUT_ENABLE, OUTPUT);

  pinMode(RELAIS_1_BLUE, OUTPUT);
  pinMode(RELAIS_2_BLUE, OUTPUT);
  pinMode(RELAIS_3_BLUE, OUTPUT);
  pinMode(RELAIS_4_BLUE, OUTPUT);
  pinMode(RELAIS_5_BLUE, OUTPUT);
  setRelay(0, 0);
}

void loop() {
  /*
  if (relais) {
    digitalWrite(RELAIS_1_BLUE, HIGH);
    digitalWrite(RELAIS_2_BLUE, HIGH);
    digitalWrite(RELAIS_3_BLUE, HIGH);
    digitalWrite(RELAIS_4_BLUE, HIGH);
    digitalWrite(RELAIS_5_BLUE, HIGH);
    relais = 0;
  } else {
    digitalWrite(RELAIS_1_BLUE, LOW);
    digitalWrite(RELAIS_2_BLUE, LOW);
    digitalWrite(RELAIS_3_BLUE, LOW);
    digitalWrite(RELAIS_4_BLUE, LOW);
    digitalWrite(RELAIS_5_BLUE, LOW);
    relais = 1;
  }*/
  // TODO Get Unix time 
  // TODO Check if timer == Unix time 


  // TODO Filessystem
  delay(1000);
}
