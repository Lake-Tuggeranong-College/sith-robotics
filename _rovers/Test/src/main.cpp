#include <Arduino.h>
// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX


// Set to true to enable Serial comms.
// set to false to stop all serial comms.


#define DEBUG false //This makes the device not work until you open the serial monitor, only use when you need to debug
//otherwise turn off for devices not connected to a computer

// Board Libraries:
// https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
// https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Adafruit_miniTFTWing.h"



Adafruit_miniTFTWing ss;
#define TFT_RST -1  // we use the seesaw for resetting to save a pin
#define TFT_CS 5
#define TFT_DC 6
Adafruit_ST7735 tft_7735 = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ST77xx *tft = NULL;

#include "comms.h"

String cachedMessage = "";
int cachedRssi = 0;

void initialiseTFT() {
  if (!ss.begin()) {
    if (DEBUG) {
      Serial.println("seesaw couldn't be found!");
    }
    while (1)
      ;
  }

  ss.tftReset();                          // reset the display
  ss.setBacklight(TFTWING_BACKLIGHT_ON);  // turn off the backlight
  tft_7735.initR(INITR_MINI160x80);       // initialize a ST7735S chip, mini display
  tft = &tft_7735;
  tft->setRotation(3);
    tft->fillScreen(ST77XX_BLACK);

  if (DEBUG) {
    Serial.println("TFT initialized");
  }
}

void displayMessages(){
  if(cachedMessage != ""){
    tft->setCursor(0, 0);
    tft->setTextSize(1);
    tft->fillScreen(ST77XX_BLACK);
    tft->print(cachedMessage);
    tft->println(rf95.lastRssi(), DEC);
  }
}

// Initialises the Serial Monitor
// Waits for Serial to be ready before continuing
void initialiseSerial() {
  Serial.begin(9600);                  // Start serial at 9600 baud
  while (!Serial) delay(1);            // Wait for Serial to be ready
  delay(100);                          // Short delay for stability
  Serial.println("Feather LoRa TX!");  // Print startup message
}

#define TRIG_PIN 10
#define ECHO_PIN 13
#define SPEED_OF_SOUND_CM_US 0.0343

// Setup function runs once at startup
void setup() {
  initialiseLoraPins();  // Configure LoRa module pins
  if (DEBUG) {
    initialiseSerial();
  }// Start serial communication
  resetRadio();         // Reset the LoRa radio module
  initialiseRadio();    // Initialise radio settings
  setRadioFrequency();  // Set operating frequency
  setRadioPower();      // Set transmission power
  initialiseTFT();
  pinMode(LED_BUILTIN, OUTPUT);  // Set built-in LED pin as output
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}





float getDistance3V3(int trig, int echo) {
  // 1. Clear the trigger pin
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  
  // 2. Send the 10-microsecond high pulse to start measurement
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  // 3. Measure the high pulse width on Echo pin
  // At 3.3V, the sensor's maximum reliable range is roughly 400cm.
  // A 24000us timeout handles up to ~4.1 meters perfectly.
  long duration = pulseIn(echo, HIGH, 24000); 
  
  // Return -1 if the pulse timed out or failed
  if (duration == 0) {
    return -1.0; 
  }
  
  // 4. Convert travel time to distance: (duration * speed of sound) / 2
  float distanceCm = (duration * 0.0343) / 2.0;
  
  return distanceCm;
}


// Main loop runs repeatedly after setup
void loop() {
  tft->fillScreen(ST77XX_BLACK);
  tft->setCursor(0, 0);
  tft->setTextSize(2);
  float currentDistance = getDistance3V3(TRIG_PIN, ECHO_PIN);
  tft->println(currentDistance);
  delay(250);
}