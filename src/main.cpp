/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp8266-nodemcu-firebase-realtime-database/
*********/

#include <Arduino.h>
#include <Wire.h>
#include <Firebase_ESP_Client.h>
#include <WiFiClientSecure.h>
#include <Adafruit_PN532.h>
#include <Ticker.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>


// Network and Firebase credentials
#define WIFI_SSID "Ghajoy"
#define WIFI_PASSWORD "ghajoy2327"

#define Web_API_KEY "AIzaSyCiZ0bpKHxDFmzquUa6VuzHXn5uAlrFHVo"
#define DATABASE_URL "https://iot-doorlock-65509-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "test@test.com"
#define USER_PASS "test123"

#define PN532_irq 0
#define PN532_reset -1
//firebase set
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;


// variables to control the sending of data
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds
bool Authorized = false; // Flag to check if the card is authorized

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
  Serial.println("Connected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.printf("FIrebase client V%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = Web_API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASS;

  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.reconnectNetwork(true);

  fbdo.setBSSLBufferSize(4096, 1024); // Set the buffer size for BSSL to 4096 bytes

  Firebase.begin(&config, &auth);
}

void loop(){
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (Firebase.ready() && success)
  {
    Serial.println("Found an NFC card!");
    Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++) {
      Serial.print(" 0x");Serial.print(uid[i], HEX);
    }
    Serial.println("");
    Serial.println("Waiting for an NFC card...");

    String cardID = "";
    for (uint8_t i=0; i < uidLength; i++) {
      cardID += String(uid[i], HEX);
    }
    Serial.print("Card ID: ");
    Serial.println(cardID);

    Firebase.RTDB.setString(&fbdo, "/test/cardID" + cardID, cardID);

    // Push an integer to a specific path
    int value = 42; // Your integer value
    Serial.printf("Push integer... %s\n", 
    Firebase.RTDB.pushInt(&fbdo, "/test/integer", value) ? "ok" : fbdo.errorReason().c_str());

    // Alternatively, you can set an integer at a specific path
    Serial.printf("Set integer... %s\n", 
      Firebase.RTDB.setInt(&fbdo, "/test/set/integer", value) ? "ok" : fbdo.errorReason().c_str());

    // Get the pushed integer back
    Serial.printf("Get pushed integer... %s\n", 
      Firebase.RTDB.getInt(&fbdo, "/test/integer/" + fbdo.pushName()) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
  }
}
