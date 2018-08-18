//**********************************************************
#include "FS.h"

String realSize = String(ESP.getFlashChipRealSize());
String ideSize = String(ESP.getFlashChipSize());
bool flashCorrectlyConfigured = realSize.equals(ideSize);

void setup() {
Serial.begin(115200);

if(flashCorrectlyConfigured){
SPIFFS.begin();
Serial.println("flash correctly configured, SPIFFS started, IDE size: " + ideSize + ", real size: " + realSize);
}
else Serial.println("flash incorrectly configured, SPIFFS cannot start, IDE size: " + ideSize + ", real size: " + realSize);

Serial.println("\nVery basic Spiffs example, writing 10 lines to SPIFFS filesystem, and then read them back");
SPIFFS.begin();

if (!SPIFFS.exists("/formatComplete.txt")) {
Serial.println("Please wait 30 secs for SPIFFS to be formatted");
SPIFFS.format();
//delay(30000);
Serial.println("Spiffs formatted");

File f = SPIFFS.open("/formatComplete.txt", "w");
if (!f) {
    Serial.println("file open failed");
} else {
    f.println("Format Complete");
}
} else {
Serial.println("SPIFFS is formatted. Moving along...");
}
}

void loop() {
// open file for writing
File f = SPIFFS.open("/f.txt", "w");
if (!f) {
Serial.println("file open failed");
}
Serial.println("====== Writing to SPIFFS file =========");
// write 10 strings to file
for (int i=1; i<=10; i++){
f.print("Millis() : ");
f.println(millis());
Serial.println(millis());
}

f.close();

// open file for reading
f = SPIFFS.open("/f.txt", "r");
if (!f) {
Serial.println("file open failed");
} Serial.println("====== Reading from SPIFFS file =======");
// write 10 strings to file
for (int i=1; i<=10; i++){
String s=f.readStringUntil('\n');
Serial.print(i);
Serial.print(":");
Serial.println(s);
}
// wait a few seconds before doing it all over again
delay(3000);

}
//*************************************************************************
