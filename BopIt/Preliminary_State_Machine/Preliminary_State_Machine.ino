// INCLUDES
#include <Wire.h>             // For I2C communication devices
#include <SPI.h>              // For SPI communication devices
#include <Adafruit_GFX.h>     // For graphics on OLED
#include <Adafruit_SSD1306.h> // For the OLED itself
#include <FastLED.h>          // For the LED Light Strip
#include <FastIMU.h>          // For Grabbing IMU data off I2C
#include "mp3tf16p.h"         // For the DFPlayer

// COLOR DEFINITIONS
// #define WHITE_25 0x202020
// #define WHITE_50 0x303030
// #define WHITE_MAX 0xFFFFFF
// #define RED_50 0x000030
// #define RED_MAX 0x0000FF
// #define BLUE_50 0x300000
// #define BLUE_MAX 0xFF0000

// OLED CONFIGURATION
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_MOSI  11     // SPI MOSI
#define OLED_CLK   13     // SPI SCK
#define OLED_DC    8      // Data/Command
#define OLED_CS    10     // Chip Select
#define OLED_RESET 9      // Reset (-1 if sharing arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// LED STRIP CONFIGURATION
#define NUM_LEDS 144
#define DATA_PIN 6
CRGB leds[NUM_LEDS];

// IMU CONFIGURATION
#define IMU_ADDRESS 0x68
MPU6050 imu;
const calData calibrationData = {
    true,                     // valid flag
    {0.091, 0.001, 0.006},    // accelBias
    {-0.313, -0.412, -1.443}, // gyroBias
    {0.0, 0.0, 0.0},          // magBias (unused for MPU6050)
    {1.0, 1.0, 1.0}           // magScale (unused for MPU6050)
};
AccelData a;                  // accelerometer data
GyroData g;                   // gyro data

// PIN DEFINITIONS
#define POWER_PIN 2      // Toggle switch
#define START_PIN 7      // SW1 
#define FIREBALL_PIN 4   // SW2

// Initialize DFPlayer
// MP3Player mp3(10, 11);  // Using the custom MP3Player class

// GAME STATES
enum GameState {
  MENU_STATE,
  START_STATE,
  ACTION_SELECT_STATE,
  FIREBALL_STATE,
  SWING_STATE,
  THRUST_STATE,
};

// GAME VARIABLES
GameState currentState = MENU_STATE;
int health = 3;
int level = 0;
unsigned long actionTime = 0;
unsigned long prev_currentTime = 0;
unsigned long ACTION_TIMEOUT = 5000;    // 5 seconds in milliseconds
unsigned long total_actions = 1;
unsigned long action_count = 0;
float decay_rate = 0.75;
const unsigned long STATE_POLLING_TIME = 100; // Poll every 0.01 seconds
bool ACTION_SUCCESS = false;                  // Whether seelected action was performed
float SWING_THRESHOLD = 2.5;                  // aZ
float THRUST_THRESHOLD = 1.5;                 // aY
float ORIENT_THRESHOLD = 30;                  // gX/gY/gZ

void setup() {
  // Initialize Serial for debugging
  Serial.begin(9600);
  while (!Serial) {
    delay(10);  // wait for serial port to connect
  }
  
  //Initialize I2C
  Wire.begin();
  
  // Initialize pins
  pinMode(POWER_PIN, INPUT);
  pinMode(START_PIN, INPUT);
  pinMode(FIREBALL_PIN, INPUT);
  
  // Initialize LED strip
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  // Initialize MP3 player
  // mp3.initialize();
  
  // Initialize OLED display with SPI
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Initializing..."));
  display.display();
  delay(2000);

  // Initialize IMU
  int err = imu.init(calibrationData, IMU_ADDRESS);
  if (err != 0) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(F("IMU Error: "));
    display.println(err);
    display.display();
    Serial.print("IMU Error: "); Serial.println(err);
    for(;;);
  }

  // Set Accelerometer Range (after calibration)
  err = imu.setAccelRange(8); // +/- 8g Range
  if (err != 0) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("Accel range err"));
    display.display();
    for(;;);
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("Setup complete!"));
  display.display();
  delay(1000);
}

void updateDisplay(bool threshold, AccelData accel, GyroData gyro) {
  display.clearDisplay();
  
  // ROW 1 OLED OUTPUT
  display.setCursor(0,0);
  if (currentState == MENU_STATE) display.println(F("MENU"));
  else if (currentState == START_STATE) display.println(F("START"));
  else if (currentState == ACTION_SELECT_STATE) display.println(F("ACTION SELECT"));
  else if (currentState == FIREBALL_STATE) display.println(F("FIREBALL!"));
  else if (currentState == THRUST_STATE) display.println(F("THRUST!"));
  else if (currentState == SWING_STATE) display.println(F("SWING!")); 
  else display.println(F("INVALID STATE!"));

  // ROW 2 + 3 + 4 OLED OUTPUT
  display.print(F("Level: ")); display.println(level);
  display.print(F("Health: ")); display.println(health);
  if (threshold) display.println(F("THRESHOLD MET"));
  else display.println(F("THRESHOLD NOT MET"));
  delay(5);
  // if (currentState != SWING_STATE && currentState != THRUST_STATE && currentState != FIREBALL_STATE) {
  //   display.print(F("Level: ")); display.println(level);
  //   display.print(F("Health: ")); display.println(health);
  // } else {
  //   // DISPLAY IMU DATA
  //   // display.setCursor(0, 8);
  //   // display.print(accel.accelX); display.print(F(", "));
  //   // display.print(accel.accelZ); display.print(F(", "));
  //   // display.println(accel.accelZ);

  //   // // display.setCursor(0, 16);
  //   // display.print(gyro.gyroX); display.print(F(", "));
  //   // display.print(gyro.gyroZ); display.print(F(", "));
  //   // display.println(gyro.gyroZ);

  //   // ROW 4 OUTPUT
  //   // display.setCursor(0, 24);
  //   if (threshold) display.println(F("THRESHOLD MET"));
  //   else display.println(F("THRESHOLD NOT MET"));
  //   delay(5);
  // }

  display.display();
}

void showSuccessPattern(CRGB color) {
  const int LED = 20;
  for (int i = -LED; i < NUM_LEDS; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      if (j >= i && j < i + LED + 1) {
        leds[j] = color;
      } else {
        leds[j] = CRGB::Black;
      }
    }
    FastLED.show();
    delay(5);
  }
}

void showFailurePattern() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 4 == 0) leds[i] = 0x080808;
    else leds[i] = 0x000000;
  }
  FastLED.show();
  delay(5);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 4 == 0) leds[i] = 0x202020;
    else leds[i] = 0x000000;
  }
  FastLED.show();
  delay(5);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 4 == 0) leds[i] = 0x404040;
    else leds[i] = 0x000000;
  }
  FastLED.show();
  delay(5);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 4 == 0) leds[i] = 0x808080;
    else leds[i] = 0x000000;
  }
  FastLED.show();
  delay(5);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 4 == 0) leds[i] = 0x404040;
    else leds[i] = 0x000000;
  }
  FastLED.show();
  delay(5);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 4 == 0) leds[i] = 0x202020;
    else leds[i] = 0x000000;
  }
  FastLED.show();
  delay(5);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 4 == 0) leds[i] = 0x080808;
    else leds[i] = 0x000000;
  }
  FastLED.show();
  delay(5);
}

bool checkFireballThreshold(GyroData gData) {
  if (abs(gData.gyroX) < ORIENT_THRESHOLD 
      && abs(gData.gyroY) < ORIENT_THRESHOLD
      && abs(gData.gyroZ) < ORIENT_THRESHOLD){
    return true;
  } else {
    return false;
  }
}

void updateDifficulty() {
  if (action_count >= total_actions) {
    level++;
    total_actions++;
    action_count = 0;
    ACTION_TIMEOUT = (unsigned long) ACTION_TIMEOUT * decay_rate;
  }
}

void loop() {
  // start event timers
  unsigned long currentTime = millis();
  bool thresh_met = false;

  if (currentTime - prev_currentTime >= STATE_POLLING_TIME) {
    // Check power toggle at all times
    if (digitalRead(POWER_PIN) == LOW) {
      currentState = MENU_STATE;
    }

    if (currentState == MENU_STATE) {
      FastLED.clear();
      FastLED.show();
      updateDisplay(thresh_met, a, g);
      // mp3.playTrackNumber(0, 30);  // Play menu sound
      while (digitalRead(POWER_PIN) == LOW) {
        delay(100);  // Wait for toggle to be set to HIGH
      }
      currentState = START_STATE;
    
    } else if (currentState == START_STATE) {
      health = 3;
      level = 0;
      action_count = 0;
      total_actions = 1;
      updateDisplay(thresh_met, a, g);
      while(digitalRead(START_PIN) == LOW) {
        // mp3.playTrackNumber(1, 30);  // Play start sound
        delay(100);  // Debounce
      }
      currentState = ACTION_SELECT_STATE;

    } else if (currentState == ACTION_SELECT_STATE) {
      updateDifficulty();
      updateDisplay(thresh_met, a, g);
      delay(5000);
      int action = random(3);
      // Randomly select next action
      if (action == 0) {
        currentState = FIREBALL_STATE;
        // mp3.playTrackNumber(2, 30);  // Play fireball prompt
      } else if (action == 1 ){
        currentState = SWING_STATE;
        // mp3.playTrackNumber(3, 30);  // Play start button prompt
      } else {
        currentState = THRUST_STATE;
      }
      ACTION_SUCCESS = false;
      actionTime = millis();  // Start the timer

    } else if (currentState == FIREBALL_STATE) {
      updateDisplay(thresh_met, a, g);
      while (millis() - actionTime <= ACTION_TIMEOUT) {
        delay(10); // debounce
        imu.update();
        imu.getAccel(&a);
        imu.getGyro(&g);
        if (digitalRead(FIREBALL_PIN) == HIGH && checkFireballThreshold(g)) {
          // mp3.playTrackNumber(4, 30);  // Play success sound
          showSuccessPattern(CRGB::Green);
          action_count++;
          currentState = ACTION_SELECT_STATE;
          thresh_met = true;
          ACTION_SUCCESS = true;
          break;
        }
      }
      // mp3.playTrackNumber(5, 30);  // Play failure sound
      if (!ACTION_SUCCESS) {
        health--;
        thresh_met = false;
        updateDisplay(thresh_met, a, g);
        showFailurePattern();
      }
      // Check for Health Status and advance state
      if (health <= 0) currentState = START_STATE;
      else currentState = ACTION_SELECT_STATE;

    } else if (currentState == THRUST_STATE) {
      updateDisplay(thresh_met, a, g);
      while (millis() - actionTime <= ACTION_TIMEOUT) {
        delay(10);  // Debounce
        imu.update();
        imu.getAccel(&a);
        imu.getGyro(&g);
        if (abs(a.accelY) > THRUST_THRESHOLD) {
          // mp3.playTrackNumber(4, 30);  // Play success sound
          showSuccessPattern(CRGB::Red);
          action_count++;
          currentState = ACTION_SELECT_STATE;
          thresh_met = true;
          ACTION_SUCCESS = true;
          break;
        }
      }
      // mp3.playTrackNumber(5, 30);  // Play failure sound
      if (!ACTION_SUCCESS) {
        health--;
        thresh_met = false;
        updateDisplay(thresh_met, a, g);
        showFailurePattern();
      }
      // Check for Health Status and advance state
      if (health <= 0) currentState = START_STATE;
      else currentState = ACTION_SELECT_STATE;

    } else if (currentState == SWING_STATE) {
      updateDisplay(thresh_met, a, g);
      while (millis() - actionTime <= ACTION_TIMEOUT) {
        delay(10);  // Debounce
        imu.update();
        imu.getAccel(&a);
        imu.getGyro(&g);
        if (abs(a.accelZ) > SWING_THRESHOLD) {
          // mp3.playTrackNumber(4, 30);  // Play success sound
          showSuccessPattern(CRGB::Red);
          action_count++;
          currentState = ACTION_SELECT_STATE;
          thresh_met = true;
          ACTION_SUCCESS = true;
          break;
        }
      }
      // mp3.playTrackNumber(5, 30);  // Play failure sound
      if (!ACTION_SUCCESS) {
        health--;
        thresh_met = false;
        updateDisplay(thresh_met, a, g);
        showFailurePattern();
      }
      // Check for Health Status and advance state
      if (health <= 0) currentState = START_STATE;
      else currentState = ACTION_SELECT_STATE;
    } else {
      level = 111;
      health = -111;
      currentState = START_STATE;
    }
    prev_currentTime = currentTime;
  }
}