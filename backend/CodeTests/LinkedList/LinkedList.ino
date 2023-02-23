//#include <ArduinoJson.h>
//#include <ArduinoSTL.h>

//#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h> 
namespace Name {
  #include <LinkedList.h>
}
#include "LittleFS.h"

struct Timers {
    unsigned long time = 0;
    int Relay = 0;
    bool state = 0;
};

Name::LinkedList<Timers> TimerList = Name::LinkedList<Timers>();

void setup() {
    Serial.begin(115200);
    Serial.println("Hello!");
    Timers timer = {0,0,0};
    TimerList.add(timer);
    Timers timeer = {100,4,true};
    TimerList.add(timeer);
    Timers timreer = {200,1,false};
    TimerList.add(timreer);
}

void loop() { 
    int listSize = TimerList.size();

   Serial.print("There are ");
   Serial.print(listSize);
   Serial.print(" integers in the list. The negative ones are: ");

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

   while (true) {delay(1000);} // nothing else to do, loop forever
}