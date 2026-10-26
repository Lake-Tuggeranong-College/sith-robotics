#include <Arduino.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include <Wire.h>
#include "Adafruit_ADT7410.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Adafruit_miniTFTWing.h"

// Set to true to enable Serial comms.
// set to false to stop all serial comms.
#define DEBUG false 

// --- ROVER ID DEFINITION ---
int ROVER_ID = 1;

// Create the ADT7410 temperature sensor object
Adafruit_ADT7410 tempsensor = Adafruit_ADT7410();

#include "comms.h"

// For use with the onboard Neopixel (RGB LED)
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

#include <Adafruit_MotorShield.h>
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *motorLeft = AFMS.getMotor(1); 
Adafruit_DCMotor *motorBackLeft = AFMS.getMotor(2);
Adafruit_DCMotor *motorBackRight = AFMS.getMotor(3);
Adafruit_DCMotor *motorRight = AFMS.getMotor(4); 

// TFT Wing Objects & Pins
Adafruit_miniTFTWing ss;
#define TFT_RST -1  // Uses seesaw for reset
#define TFT_CS 5
#define TFT_DC 6
Adafruit_ST7735 tft_7735 = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ST77xx *tft = NULL;

String cachedMessage = "";
int cachedRssi = 0;
bool currentlyRoaming = false;

#define TRIG_PIN 10
#define ECHO_PIN 13

unsigned long startTime;

void initialiseTemperatureMotionWing() {
  if (!tempsensor.begin()) {
    if (DEBUG) {
      Serial.println("Couldn't find ADT7410!");
    }
    while (1);
  }
}

void initialiseTFT() {
  if (!ss.begin()) {
    if (DEBUG) {
      Serial.println("seesaw couldn't be found!");
    }
    while (1);
  }

  ss.tftReset();                          // Reset the display
  ss.setBacklight(TFTWING_BACKLIGHT_ON);  // Turn on backlight
  tft_7735.initR(INITR_MINI160x80);       // Initialize ST7735S mini display
  tft = &tft_7735;
  tft->setRotation(3);                    // Landscape orientation
  tft->fillScreen(ST77XX_BLACK);

  if (DEBUG) {
    Serial.println("TFT initialized");
  }
}

void initialiseSerial() {
  Serial.begin(9600);
  while (!Serial) delay(1);
  delay(100);
  Serial.println("Feather LoRa TX Test!");
}

void initialiseMotorShield() {
  if (!AFMS.begin()) {  
    if (DEBUG) {
      Serial.println("Could not find Motor Shield. Check wiring.");
    }
    while (1);
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
  
  // Convert ROVER_ID to char string dynamically
  char roverIDStr[10];
  snprintf(roverIDStr, sizeof(roverIDStr), "%d", ROVER_ID);

  char packetBuffer[20];  // Buffer to hold the final packet
  snprintf(packetBuffer, sizeof(packetBuffer), "%s,%.1f", roverIDStr, c);

  transmitData(packetBuffer, String(ROVER_ID).c_str());  
}

char test_command[16];
char forward_command[16];
char right_command[16];
char start_command[16];
char left_command[16];
char stop_command[16];
char beep_command[16];
char backward_command[16];
char autonomous_command[16];

// Helper function to update commands
void updateCommands() {
  snprintf(test_command, sizeof(test_command), "%d,test", ROVER_ID);
  snprintf(forward_command, sizeof(forward_command), "%d,forward", ROVER_ID);
  snprintf(right_command, sizeof(right_command), "%d,right", ROVER_ID);
  snprintf(start_command, sizeof(start_command), "%d,start", ROVER_ID);
  snprintf(left_command, sizeof(left_command), "%d,left", ROVER_ID);
  snprintf(stop_command, sizeof(stop_command), "%d,stop", ROVER_ID);
  snprintf(beep_command, sizeof(beep_command), "%d,beep", ROVER_ID);
  snprintf(backward_command, sizeof(backward_command), "%d,backward", ROVER_ID);
  snprintf(autonomous_command, sizeof(autonomous_command), "%d,autoDrive", ROVER_ID);
}

void commandTest() {
  if (DEBUG) {
    Serial.println("Command: test");
  }
  pixels.clear();  
  pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  pixels.show();  
  delay(1000);
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();  
  delay(1000);
  pixels.setPixelColor(0, pixels.Color(0, 0, 150));
  pixels.show();  
  delay(1000);
  pixels.clear();  
  pixels.show();
}

void commandForward() {
  motorLeft->run(FORWARD);
  delay(30); // Stagger start to reduce current spike
  motorBackLeft->run(FORWARD); 
  delay(30);
  motorRight->run(FORWARD);
  delay(30);
  motorBackRight->run(FORWARD); 
}

void commandBackward() {
  motorLeft->run(BACKWARD);
  delay(30);
  motorBackLeft->run(BACKWARD); 
  delay(30);
  motorRight->run(BACKWARD);
  delay(30);
  motorBackRight->run(BACKWARD); 
}

void commandRight() {
  motorLeft->run(FORWARD);
  delay(30);
  motorBackLeft->run(FORWARD); 
  delay(30);
  motorRight->run(BACKWARD); // Right side goes backward to turn
  delay(30);
  motorBackRight->run(BACKWARD); 
}

void commandLeft() {
  motorLeft->run(BACKWARD);
  delay(30);
  motorBackLeft->run(BACKWARD); 
  delay(30);
  motorRight->run(FORWARD);
  delay(30);
  motorBackRight->run(FORWARD); 
}

void commandStop() {
  motorLeft->run(RELEASE);
  motorRight->run(RELEASE);
  motorBackLeft->run(RELEASE);
  motorBackRight->run(RELEASE);
}

void commandStart() {
  // to be implemented later.
}

float getDistance3V3(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  long duration = pulseIn(echo, HIGH, 24000); 
  
  if (duration == 0) {
    return -1.0; 
  }
  
  float distanceCm = (duration * 0.0343) / 2.0;
  return distanceCm;
}

void roamTick(){
  commandStop();
  delay(100);
  float currentDistance = getDistance3V3(TRIG_PIN, ECHO_PIN);
  
  if (currentDistance > 0.0 && currentDistance <= 12.0){
    long randomDir = random(2);
    if (randomDir == 1){
      commandLeft();
      delay(4000);
    } else {
      commandRight();
      delay(4000);
    }
    commandForward();
  }
  
  commandForward();
  delay(400);
}

void drawScreen(){


  tft->fillScreen(ST77XX_BLACK);
  tft->setCursor(0, 0);
  tft->print("Rover ID: ");
  tft->print(String(ROVER_ID));
  tft->println("");
  tft->print("Auto Roam: ");
  tft->print(String(currentlyRoaming));
}

void setup() {
  initialiseLoraPins();
  if (DEBUG) {
    initialiseSerial();
  }
  resetRadio();
  initialiseRadio();
  setRadioFrequency();
  setRadioPower();
  initialiseMotorShield();
  initialiseTFT();

  pinMode(LED_BUILTIN, OUTPUT);
  pixels.begin();             
  pixels.setBrightness(100);  

  motorLeft->setSpeed(125);
  motorRight->setSpeed(125);
  motorBackLeft->setSpeed(125); 
  motorBackRight->setSpeed(125); 
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  tft->fillScreen(ST77XX_BLACK);
  tft->setCursor(0, 0);
  tft->setTextColor(ST77XX_WHITE);
  tft->setTextSize(2);
  
  startTime = millis();
  tft->fillScreen(ST77XX_BLACK);
  tft->setCursor(0, 0);
  drawScreen();
}

void loop() {
  cachedMessage = waitForReply();
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - startTime;

  if (DEBUG) {
    Serial.println(cachedMessage);
  }


  if (cachedMessage == String(ROVER_ID) + ",Ping Devices"){
    transmitData("Rover", String(ROVER_ID).c_str());
  }

  if (cachedMessage == autonomous_command) {  
    currentlyRoaming = !currentlyRoaming;
    cachedMessage = "";
  }
  uint32_t buttons = ss.readButtons();

  // Debounce delay can be added if buttons are too sensitive
  if (!(buttons & TFTWING_BUTTON_UP)) {
    ROVER_ID += 1;
    updateCommands(); // Update the command strings
    drawScreen();
    delay(200); 
  }  

  if (!(buttons & TFTWING_BUTTON_DOWN)) {
    ROVER_ID -= 1;
    updateCommands(); // Update the command strings
    drawScreen();
    delay(200);
  }   
  

  if (currentlyRoaming == true){
    roamTick();
    startTime = currentTime; 
    return;
  }

  if (cachedMessage == test_command) {  
    commandTest();
    startTime = currentTime; 
  }
  else if (cachedMessage == forward_command) {  
    commandForward();
    startTime = currentTime; 
  }
  else if (cachedMessage == right_command) {  
    commandRight();
    startTime = currentTime; 
  }
  else if (cachedMessage == start_command) {  
    commandStart();
    startTime = currentTime; 
  }
  else if (cachedMessage == left_command) {  
    commandLeft();
    startTime = currentTime; 
  }
  else if (cachedMessage == backward_command) {  
    commandBackward();
    startTime = currentTime; 
  }
  else if (elapsedTime >= 5000){
    commandStop();
    startTime = currentTime; 
  }

  if (cachedMessage == stop_command) {  
    commandStop();
  }

  // transmitTemperature();
  delay(100);
}
