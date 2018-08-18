/*
Scrolling Wi-Fi display
Author: Erico Mendonca <erico.mendonca @ gmail.com>
Apr/2018

Uses a Wemos D1 Mini Pro.
A change needs to be done in Arduino15/packages/esp8266/hardware/esp8266/2.4.0/cores/esp8266/Esp.cpp in order for it to work with Wemos.
Add the following line:

static const int FLASH_INT_MASK = ((B10 << 8) | B00111010);

right before this line:
bool EspClass::flashEraseSector(uint32_t sector) {

The SPIFFS filesystem must first be initialized with SPIFFS.format(). This can take a few minutes. This allows the configuration file to be read/written to.

Connections:
D5 -> CLK (clock)
D7 -> DIN (data IN)
D4 -> CS (chip select)
GND -> GND
5v -> VCC

I used a "1-button shield" on top of the Wemos for resetting/displaying configuration. It's wired to D3.

Usage:
 - Connect to the SSID with the password shown.
 - Access http://<IP> on a browser and input a message.

To change configuration:
 - Connect to the SSID with the password shown.
 - Access http://<IP>/setup and change the config information.

To reset the configuration:
 - Power up (or press the reset button on the side)
 - while "starting" is being displayed, press and hold the shield button.
 - "reset configuration" will be shown
 - connect to the new SSID with the information shown


 Have fun!
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include "FS.h"

// default values
const char *DEFAULT_SSID = "ledmatrix-WEMOS"; 
const char *DEFAULT_PASS = "led12345678";
const char *DEFAULT_MESSAGE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567890-=`~!@#$%^&*()_+[]\{}|:;\"'<>,.?/";
int DEFAULT_DELAY = 20; // draw delay in milliseconds
int DEFAULT_SPACING = 2; // default spacing between characters
int DEFAULT_BRIGHTNESS = 15; // default display brightness
const char *config_file = "/wifi.conf"; // file that holds WIFI configuration
const char *msg_file = "/message.conf"; // file that holds last message and settings

char ssid[128]="", passphrase[128]=""; // WIFI connection info

ESP8266WebServer server(80);                             // HTTP server will listen at port 80
int refresh=0;
int pinCS = 2; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int numberOfHorizontalDisplays = 8;
int numberOfVerticalDisplays = 1;
String decodedMsg;
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

int brightness=DEFAULT_BRIGHTNESS; // 0-15
int wait=DEFAULT_DELAY; // in milliseconds
int spacer = DEFAULT_SPACING; // space between chars
int width = 5 + spacer; // The font width is 5 pixels
 
char result[128];
const int buttonPin = D3;
int buttonState = 0;

// function prototypes
String getMessageForm(void);
String getSetupForm(void);
String getFooter(void);
void handle_msg(void);
void handle_setup(void);
void showMsg(void);
void setup(void);
void setupWifi(void);
void readConfig(void);
void readMessageConfig(void);
void saveConfig(void);
void saveMessageConfig(void);
void showNetInfo(void);
void resetMessage();
void resetNetInfo(void);

// Web forms sent to the client
String header =   "<h2>matrix display</h2>"
  "<span>a simple MAX7219-based display for your WEMOS D1, by docca</span>";
  
String setup_success =
  "<center>" + header +
  "<p>Configuration saved successfully!</p>"
  "</center>";

String setup_failed =
  "<center>" + header +
  "<h1>wifi setup</h1>"
  "<p>SSID and passphrase both need to be longer than 8 characters! Configuration has NOT been changed.</p>"
  "</center>";  
  
String getFooter(void)
{
  String footer = 
  "<p><center><a href='/msg?msg=" + decodedMsg + 
  "&delay=" + String(wait) + 
  "&spacing=" + String(spacer) + 
  "&brightness=" + String(brightness) + 
  "'>message</a>&nbsp;|&nbsp;<a href='/setup'>setup page</a></center></p>";
  return footer;
}

String getMessageForm(void)
{
  String formcontents =
  "<center>" + header +
  "<p><form action='msg'>"
  "<p>Message <input type='text' name='msg' value='" + decodedMsg + "' size=100 autofocus></p>"
  "<p>Delay <input type='text' name='delay' size=3 value=" + String(wait) + ">&nbsp;Spacing <input type='text' name='spacing' size=3 value=" + String(spacer) + ">&nbsp;"
  "Brightness (0-15) <input type='text' name='brightness' size=2 value=" + String(brightness) + "></p>"
  "<input type='submit' value='Submit'></p></center>"
  "</form>" + getFooter();

  return formcontents;
}

String getSetupForm(void)
{
  String formcontents = 
  "<center>" + header + 
  "<p><form action='dosetup'>"
  "<p>SSID <input type='text' name='ssid' size=32 autofocus value='" + ssid  + "'></p>"
  "<p>Passphrase <input type='text' name='passphrase'  size=32 value='" + passphrase + "'></p>"
  "<p><input type='submit' value='Submit'></p></center>"
  "</form>" + getFooter();

   return formcontents;
}

// Restores special characters that are URL-encoded to actual characters
String translateChars(String message)
{
  message.replace("+", " ");      
  message.replace("%21", "!");  
  message.replace("%22", "");  
  message.replace("%23", "#");
  message.replace("%24", "$");
  message.replace("%25", "%");  
  message.replace("%26", "&");
  message.replace("%27", "'");  
  message.replace("%28", "(");
  message.replace("%29", ")");
  message.replace("%2A", "*");
  message.replace("%2B", "+");  
  message.replace("%2C", ",");  
  message.replace("%2F", "/");   
  message.replace("%3A", ":");    
  message.replace("%3B", ";");  
  message.replace("%3C", "<");  
  message.replace("%3D", "=");  
  message.replace("%3E", ">");
  message.replace("%3F", "?");  
  message.replace("%40", "@");
  return message;
}
/*
  handles the messages coming from the webbrowser, restores a few special characters and 
  constructs the strings that can be sent to the display
*/
void handle_msg() {                      
  matrix.fillScreen(LOW);

  String msg = server.arg("msg");
  String waitvalue = server.arg("delay");
  String spacervalue = server.arg("spacing");
  String brightnessvalue = server.arg("brightness");
  decodedMsg = translateChars(msg);
  wait=waitvalue.toInt() ;
  spacer=spacervalue.toInt();
  brightness=brightnessvalue.toInt();
  matrix.setIntensity(brightness);
  
  decodedMsg = translateChars(decodedMsg);
  String form = getMessageForm();
  server.send(200, "text/html", form);    // Send same page so they can send another msg
  refresh=1;
  saveMessageConfig();
}

void handle_setup() {                      
  matrix.fillScreen(LOW);

  Serial.println("Processing setup form");
  // compare new data with old data
  
  String id = server.arg("ssid");
  String pass = server.arg("passphrase");
  if ((id.length() >= 8) && (pass.length() >= 8))
  {
    server.send(200, "text/html", setup_success + getFooter());
    strcpy(ssid, id.c_str());
    strcpy(passphrase, pass.c_str());
    Serial.printf("New SSID = %s, new passphrase = %s\n", ssid, passphrase);
    server.send(200, "text/html", setup_success);
    saveConfig();
    setupWifi();
    decodedMsg = "config saved";
  } 
  else
  {
    server.send(200, "text/html", setup_failed + getFooter());
    decodedMsg = "setup error";
    Serial.println("setup error, try again");
  }
  showMsg();
}

void showMsg()
{
  Serial.printf("showing message: %s\n", decodedMsg.c_str());
  for ( int i = 0 ; i < width * decodedMsg.length() + matrix.width() - 1 - spacer; i++ ) {
    server.handleClient();                        // checks for incoming messages
    if (refresh==1) i=0;
    refresh=0;
    matrix.fillScreen(LOW);

    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically
 
    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < decodedMsg.length() ) {
        matrix.drawChar(x, y, decodedMsg[letter], HIGH, LOW, 1);
      }

      letter--;
      x -= width;
    }
    matrix.write(); // Send bitmap to display
    delay(wait);
  }
}

void setup(void) {

  Serial.begin(115200);                           // full speed to monitor
   
  // button setup
  pinMode(buttonPin, INPUT);

  // led matrix setup
  matrix.setIntensity(DEFAULT_BRIGHTNESS); // Use a value between 0 and 15 for brightness

  // display positions
  matrix.setPosition(0, 0, 0);
  matrix.setPosition(1, 1, 0); 
  matrix.setPosition(2, 2, 0); 
  matrix.setPosition(3, 3, 0); 
  matrix.setPosition(4, 4, 0); 
  matrix.setPosition(5, 5, 0); 
  matrix.setPosition(6, 6, 0); 
  matrix.setPosition(7, 7, 0); 

  // rotation (1=90 degrees CW, 3=90 degrees CCW)
  matrix.setRotation(0, 1);
  matrix.setRotation(1, 1);
  matrix.setRotation(2, 1);
  matrix.setRotation(3, 1);
  matrix.setRotation(4, 1);
  matrix.setRotation(5, 1);
  matrix.setRotation(6, 1);
  matrix.setRotation(7, 1);
  
  decodedMsg="starting...";
  showMsg();
  
  bool result = SPIFFS.begin();
  Serial.println("SPIFFS opened: " + result);
  
  setupWifi();
  //ESP.wdtDisable();                               // used to debug, disable wachdog timer, 
  // Set up the endpoints for HTTP server,  Endpoints can be written as inline functions:
  server.on("/", []() {
    String form = getMessageForm();
    server.send(200, "text/html", form);
  });

  // Set up the endpoints for HTTP server,  Endpoints can be written as inline functions:
  server.on("/setup", []() {
    String form_setup = getSetupForm();
    server.send(200, "text/html", form_setup);
  });

  // entry points
  server.on("/msg", handle_msg);
  server.on("/dosetup", handle_setup);
  server.begin();
  Serial.println("Webserver ready!");

  // reads previously stored message, if any
  readMessageConfig();
  
}

void setupWifi()
{
  // load config from file
  readConfig();
  delay(500);
  Serial.print("Setting soft-AP ... ");
  if (WiFi.softAP(ssid, passphrase))
  {
      decodedMsg="WiFi Ready!";
      Serial.printf("Success. SSID: %s, PASS: %s\n", ssid, passphrase);
      showNetInfo();
      Serial.println(WiFi.softAPIP());
  }
  else
  {
    decodedMsg="WiFi Failed!";
    Serial.printf("WiFi Failure.\n"); 
  }
  showMsg();
}

void readConfig()
{
  memset(&ssid, 0, sizeof(ssid));
  memset(&passphrase, 0, sizeof(passphrase));
  
  // read button state, HIGH when not pressed, LOW when pressed
  buttonState = digitalRead(buttonPin);

  if (buttonState == LOW) {
      resetNetInfo();
      resetMessage();
   } 
   
  Serial.println("reading config file");
  File f = SPIFFS.open(config_file, "r");
  
  if (!f)
  {
      f.close();
      Serial.println("No configuration found, using default values.");
      memset(&ssid, 0, sizeof(ssid));
      memset(&passphrase, 0, sizeof(passphrase));
      strcpy(ssid, DEFAULT_SSID);
      strcpy(passphrase, DEFAULT_PASS);

      // create a new file
      saveConfig();
      return;
  }

  // file read okay
   while(f.available()) {
      //Lets read line by line from the file
      String line = f.readStringUntil('\n');
      
      int pos = line.indexOf('=');
      if (line.substring(0,pos) == "ssid")
      {  
          strcpy(ssid, line.substring(pos+1).c_str());
          Serial.printf("got ssid = %s\n", ssid);
      }
      if (line.substring(0,pos) == "password")
      {
          strcpy(passphrase, line.substring(pos+1).c_str());
          Serial.printf("got password = %s\n", passphrase);
      }
      Serial.println(line);
   } 
  f.close();

}

void readMessageConfig() 
{
  // deal with the message configuration file
  Serial.println("reading message config file");
  File f = SPIFFS.open(msg_file, "r");
  
  if (!f)
  {
      f.close();
      Serial.println("No message file found, creating new one.");
      memset(&decodedMsg, 0, sizeof(decodedMsg));
      decodedMsg = DEFAULT_MESSAGE;
      wait=DEFAULT_DELAY;
      brightness=DEFAULT_BRIGHTNESS;
      
      // create a new file
      saveConfig();
      return;
  }

  // file read okay
   while(f.available()) {
      //Lets read line by line from the file
      String line = f.readStringUntil('\n');
      
      int pos = line.indexOf('=');
      if (line.substring(0,pos) == "message")
      {  
          decodedMsg = line.substring(pos+1).c_str();
          Serial.printf("got message = %s\n", decodedMsg.c_str());
      } 
      else if (line.substring(0,pos) == "delay")
      {  
          wait=line.substring(pos+1).toInt();
          Serial.printf("got delay = %d\n", wait);
      }
      else if (line.substring(0,pos) == "brightness")
      {  
          brightness=line.substring(pos+1).toInt();
          Serial.printf("got brightness = %d\n", brightness);
          matrix.setIntensity(brightness);
      }
   } 
}

void saveConfig()
{
  Serial.println("Saving configuration...");

  // open the file in write mode
  File f = SPIFFS.open(config_file, "w");
  if (!f) {
    Serial.println("file creation failed");
  }
  
  // now write two lines in key/value style with  end-of-line characters
  f.printf("ssid=%s\n", ssid);
  f.printf("password=%s\n", passphrase);
  
  f.close();
  Serial.println("Saved configuration...");
  
}

void saveMessageConfig()
{
  Serial.println("Saving message configuration...");
  
  // open the file in write mode
  File f = SPIFFS.open(msg_file, "w");
  if (!f) {
    Serial.println("file creation failed");
  }
  
  // write lines in key/value style with end-of-line characters
  f.printf("message=%s\n", decodedMsg.c_str());
  f.printf("delay=%d\n", wait);
  f.printf("brightness=%d\n", brightness);
  
  f.close();
  Serial.println("Saved message configuration...");
  
}

void showNetInfo()
{
  String oldmsg=decodedMsg;

  memset(&result, 0, sizeof(result)); 
  sprintf(result, "SSID: %s PASS: %s IP:%d.%d.%d.%d", ssid, passphrase, WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
  decodedMsg=result;
  showMsg();
  decodedMsg=oldmsg;
}

void resetMessage()
{
  decodedMsg=DEFAULT_MESSAGE;
  brightness=DEFAULT_BRIGHTNESS;
  Serial.println("Message reset to the default one.");
  showMsg();
  saveMessageConfig();
}

void resetNetInfo()
{
  decodedMsg="reset configuration";
  Serial.println("Resetting configuration, using default values.");
  strcpy(ssid, DEFAULT_SSID);
  strcpy(passphrase, DEFAULT_PASS);
  showMsg();
  saveConfig();
}

void loop(void) {

   // read button state, HIGH when not pressed, LOW when pressed
  buttonState = digitalRead(buttonPin);

  if (buttonState == LOW) {
    Serial.printf("showing net info\n");
    showNetInfo();
  }
   
  showMsg();
}

