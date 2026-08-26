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
  Serial1.begin(115200); 
  Serial.begin(115200); 

  tft->fillScreen(ST77XX_RED);
  delay(100);
  tft->fillScreen(ST77XX_GREEN);
  delay(100);
  tft->fillScreen(ST77XX_BLUE);
  delay(100);
  tft->fillScreen(ST77XX_BLACK);
  delay(100);
  tft->setTextWrap(true);
  tft->setCursor(0, 0);
  tft->setTextColor(ST77XX_WHITE);
  tft->setTextSize(2);
  tft->println("Darth box");
  tft->println("active!");
  //SITH = Super Intelligent Telecoms Home
  //tft->setTextSize(1);
  //tft->println("");
  //tft->println("");
  delay(2000);
  tft->fillScreen(ST77XX_BLACK);
  tft->setCursor(0, 0);
  tft->setTextSize(2);
  transmitData("Server is active!!!", "SERVER");
}


// Main loop runs repeatedly after setup
void loop() {
  cachedMessage = waitForReply();
  cachedRssi = rf95.lastRssi();

  if (Serial1.available() > 0) {
    
    // Read the incoming line up to the '\n' character
    String receivedMessage = Serial1.readStringUntil('\n');
    
    // Trim accidental whitespace or carriage returns ('\r')
    receivedMessage.trim();
    
    // Display the final string on the computer monitor
    tft->fillScreen(ST77XX_BLACK);
    tft->print(receivedMessage);
    transmitData(receivedMessage.c_str(), "SERVER");

  displayMessages();
  
  delay(100);
  }
}