#include "LittleFS.h"
#include "password.h"

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

const char *filename = "/exaple"; //TODO Delete fils exaple example from arduino esp

void setup() {
  Serial.begin(115200);
  Serial.println("Test");
  // testing with real pinout
  pinMode(SHIFT_OUT, OUTPUT);
  pinMode(SHIFT_SHIFT, OUTPUT);
  pinMode(SHIFT_OUTPUT_ENABLE, OUTPUT);

  pinMode(RELAIS_1_BLUE, OUTPUT);
  pinMode(RELAIS_2_BLUE, OUTPUT);
  pinMode(RELAIS_3_BLUE, OUTPUT);
  pinMode(RELAIS_4_BLUE, OUTPUT);
  pinMode(RELAIS_5_BLUE, OUTPUT);


  delay(10000);
    LittleFS.begin();
    if (!LittleFS.exists(filename)){
      File file = LittleFS.open(filename, "w");
      if (!file){
        Serial.println("could not open the file for Writing");
        return;
      }
      Serial.println("File Don't exists");    
      auto bytesWritten = file.write("This is a Test file \n");
      Serial.printf("bytes written:%d \n", bytesWritten);
      file.write("hallo \n");
      file.close();
    }
    Serial.println("Test2"); 
    LittleFS.end();
}

void loop() { 
  LittleFS.begin();
  File file = LittleFS.open(filename, "r");
  auto content = file.readString();
  Serial.println(content.c_str());
  file.close();
  LittleFS.end();
  delay(10000);
}