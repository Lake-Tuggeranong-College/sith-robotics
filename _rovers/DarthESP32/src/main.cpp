#include <Arduino.h>
#include <WiFi.h>

const char* ssid = "YOUR_NETWORK_NAME";
const char* password = "YOUR_PASSWORD";
// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX


// Set to true to enable Serial comms.
// set to false to stop all serial comms.



// Setup function runs once at startup
// ESP32 Code - Swapping the pins in software
#define ESP32_PHYSICAL_TX_TRACK 17  // Check your specific ESP32 board pinout
#define ESP32_PHYSICAL_RX_TRACK 16

void setup() {
  Serial.begin(115200);
  
  // Software trick: assign RX logic to the TX track, and TX logic to the RX track
  Serial1.begin(115200, SERIAL_8N1, ESP32_PHYSICAL_TX_TRACK, ESP32_PHYSICAL_RX_TRACK); 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}



void loop() {
  if (Serial1.available() > 0) {
    
    // Read the incoming line up to the '\n' character
    String receivedMessage = Serial1.readStringUntil('\n');
    
    // Trim accidental whitespace or carriage returns ('\r')
    receivedMessage.trim();
    
  delay(100);
  }
}