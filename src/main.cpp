/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp8266-nodemcu-firebase-realtime-database/
*********/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <WiFiClientSecure.h>
#include <Adafruit_PN532.h>
#include <Ticker.h>


// Network and Firebase credentials
#define WIFI_SSID "Ghajoy"
#define WIFI_PASSWORD "ghajoy2327"

#define Web_API_KEY "AIzaSyCiZ0bpKHxDFmzquUa6VuzHXn5uAlrFHVo"
#define DATABASE_URL "https://iot-doorlock-65509-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "test@test.com"
#define USER_PASS "test123"

#define PN532_irq 0
#define PN532_reset -1

void processData(AsyncResult &aResult);
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;


// variables to control the sending of data
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds
bool Authorized = false; // Flag to check if the card is authorized

// Variables to send to the Database
int intValue = 0;
float floatValue = 0.01;
String stringValue = "";

Adafruit_PN532 nfc(PN532_irq, PN532_reset);

void setup(){
  Serial.begin(115200);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1);
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); 
  Serial.print((versiondata>>24) & 0xFF, DEC);
  Serial.print("Firmware ver. "); 
  Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); 
  Serial.println((versiondata>>8) & 0xFF, DEC);

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();

  // Configure SSL client
  ssl_client.setInsecure();
  ssl_client.setTimeout(1000); // Set connection timeout
  ssl_client.setBufferSizes(4096, 1024); // Set buffer sizes

  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void loop(){
  // Maintain authentication and async tasks
  app.loop();
  Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());

  if (app.ready()){
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Convert UID to hex string
    String uidString = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) uidString += "0";
      uidString += String(uid[i], HEX);
    }
    
    
    Serial.print("Formatted UID: ");
    Serial.println(uidString); // Debug output
    
    Database.set<String>(aClient, "/cards/authorized_card/" + uidString, uidString, processData);
    delay(2000); // Delay to avoid flooding the database with requests
    }
  }
}

void processData(AsyncResult &aResult){
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}