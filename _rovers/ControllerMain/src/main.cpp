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
unsigned long startTime;
bool hasSentStop = false;
int selectedOption = 0;
String currentMenu = "mainMenu";

String cachedMessage = "";
int cachedRssi = 0;
// Messages in lora are one time use, once it returns the buffer is cleared and returns empty if called again
// This causes issues when doing multiple sequential reads of a single lora packet
// We cache the message so multiple functions in code can read it

#include "comms.h"
int currentRoverID = 1; // Active rover ID being controlled/queried
bool showID = false; 
unsigned long idDisplayStart = 0;
const int idDisplayDuration = 700; // ms 

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

// Initialises the Serial Monitor
void initialiseSerial() {
  Serial.begin(9600);                  // Start serial at 9600 baud
  while (!Serial) delay(1);            // Wait for Serial to be ready
  delay(100);                          // Short delay for stability
  Serial.println("Feather LoRa TX!");  // Print startup message
}

void transmitStopCommand(){
  uint32_t buttons = ss.readButtons();
  
  if (buttons == 3740 && hasSentStop == false){
    for (int i = 0; i <= 2; i++){ 
      transmitData("stop", String(currentRoverID).c_str());
      delay(20);
      transmitData("stop", String(currentRoverID).c_str());
    }
    hasSentStop = true;
    return;
  }
}

void buttonTransmit() {
  uint32_t buttons = ss.readButtons();
  uint16_t color;

  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_LEFT)) {
    color = ST77XX_WHITE;
    transmitData("left", String(currentRoverID).c_str());
    hasSentStop = false;
  }
  tft->fillTriangle(150, 30, 150, 50, 160, 40, color);

  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_RIGHT)) {
    color = ST77XX_WHITE;
    transmitData("right", String(currentRoverID).c_str());
    hasSentStop = false;
  }
  tft->fillTriangle(120, 30, 120, 50, 110, 40, color);

  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_DOWN)) {
    color = ST77XX_WHITE;
    transmitData("backward", String(currentRoverID).c_str());
    hasSentStop = false;
  }
  tft->fillTriangle(125, 26, 145, 26, 135, 16, color);

  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_UP)) {
    color = ST77XX_WHITE;
    transmitData("forward", String(currentRoverID).c_str());
    hasSentStop = false;
  }
  tft->fillTriangle(125, 53, 145, 53, 135, 63, color);

  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_A)) {
    color = ST7735_GREEN;
    transmitData("beep", String(currentRoverID).c_str());
    hasSentStop = false;
  }
  tft->fillCircle(30, 57, 10, color);

  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_B)) {
    currentMenu = "mainMenu";
    tft->fillScreen(ST77XX_BLACK);
  }

  color = ST77XX_BLACK;
  if (!(buttons & TFTWING_BUTTON_SELECT)) {
    currentRoverID++;
    if (currentRoverID > 5) currentRoverID = 1;
    color = ST77XX_RED;
    transmitData("IDCycle", String(currentRoverID).c_str());
    hasSentStop = false;
    showID = true;
    idDisplayStart = millis();
    delay(300);
  }
  tft->fillCircle(135, 40, 7, color);
}

#define ITEMS_PER_PAGE 3
#define ITEM_HEIGHT   20

const char* mainMenuItems[] = {
  "Driving",      // 0
  "Ping ID",      // 1
  "Receive",      // 2
  "Check ID",     // 3 -> Now opens ID Selection Menu
  "Auto Drive",   // 4
  "WOOOAH"        // 5
};
#define MAIN_MENU_COUNT (sizeof(mainMenuItems) / sizeof(mainMenuItems[0]))

void drawMenuItem(int menuIndex, int screenRow, bool isSelected) {
  tft->setCursor(0, screenRow * ITEM_HEIGHT);
  tft->setTextSize(2);
  tft->print(isSelected ? ">" : " ");
  tft->print(mainMenuItems[menuIndex]);
}

void mainMenuLogic(){
  uint32_t buttons = ss.readButtons();
  
  int currentPageStart = (selectedOption / ITEMS_PER_PAGE) * ITEMS_PER_PAGE;
  int currentPageEnd = currentPageStart + ITEMS_PER_PAGE;
  if (currentPageEnd > MAIN_MENU_COUNT) currentPageEnd = MAIN_MENU_COUNT;
  
  for (int i = currentPageStart; i < currentPageEnd; i++) {
    int screenRow = i - currentPageStart; 
    drawMenuItem(i, screenRow, (i == selectedOption));
  }
  
  // Scroll UP
  if (!(buttons & TFTWING_BUTTON_UP) && selectedOption > 0) {
    tft->fillScreen(ST77XX_BLACK);
    selectedOption--;
    delay(150); // slight debounce
  }
  
  // Scroll DOWN  
  if (!(buttons & TFTWING_BUTTON_DOWN) && selectedOption < MAIN_MENU_COUNT - 1) {
    tft->fillScreen(ST77XX_BLACK);
    selectedOption++;
    delay(150); // slight debounce
  }
  
  // Select option (A button)
  if (!(buttons & TFTWING_BUTTON_A)) {
    switch(selectedOption) {
      case 0: currentMenu = "driving";   tft->fillScreen(ST77XX_BLACK); break;
      case 1: currentMenu = "pingMenu";  tft->fillScreen(ST77XX_BLACK); break;              
      case 2: currentMenu = "receiver";  tft->fillScreen(ST77XX_BLACK); break;
      case 3: currentMenu = "idMenu";    tft->fillScreen(ST77XX_BLACK); break; // ID Menu
      case 4: currentMenu = "autoDrive"; tft->fillScreen(ST77XX_BLACK); break;
      default: break; 
    }
    delay(200);
  }
}

// --- NEW ID CONFIGURATION MENU LOGIC ---
void idMenuLogic() {
  uint32_t buttons = ss.readButtons();

  // Change Rover ID with Up and Down buttons
  if (!(buttons & TFTWING_BUTTON_UP)) {
    currentRoverID++;
    if (currentRoverID > 5) currentRoverID = 5; // Max ID limit
    tft->fillScreen(ST77XX_BLACK);
    delay(200); // debounce
  }

  if (!(buttons & TFTWING_BUTTON_DOWN)) {
    currentRoverID--;
    if (currentRoverID < 1) currentRoverID = 1; // Min ID limit
    tft->fillScreen(ST77XX_BLACK);
    delay(200); // debounce
  }

  // Return to Main Menu with B button
  if (!(buttons & TFTWING_BUTTON_B)) {
    tft->fillScreen(ST77XX_BLACK);
    currentMenu = "mainMenu";
    delay(200);
  }

  // Render ID menu content
  tft->setCursor(0, 0);
  tft->setTextSize(2);
  tft->setTextColor(ST77XX_WHITE);
  tft->println("Select ID:");
  tft->print("ID: ");
  tft->println(currentRoverID);
  tft->println("");
  tft->setTextSize(1);
  tft->println("UP/DOWN: Change");
  tft->println("B: Back");
}

void receiverMenuLogic(){
  uint32_t buttons = ss.readButtons();
  tft->setCursor(0, 0);
  tft->setTextSize(1);

  if(cachedMessage != ""){
    tft->fillScreen(ST77XX_BLACK);
    tft->print(cachedMessage);
    tft->println(rf95.lastRssi(), DEC);
  }
  
  if (!(buttons & TFTWING_BUTTON_B)) {
    tft->fillScreen(ST77XX_BLACK);
    currentMenu = "mainMenu";
  }
}

void autoDriveLogic(){
  tft->setCursor(0, 0);
  tft->print("Auto drive sent!");
  transmitData("autoDrive", String(currentRoverID).c_str());
  delay(1000);
  tft->fillScreen(ST77XX_BLACK);
  currentMenu = "mainMenu"; 
}

void pingMenuLogic(){
  tft->setCursor(0, 0);
  tft->setTextSize(1);
  for (int i = 0; i < 3; i++) {
    transmitData("Ping Devices", String(currentRoverID).c_str());
    if(cachedMessage != ""){
      break;
    }
    delay(200);
  }
  if(cachedMessage != ""){
    tft->print("A Device pinged back!");
    tft->print(cachedMessage);
  } else{
    tft->print("No devices in range");
  }
  delay(2000);
  tft->fillScreen(ST77XX_BLACK);
  currentMenu = "mainMenu";
}

void drawMenu(){
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - startTime;
  
  if (currentMenu == "driving"){
    if (elapsedTime >= 300){
      buttonTransmit();
      startTime = currentTime; 
    }
    transmitStopCommand();
  }
  
  if (currentMenu == "mainMenu"){
    mainMenuLogic();
  }

  if (currentMenu == "idMenu"){
    idMenuLogic();
  }

  if (currentMenu == "receiver"){
    receiverMenuLogic();
  }

  if (currentMenu == "pingMenu"){
    pingMenuLogic();
  }

  if (currentMenu == "autoDrive"){
    autoDriveLogic();
  }
}

void handleIDDisplay() {
  if (showID) {
    tft->fillRect(70, 20, 60, 80, ST77XX_BLACK);

    tft->setCursor(80, 32);
    tft->setTextColor(ST77XX_WHITE);
    tft->setTextSize(5);
    tft->print(currentRoverID);

    if (millis() - idDisplayStart > idDisplayDuration) {
      tft->fillRect(120, 5, 20, 15, ST77XX_BLACK);
      showID = false;
    }
  }
}

void checkForPings(){
  if (cachedMessage == String(currentRoverID) + ",Ping Devices"){
    transmitData("Controller", String(currentRoverID).c_str());
  }
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
  initialiseTFT();
  pinMode(LED_BUILTIN, OUTPUT);  
  Serial1.begin(115200);

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
  tft->println("Welcome To");
  tft->println("SITH OS");
  tft->setTextSize(1);
  tft->println("");
  tft->println("Super Intelligent Tele Home");
  delay(2000);
  tft->fillScreen(ST77XX_BLACK);
  tft->setCursor(0, 0);
  tft->setTextSize(2);
  Serial1.println("Feather LoRa TX!");
}

void loop() {
  cachedMessage = waitForReply();
  cachedRssi = rf95.lastRssi();

  drawMenu();
  checkForPings();
  delay(100);
}