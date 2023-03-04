//#define SSID "URE ssid"
//#define PASSWORD "URE password"
#include "password.h"
#ifndef SSID_WLAN
    #define ACCESS_POINT_SSID "D1 Mini"
    #define ACCESS_POINT_PASSWORD "Pass"
    #define FILE_WLAN_PASSWORD "env.wlan"
#endif
#define NUMBER_OF_RELAY 5
#define NUMBER_OF_TIMERS 20
// Define PINS ///////////////////////////////////////////////////////////////////////////////////////////////////
#define BLINK_LEDS {0,2,14,12,13}
//#define SHIFT_OUT 16
//#define SHIFT_CLOCK 5
//#define SHIFT_OUTPUT_ENABLE 4

#define SHIFT_OUT 16           // GPIO DO
#define SHIFT_SHIFT 5          // GPIO D1
#define SHIFT_OUTPUT_ENABLE 4  //GPIO D2
// Webserver /////////////////////////////////////////////////////////////////////////////////////////////////////
#define WSERVER_PORT 80
#define WSOCKET_ACCSESS "/ws"
#define WEBSOCKET_EVENTS "/events"
// Relais Startup state //////////////////////////////////////////////////////////////////////////////////////////
// every bit is 1 Relais LSB is the fist relay
#define RELAY_STARTUP 0b0
// Get Files /////////////////////////////////////////////////////////////////////////////////////////////////////
// ! Delete test file later
#define URL_TEST "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backend/data/test_example.txt"
#define FILE_TEST "/www/example.txt"
// TODO create index etz file
#define URL_INDEX_HTML  "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backend/data/indexPage/index.html"
#define URL_INDEX_CSS   "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backend/data/indexPage/style.css"
#define URL_INDEX_JS    "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backend/data/indexPage/index.js"
#define FILE_INDEX_HTML "/index.html"
#define FILE_INDEX_CSS  "/style.css"
#define FILE_INDEX_JS   "/index.js"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SERIAL_SPEED 115200
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do not touch /////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// edit values https://arduinojson.org/v6/assistant/#/step1
#define LAGES_SOCKET_EVENT_FOR_ARDURINO_JSON 200
#define LOG_DOWNLADE(URL, FILE) Serial.print("[Downlade File] start Downladeing html"); \
                                Serial.print(FILE); \
                                Serial.print(" from "); \
                                Serial.println(URL);