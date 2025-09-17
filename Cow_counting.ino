#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// ---------------- WiFi & Firebase ----------------
#define WIFI_SSID "Prachi"
#define WIFI_PASSWORD "1234abcd"
#define API_KEY "AIzaSyD0XD1T7KiPygPBBTB9paDqI2V8Gdq6OeI"
#define DATABASE_URL "https://real-time-data-8b406-default-rtdb.asia-southeast1.firebasedatabase.app/"

// ---------------- RFID ----------------
#define SS_PIN 15  // D8
#define RST_PIN 0  // D3
MFRC522 rfid(SS_PIN, RST_PIN);

// ---------------- GPS ----------------
#define GPS_RX 4  // D2
#define GPS_TX 5  // D1
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

// ---------------- Firebase ----------------
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ---------------- MAX30100 ----------------
PulseOximeter pox;   // One object, reused per channel
#define TCA9548A_ADDR 0x70  // Multiplexer address

// ---------------- RFID UIDs ----------------
String cow1UID = "6A6A3D81";
String cow2UID = "B8744212";
String cow3UID = "700EE921";

// ---------------- Cow status ----------------
bool cow1In = false, cow2In = false, cow3In = false;
int cowsInCount = 0;

// ---------------- Timers ----------------
uint32_t tsLastReport = 0;
bool signupOK = false;

// ---------------- Multiplexer Function ----------------
void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(2);
}

// ---------------- Beat Detection ----------------
void onBeatDetected() {
  Serial.println("Beat detected!");
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  Wire.begin();

  // WiFi setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
  Serial.println("\nConnected, IP: " + WiFi.localIP().toString());

  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase SignUp OK");
    signupOK = true;
  } else {
    Serial.printf("SignUp Error: %s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // RFID setup
  SPI.begin();
  rfid.PCD_Init();

  // Test multiplexer + MAX30100 init
  for (int ch = 0; ch < 3; ch++) {
    tcaSelect(ch);
    if (!pox.begin()) {
      Serial.printf("MAX30100 on channel %d FAILED\n", ch);
    } else {
      Serial.printf("MAX30100 on channel %d SUCCESS\n", ch);
    }
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }
}

void loop() {
  // Check RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String tagUID = getTagUID(rfid.uid.uidByte, rfid.uid.size);
    Serial.println("RFID UID: " + tagUID);

    if (tagUID == cow1UID) { cow1In = !cow1In; updateCowStatus(cow1In, "Cow1"); }
    else if (tagUID == cow2UID) { cow2In = !cow2In; updateCowStatus(cow2In, "Cow2"); }
    else if (tagUID == cow3UID) { cow3In = !cow3In; updateCowStatus(cow3In, "Cow3"); }

    delay(200);
  }

  // GPS reading
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        updateGPSLocation();
      }
    }
  }

  // Periodic sensor updates
  if (millis() - tsLastReport > 3000) { // every 3s
    if (Firebase.ready() && signupOK) {
      // Cow1 -> Multiplexer channel 0
      tcaSelect(0);
      pox.update();
      float hr1 = pox.getHeartRate();
      float spo2_1 = pox.getSpO2();
      Firebase.RTDB.setFloat(&fbdo, "/CowData/Cow1/HeartRate", hr1);
      Firebase.RTDB.setFloat(&fbdo, "/CowData/Cow1/SpO2", spo2_1);

      // Cow2 -> channel 1
      tcaSelect(1);
      pox.update();
      float hr2 = pox.getHeartRate();
      float spo2_2 = pox.getSpO2();
      Firebase.RTDB.setFloat(&fbdo, "/CowData/Cow2/HeartRate", hr2);
      Firebase.RTDB.setFloat(&fbdo, "/CowData/Cow2/SpO2", spo2_2);

      // Cow3 -> channel 2
      tcaSelect(2);
      pox.update();
      float hr3 = pox.getHeartRate();
      float spo2_3 = pox.getSpO2();
      Firebase.RTDB.setFloat(&fbdo, "/CowData/Cow3/HeartRate", hr3);
      Firebase.RTDB.setFloat(&fbdo, "/CowData/Cow3/SpO2", spo2_3);

      Serial.printf("Cow1 HR=%.1f SpO2=%.1f | Cow2 HR=%.1f SpO2=%.1f | Cow3 HR=%.1f SpO2=%.1f\n",
                    hr1, spo2_1, hr2, spo2_2, hr3, spo2_3);

      Firebase.RTDB.setInt(&fbdo, "/CowCount/CowsIn", cowsInCount);
      updateCowList();
    }
    tsLastReport = millis();
  }
}

// ---------------- Helper Functions ----------------
void updateCowStatus(bool isIn, const String& cowName) {
  if (isIn) cowsInCount++; else cowsInCount--;
  Firebase.RTDB.setString(&fbdo, "/CowData/" + cowName + "/Status", isIn ? "In" : "Out");
}

void updateCowList() {
  String cowsInList = "";
  if (cow1In) cowsInList += "Cow1, ";
  if (cow2In) cowsInList += "Cow2, ";
  if (cow3In) cowsInList += "Cow3, ";
  if (cowsInList.length() > 0) cowsInList.remove(cowsInList.length() - 2);
  Firebase.RTDB.setString(&fbdo, "/CowCount/CowsInList", cowsInList);
}

String getTagUID(byte *uid, byte size) {
  String uidStr = "";
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
  }
  uidStr.toUpperCase();
  return uidStr;
}

void updateGPSLocation() {
  if (Firebase.ready() && signupOK) {
    FirebaseJson json;
    json.set("latitude", gps.location.lat(), 6);
    json.set("longitude", gps.location.lng(), 6);
    Firebase.RTDB.setJSON(&fbdo, "/GPSLocation", &json);
    Serial.printf("GPS: Lat %.6f, Lng %.6f\n", gps.location.lat(), gps.location.lng());
  }
}
