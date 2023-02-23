#include<ArduinoJson.h>

void setup() {
    Serial.begin(115200);
    char json[] = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, json);

    const char* sensor = doc["sensor"];
    long time          = doc["time"];
    double latitude    = doc["data"][0];
    double longitude   = doc["data"][1];
    Serial.print(sensor);
    Serial.print(time, HEX);    
    Serial.print(latitude, HEX);  
}

void loop() { 

}