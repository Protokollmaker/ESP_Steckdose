#define HOSTNAME "steckerleiste"

#define RELAY_NUMBER 5
#define TIMER_NUMBER RELAY_NUMBER
#define BLINK_LEDS {0,2,14,12,13}
#define RELAY_PIN 4

#define URL_INDEX_HTML  "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backendFor1Relais/data/V2.0/half1index.html"
#define URL_INDEX_CSS   "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backendFor1Relais/data/V2.0/style.css"
#define URL_INDEX_JS1    "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backendFor1Relais/data/V2.0/half1index.js"
#define URL_INDEX_JS2    "https://raw.githubusercontent.com/Protokollmaker/ESP_Steckdose/master/backendFor1Relais/data/V2.0/half2index.js"
#define FILE_INDEX_HTML "/index.html"
#define FILE_INDEX_CSS  "/style.css"
#define FILE_INDEX_JS   "/index.js"

#define LOG_DOWNLADE(URL, FILE) Serial.print("[Downlade File] start Downladeing html "); \
                                Serial.print(FILE); \
                                Serial.print(" from "); \
                                Serial.println(URL);
