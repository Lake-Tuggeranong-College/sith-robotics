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


const int buzzer = 10;  //buzzer to arduino pin 10

// --- NEW CONSTANT DEFINITION ---
const char *ROVER_ID = "1";

#include <Wire.h>
#include "Adafruit_ADT7410.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Adafruit_miniTFTWing.h"

// Create the ADT7410 temperature sensor object
Adafruit_ADT7410 tempsensor = Adafruit_ADT7410();

// Board Libraries:
// https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
// https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json



#include "comms.h"

// For use with the onboard Neopixel (RGB LED)
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_miniTFTWing ss;
#define TFT_RST -1  // we use the seesaw for resetting to save a pin
#define TFT_CS 5
#define TFT_DC 6
Adafruit_ST7735 tft_7735 = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ST77xx *tft = NULL;
#include <Adafruit_MotorShield.h>
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *motorLeft = AFMS.getMotor(1); //testing this not final
Adafruit_DCMotor *motorBackLeft = AFMS.getMotor(2);
Adafruit_DCMotor *motorBackRight = AFMS.getMotor(3);
Adafruit_DCMotor *motorRight = AFMS.getMotor(4); //testing this not final

// Command definitions
String test_command = String(ROVER_ID) + ",test";
String forward_command = String(ROVER_ID) + ",forward";
String right_command = String(ROVER_ID) + ",right";
String start_command = String(ROVER_ID) + ",start";
String left_command = String(ROVER_ID) + ",left";
String stop_command = String(ROVER_ID) + ",stop";
String beep_command = String(ROVER_ID) + ",beep";
String backward_command = String(ROVER_ID) + ",backward";

int currentRoverID = 1;
unsigned long startTime;

void initialiseTemperatureMotionWing() {
  if (!tempsensor.begin()) {
    if (DEBUG) {
      Serial.println("Couldn't find ADT7410!");
    }
    while (1)
      ;
  }
}

void initialiseSerial() {
  Serial.begin(9600);
  while (!Serial) delay(1);
  delay(100);
  Serial.println("Feather LoRa TX Test!");
}

void initialiseBuzzer() {

  pinMode(buzzer, OUTPUT);  // Set buzzer - pin 9 as an output
}

void initialiseMotorShield() {
  if (!AFMS.begin()) {  // create with the default frequency 1.6KHz
    // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    if (DEBUG) {
      Serial.println("Could not find Motor Shield. Check wiring.");
    }
    while (1)
      ;
  }
  if (DEBUG) {
    Serial.println("Motor Shield found.");
  }
}


void transmitTemperature() {
  float c = tempsensor.readTempC();
  float f = c * 9.0 / 5.0 + 32;
  if (DEBUG) {
    Serial.print("Temp: ");
    Serial.print(c);
    Serial.print("*C\t");
    Serial.print(f);
    Serial.println("*F");
  }
  const char *roverID = "1";
  char packetBuffer[20];  // Buffer to hold the final packet
  const char *packetToTx;


  // Format the string into packetBuffer
  snprintf(packetBuffer, sizeof(packetBuffer), "%s,%.1f", roverID, c);

  // Assign the formatted string to packetToTx
  packetToTx = packetBuffer;


  transmitData(packetToTx, ROVER_ID);  // UPDATED CALL
}

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


void commandTest() {
  if (DEBUG) {
    Serial.println("Command: test");
  }
  pixels.clear();  // Set all pixel colors to 'off'
  pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  pixels.show();  // Send the updated pixel colors to the hardware.
  delay(1000);
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();  // Send the updated pixel colors to the hardware.
  delay(1000);
  pixels.setPixelColor(0, pixels.Color(0, 0, 150));
  pixels.show();  // Send the updated pixel colors to the hardware.
  delay(1000);
  pixels.clear();  // Set all pixel colors to 'off'
  pixels.show();
}

void commandForward() {
  motorLeft->run(FORWARD);
  motorRight->run(FORWARD);
  motorBackLeft->run(FORWARD); //testing this not final
  motorBackRight->run(FORWARD); //testing this not final
}

void commandBackward() {
  motorLeft->run(BACKWARD);
  motorRight->run(BACKWARD);
  motorBackLeft->run(BACKWARD); //testing this not final
  motorBackRight->run(BACKWARD); //testing this not final
}


void commandRight() {
  motorLeft->run(FORWARD);
  motorRight->run(BACKWARD);
  motorBackLeft->run(FORWARD); //testing this not final
  motorBackRight->run(BACKWARD); //testing this not final
}


void commandLeft() {
  motorLeft->run(BACKWARD);
  motorRight->run(FORWARD);
  motorBackLeft->run(BACKWARD); //testing this not final
  motorBackRight->run(FORWARD); //testing this not final
}

void commandStop() {
  // Stops the motors
  motorLeft->run(RELEASE);
  motorRight->run(RELEASE);
  motorBackLeft->run(RELEASE);
  motorBackRight->run(RELEASE);
}

void commandStart() {
  // to be implemented later.
}

void commandBeep() {

  tone(buzzer, 1000);  // Send 1KHz sound signal...
  delay(100);          // ...for 1 sec
  noTone(buzzer);      // Stop sound...
}

void buttonTransmit() {
  uint32_t buttons = ss.readButtons();

  uint16_t color;


  // 3740 is the value that is printed when no buttons are pressed, the number is changed when buttons are pressed.
  // not sure exactly what the number means yet, to be investigated

  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_LEFT)) {
    // Serial.println("LEFT");
    color = ST77XX_WHITE;
    //transmitData("left", ROVER_ID);
    
    
  }

  tft->fillTriangle(150, 30, 150, 50, 160, 40, color);


  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_RIGHT)) {
    // Serial.println("RIGHT");
    color = ST77XX_WHITE;
    //transmitData("right", ROVER_ID);
    
    
  }

  tft->fillTriangle(120, 30, 120, 50, 110, 40, color);


  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_DOWN)) {
    // Serial.println("DOWN");
    color = ST77XX_WHITE;
    //transmitData("backward", ROVER_ID);
    
    
  }

  tft->fillTriangle(125, 26, 145, 26, 135, 16, color);


  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_UP)) {
    // Serial.println("UP");
    color = ST77XX_WHITE;
    //transmitData("forward", ROVER_ID);
    
    
  }

  tft->fillTriangle(125, 53, 145, 53, 135, 63, color);


  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_A)) {
    // Serial.println("A");
    color = ST7735_GREEN;
    //transmitData("beep", ROVER_ID);
    
    
  }

  tft->fillCircle(30, 57, 10, color);


  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_B)) {
    // Serial.println("B");
    
  }


  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_SELECT)) {
    Serial.println("SELECT");
    Serial.println(currentRoverID);
    currentRoverID++;
    if (currentRoverID > 5) currentRoverID = 1;
    // color = ST77XX_RED;
    // //transmitData("IDCycle", ROVER_ID);
    // transmitData("IDCycle", String(currentRoverID).c_str());
    // hasSentStop = false;
    delay(300);


  }

  tft->fillCircle(135, 40, 7, color);
  waitForReply();
  
}

void setup() {
  initialiseLoraPins();
  if (DEBUG) {
    initialiseSerial();
  }
  resetRadio();
  initialiseRadio();
  initialiseTFT();
  setRadioFrequency();
  setRadioPower();
  initialiseMotorShield();
  initialiseBuzzer();

  pinMode(LED_BUILTIN, OUTPUT);

  // Neopixel
  // pinMode(PIN_NEOPIXEL, OUTPUT);
  pixels.begin();             // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(100);  // Set brightness (0-255)

  //set speed of motors once, this could be variable to add configurable speed
  motorLeft->setSpeed(150);
  motorRight->setSpeed(150);
  motorBackLeft->setSpeed(150); 
  motorBackRight->setSpeed(150); 

  startTime = millis();
}



void loop() {
  // readGPS();
  // transmitData("test");
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - startTime;
  buttonTransmit();
  String command = waitForReply();
  String shortReply;
  if (DEBUG) {
    Serial.println(command);
  }

  if (command == test_command) {  // UPDATED TO USE ROVER_ID
    commandTest();
    startTime = currentTime; 
  }
  else if (command == forward_command) {  // UPDATED TO USE ROVER_ID
    commandForward();
    startTime = currentTime; 
  }
  else if (command == right_command) {  // UPDATED TO USE ROVER_ID
    commandRight();
    startTime = currentTime; 
  }
  else if (command == start_command) {  // UPDATED TO USE ROVER_ID
    commandStart();
    startTime = currentTime; 
  }
  else if (command == left_command) {  // UPDATED TO USE ROVER_ID
    commandLeft();
    startTime = currentTime; 
  }
  else if (command == beep_command) {  // UPDATED TO USE ROVER_ID
    commandBeep();
    startTime = currentTime; 
  }
  else if (command == backward_command) {  // UPDATED TO USE ROVER_ID
    commandBackward();
    startTime = currentTime; 
  }
    // This is suppose to stop the rover if nothing has been recieved for 1 seconds, all checks should fail meaning no valid command up until this point
  else if (elapsedTime >= 1000){
    commandStop();
    startTime = currentTime; 
  }

  // Always check if we need to stop as a safety measure
  if (command == stop_command) {  
    commandStop();
  }


  shortReply = waitForReplyShort();
  if (shortReply == String(ROVER_ID) + ",Ping Devices"){
    transmitData("Rover", ROVER_ID);
  }

  
  // transmitTemperature();
  delay(90);
}