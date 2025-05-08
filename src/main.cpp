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


// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// NFC reader
Adafruit_PN532 nfc(PN532_irq, PN532_reset);

// Variables
bool Authorized = false;
const String authorizedCardsPath = "/authorized_cards";
const int relayPin = 13; // Pin for relay control

// Function declarations
void checkCardAuthorization(String cardID);
void logAccessAttempt(String cardID, bool authorized);

void setup() {
  Serial.begin(115200);
  
  // Initialize NFC reader
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1);
  }
  
  Serial.println("Found PN532 NFC reader");
  nfc.SAMConfig();
  
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

  // Initialize Firebase
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = Web_API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASS;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  Firebase.begin(&config, &auth);
  pinMode(relayPin, OUTPUT);
}

void loop() {
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  
  // Wait for NFC card
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    String cardID = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      cardID += String(uid[i], HEX);
    }
    
    Serial.print("Scanned Card ID: ");
    Serial.println(cardID);
    
    // Check if card is authorized
    checkCardAuthorization(cardID);
    
    // Small delay before next scan
    delay(1000);
  }
}

void checkCardAuthorization(String cardID) {
  if (Firebase.ready()) {
    // Check if card exists in authorized cards list
    String path = authorizedCardsPath + "/" + cardID;
    
    Serial.printf("Checking authorization for card: %s\n", cardID.c_str());
    
    if (Firebase.RTDB.get(&fbdo, path)) {
      if (fbdo.dataType() == "boolean") {
        Authorized = fbdo.to<bool>();
        if (Authorized) {
          Serial.println("Card is AUTHORIZED");
          // Add your authorized action here (e.g., unlock door)
          digitalWrite(relayPin, HIGH); // Activate relay for 1 second
          delay(200);
          digitalWrite(relayPin, LOW); // Deactivate relay
          
        } else {
          Serial.println("Card is NOT AUTHORIZED");
        }
      } else if (fbdo.dataType() == "null") {
        Serial.println("Card not found in database - NOT AUTHORIZED");
        Authorized = false;
      }
    } else {
      Serial.println("Failed to check authorization: " + fbdo.errorReason());
    }
    
    // Log the access attempt
    logAccessAttempt(cardID, Authorized);
  }
}

void logAccessAttempt(String cardID, bool authorized) {
  if (Firebase.ready()) {
    String path = "/access_logs/" + String(millis());
    
    FirebaseJson json;
    json.set("card_id", cardID);
    json.set("timestamp/.sv", "timestamp");
    json.set("authorized", authorized);
    
    if (Firebase.RTDB.pushJSON(&fbdo, path, &json)) {
      Serial.println("Access attempt logged");
    } else {
      Serial.println("Failed to log access attempt: " + fbdo.errorReason());
    }
  }
}