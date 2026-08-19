/*
  comms.h - Non-Blocking Communication Protocol Implementation
  Version: 2.0 (Non-Blocking)
*/

#include <RH_RF95.h>

#define RFM95_CS 16
#define RFM95_INT 21
#define RFM95_RST 17
#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Volatile flag set by hardware interrupt when packet arrives
volatile bool packetReceived = false;

// Interrupt Service Routine - runs immediately when DIO0 goes high
void IRAM_ATTR onLoraPacket() {
  packetReceived = true;
}

void initialiseLoraPins() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  // Attach interrupt for non-blocking receive
  attachInterrupt(digitalPinToInterrupt(RFM95_INT), onLoraPacket, RISING);
}

void resetRadio() {
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
}

void initialiseRadio() {
  while (!rf95.init()) {
    if (DEBUG) Serial.println("LoRa radio init failed");
    while (1);
  }
  if (DEBUG) Serial.println("LoRa radio init OK!");
}

void setRadioFrequency() {
  if (!rf95.setFrequency(RF95_FREQ)) {
    if (DEBUG) Serial.println("setFrequency failed");
    while (1);
  }
}

void setRadioPower() {
  rf95.setTxPower(5, false);
}

// NON-BLOCKING: Returns received string or empty string if nothing available
String checkForMessage() {
  // Only process if interrupt fired OR radio says data is ready
  if (!packetReceived && !rf95.available()) return "";
  
  packetReceived = false; // Reset flag
  
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  if (rf95.recv(buf, &len)) {
    if (DEBUG) {
      Serial.print("RX: ");
      Serial.println((char*)buf);
    }
    return String((char*)buf);
  }
  return "";
}

// Kept for backward compatibility but now non-blocking
// Returns message if available, otherwise returns "No Reply" immediately
String waitForReply() {
  String msg = checkForMessage();
  if (msg.length() > 0) return msg;
  return "No Reply";
}

// Short timeout version - now also non-blocking for seamless integration
String waitForReplyShort() {
  return checkForMessage(); 
}

void transmitData(const char* radioPacket, const char* roverID) {
  digitalWrite(LED_BUILTIN, HIGH);
  
  char packetToTx[strlen(roverID) + strlen(radioPacket) + 2];
  strcpy(packetToTx, roverID);
  strcat(packetToTx, ",");
  strcat(packetToTx, radioPacket);

  // Clear any stale RX flag before transmitting
  packetReceived = false;
  
  rf95.send((uint8_t*)packetToTx, strlen(packetToTx) + 1);
  rf95.waitPacketSent(); // TX must block to ensure completion
  
  digitalWrite(LED_BUILTIN, LOW);
}
