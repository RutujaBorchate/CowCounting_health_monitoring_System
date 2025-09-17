#  Cow Counting and Health Monitoring System

This project is an **IoT-based Smart Farming Solution** built using **ESP8266, RFID, GPS, MAX30100, and Firebase**.  
It helps farmers to:
- Automatically **count cows** entering/leaving the farm using RFID tags.
- Monitor **cow health (Heart Rate & SpO₂)** using MAX30100 sensors.
- Track **real-time GPS location** of the farm/cows.
- Store and view all data on **Firebase Realtime Database**.

---

##  Features
- RFID-based cow identification.
- Heart rate and SpO₂ monitoring per cow.
- GPS location tracking.
- Firebase Realtime Database integration.
- Scalable for multiple cows (using TCA9548A I2C multiplexer).

---

## Hardware Used
- **ESP8266 NodeMCU**
- **MFRC522 RFID reader + RFID tags**
- **NEO-6M GPS module**
- **MAX30100 Pulse Oximeter (3 sensors via TCA9548A multiplexer)**
- **TCA9548A I2C multiplexer**
- Breadboard, jumper wires, 5V supply

---

## 🔧 Firmware Tools
- **Arduino IDE**
- Libraries:
  - `ESP8266WiFi.h`
  - `Firebase_ESP_Client.h`
  - `MFRC522.h`
  - `MAX30100_PulseOximeter.h`
  - `TinyGPS++.h`
