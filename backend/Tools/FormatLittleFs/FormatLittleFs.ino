#include <FS.h>
#include <LittleFS.h>

void setup() {
  Serial.begin(9600);

  LittleFS.begin();

  // Check if LittleFS is mounted
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  // Format LittleFS
  Serial.println("Formatting LittleFS...");
  LittleFS.format();

  // Verify if LittleFS is formatted
  if (!LittleFS.begin()) {
    Serial.println("Failed to format LittleFS");
    return;
  }

  Serial.println("LittleFS is formatted");
}

void loop() {
  // Do nothing
}