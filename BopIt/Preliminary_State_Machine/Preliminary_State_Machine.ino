// INCLUDES
#include <Wire.h>             // For I2C communication devices
#include <SPI.h>              // For SPI communication devices
#include <Adafruit_GFX.h>     // For graphics on OLED
#include <Adafruit_SSD1306.h> // For the OLED itself
#include <FastLED.h>          // For the LED Light Strip
#include <FastIMU.h>          // For Grabbing IMU data off I2C
// #include "SoftwareSerial.h"   // For the DFPlayer
// #include "DFRobotDFPlayerMini.h" // For the DFPlayer

// OLED CONFIGURATION
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
// #define OLED_RESET -1
#define OLED_MOSI  11     // SPI MOSI
#define OLED_CLK   13     // SPI SCK
#define OLED_DC    8      // Data/Command
#define OLED_CS    10     // Chip Select
#define OLED_RESET 9      // Reset (-1 if sharing arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// LED STRIP CONFIGURATION
#define NUM_LEDS 120
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
// #define DF_TX_PIN 2      // DFPlayer TX Pin
// #define DF_RX_PIN 3      // DFPlayer RX Pin
#define START_PIN 7      // SW1 
#define FIREBALL_PIN 4   // SW2

// Initialize DFPlayer
// SoftwareSerial swSerial(DF_RX_PIN, DF_TX_PIN); // RX, TX
// DFRobotDFPlayerMini player;

// GAME STATES
enum GameState {
  // MENU_STATE,
  START_STATE,
  ACTION_SELECT_STATE,
  FIREBALL_STATE,
  SWING_STATE,
  THRUST_STATE,
};

// GAME VARIABLES
GameState currentState = START_STATE;
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

// SOUND MAP
#define HEY 1
#define LISTEN 2
#define THEME 3
#define HURT 9
#define THRUST 10 
#define SWING 11
#define FIRE 12
#define SUCCESS 13
#define LEVEL_UP 14
#define DEATH 15

void setup() {
  // Initialize Serial for debugging
  Serial.begin(9600);
  while (!Serial) {
    delay(10);  // wait for serial port to connect
  }
  
  //Initialize I2C
  Wire.begin();
  
  // Initialize pins
  // pinMode(POWER_PIN, INPUT);
  pinMode(START_PIN, INPUT);
  pinMode(FIREBALL_PIN, INPUT);
  
  // Initialize LED strip
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS/2);
  FastLED.clear();
  FastLED.show();
  
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
  delay(1000);

  // Initialize MP3 player
  // display.println(F("Starting DFPlayer..."));
  // display.display();
  // swSerial.begin(9600);
  // if (!player.begin(swSerial)) {  //Use softwareSerial to communicate with mp3.
  //   display.clearDisplay();
  //   display.println(F("Unable to begin:"));
  //   display.println(F("1.check connection"));
  //   display.println(F("2.insert SD card!"));
  //   while(true);
  // }
  // display.println(F("DFPlayer Mini online."));
  // display.display();
  // player.volume(30);
  // player.play(HEY); delay(10000); // Hey!
  // player.play(LISTEN); delay(10000); // Listen!
  // player.play(THEME); delay(10000); // Theme
  // player.play(HURT); delay(10000); // Really Hurt
  // player.play(THRUST); delay(10000); // Thrust!
  // player.play(SWING); delay(10000);// Swing!
  // player.play(FIRE); delay(10000);// Fireball!
  // player.play(SUCCESS); delay(10000); // Action Complete!
  // player.play(LEVEL_UP); delay(10000);// Level Up!
  // player.play(DEATH); delay(10000); // Dead Theme (Zelda's Lullaby)
  delay(3000);

  // Initialize IMU
  int err = imu.init(calibrationData, IMU_ADDRESS);
  if (err != 0) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(F("IMU Error: "));
    display.println(err);
    display.display();
    // Serial.print("IMU Error: "); Serial.println(err);
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
  // if (currentState == MENU_STATE) display.println(F("MENU"));
  if (currentState == START_STATE) display.println(F("START"));
  else if (currentState == ACTION_SELECT_STATE) display.println(F("ACTION SELECT"));
  else if (currentState == FIREBALL_STATE) display.println(F("FIREBALL!"));
  else if (currentState == THRUST_STATE) display.println(F("THRUST!"));
  else if (currentState == SWING_STATE) display.println(F("SWING!")); 
  else display.println(F("INVALID STATE!"));

  // ROW 2 + 3 + 4 OLED OUTPUT
  display.print(F("Level: ")); display.println(level);
  display.print(F("Health: ")); display.println(health);
  // if (threshold) display.println(F("THRESHOLD MET"));
  // else display.println(F("THRESHOLD NOT MET"));
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
  tone(A2, 150, 200); delay(200); noTone(A2);
  tone(A2, 250, 200); delay(200); noTone(A2);
  tone(A2, 350, 200); delay(200); noTone(A2);
  tone(A2, 450, 200); delay(200); noTone(A2);
  tone(A2, 350, 200); delay(200); noTone(A2);
  tone(A2, 250, 200); delay(200); noTone(A2);
  tone(A2, 150, 200); delay(200); noTone(A2);
  tone(A2, 700, 500); delay(500); noTone(A2);
  const int LED = 20;
  for (int i = -LED; i < NUM_LEDS; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      if (j >= i && j < i + LED + 1) {
        leds[j] = color;
        // leds[NUM_LEDS-j] = color;
      } else {
        leds[j] = CRGB::Black;
        // leds[NUM_LEDS-j] = CRGB::Black;
      }
    }
    FastLED.show();
    delay(5);
  }
}

void showFailurePattern() {
  const int FADE_CYCLES = 10;
  unsigned long brightness = 0x08080808; 
  for (int c = 0; c < FADE_CYCLES; c++){
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i % 5 == 0) leds[i] = brightness;
      else leds[i] = 0x000000;
    }
    FastLED.show();
    delay(100);
    if (c < (FADE_CYCLES / 2)) brightness += 0x01010101;
    else brightness -= 0x01010101;
  }
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
    // player.play(LEVEL_UP); delay(10000);
  }
  tone(A2, 150, 200); delay(200); noTone(A2);
  tone(A2, 250, 200); delay(200); noTone(A2);
  tone(A2, 350, 200); delay(200); noTone(A2);
  tone(A2, 450, 200); delay(200); noTone(A2);
  tone(A2, 350, 200); delay(200); noTone(A2);
  tone(A2, 250, 200); delay(200); noTone(A2);
  tone(A2, 150, 200); delay(200); noTone(A2);
  tone(A2, 700, 500); delay(500); noTone(A2);
}

void loseLife(bool thresh_met) {
  health--;
  thresh_met = false;
  // player.play(HURT); delay(12000);
  tone(A2, 700, 200); delay(200); noTone(A2);
  tone(A2, 600, 200); delay(200); noTone(A2);
  tone(A2, 500, 200); delay(200); noTone(A2);
  tone(A2, 400, 200); delay(200); noTone(A2);
  tone(A2, 300, 200); delay(200); noTone(A2);
  tone(A2, 200, 200); delay(200); noTone(A2);
  tone(A2, 100, 200); delay(200); noTone(A2);
  tone(A2, 50, 200); delay(200); noTone(A2);
  delay(2000);
}

void loseGame() {
  currentState = START_STATE;
  // player.play(DEATH); delay(20000);
  tone(A2, 700, 400); delay(400); noTone(A2);
  tone(A2, 600, 400); delay(400); noTone(A2);
  tone(A2, 700, 400); delay(400); noTone(A2);
  tone(A2, 600, 400); delay(400); noTone(A2);
  tone(A2, 500, 5000); delay(5000); noTone(A2);
}

void loop() {
  // start event timers
  unsigned long currentTime = millis();
  bool thresh_met = false;

  if (currentTime - prev_currentTime >= STATE_POLLING_TIME) {
    
    if (currentState == START_STATE) {
      health = 3;
      level = 0;
      action_count = 0;
      total_actions = 1;
      updateDisplay(thresh_met, a, g);
      tone(A2, 150, 200); delay(200); noTone(A2);
      tone(A2, 200, 200); delay(200); noTone(A2);
      tone(A2, 250, 200); delay(200); noTone(A2);
      tone(A2, 300, 200); delay(200); noTone(A2);
      tone(A2, 350, 200); delay(200); noTone(A2);
      tone(A2, 400, 200); delay(200); noTone(A2);
      tone(A2, 450, 200); delay(200); noTone(A2);
      // player.play(THEME); delay(20000);
      while(digitalRead(START_PIN) == LOW) {
        // mp3.playTrackNumber(1, 30);  // Play start sound
        delay(100);  // Debounce
      }
      currentState = ACTION_SELECT_STATE;

    } else if (currentState == ACTION_SELECT_STATE) {
      updateDifficulty();
      updateDisplay(thresh_met, a, g);
      // player.play(HEY); delay(2000);
      // player.play(LISTEN); delay(2000);
      int action = random(3);
      // Randomly select next action
      if (action == 0) {
        currentState = FIREBALL_STATE;
        tone(A2, 150, 200); delay(200); noTone(A2);
        tone(A2, 250, 200); delay(200); noTone(A2);
        tone(A2, 250, 200); delay(200); noTone(A2);
        tone(A2, 150, 200); delay(200); noTone(A2);
        // player.play(FIRE);
      } else if (action == 1 ){
        currentState = SWING_STATE;
        // player.play(SWING);
        tone(A2, 550, 500); delay(500); noTone(A2);
        tone(A2, 450, 500); delay(500); noTone(A2);
        tone(A2, 350, 500); delay(500); noTone(A2);
        tone(A2, 250, 500); delay(500); noTone(A2);
      } else {
        currentState = THRUST_STATE;
        // player.play(THRUST);
        tone(A2, 150, 300); delay(300); noTone(A2);
        tone(A2, 750, 300); delay(300); noTone(A2);
        tone(A2, 150, 300); delay(300); noTone(A2);
        tone(A2, 750, 300); delay(300); noTone(A2);
      }
      ACTION_SUCCESS = false;
      delay(2000);
      actionTime = millis();  // Start the timer

    } else if (currentState == FIREBALL_STATE) {
      updateDisplay(thresh_met, a, g);
      while (millis() - actionTime <= ACTION_TIMEOUT) {
        delay(11); // debounce
        imu.update();
        imu.getAccel(&a);
        imu.getGyro(&g);
        if (digitalRead(FIREBALL_PIN) == HIGH && checkFireballThreshold(g)) {
          // mp3.playTrackNumber(4, 30);  // Play success sound
          showSuccessPattern(CRGB::White);
          action_count++;
          currentState = ACTION_SELECT_STATE;
          thresh_met = true;
          ACTION_SUCCESS = true;
          // player.play(SUCCESS); delay(10000);
          break;
        }
      }
      // mp3.playTrackNumber(5, 30);  // Play failure sound
      if (!ACTION_SUCCESS) {
        updateDisplay(thresh_met, a, g);
        showFailurePattern();
        loseLife(&thresh_met);
      }
      // Check for Health Status and advance state
      if (health <= 0) loseGame();
      else currentState = ACTION_SELECT_STATE;

    } else if (currentState == THRUST_STATE) {
      updateDisplay(thresh_met, a, g);
      while (millis() - actionTime <= ACTION_TIMEOUT) {
        delay(10);  // Debounce
        imu.update();
        imu.getAccel(&a);
        imu.getGyro(&g);
        if (abs(a.accelZ) > THRUST_THRESHOLD) {
          // mp3.playTrackNumber(4, 30);  // Play success sound
          showSuccessPattern(CRGB::White);
          action_count++;
          currentState = ACTION_SELECT_STATE;
          thresh_met = true;
          ACTION_SUCCESS = true;
          // player.play(SUCCESS); delay(10000);
          break;
        }
      }
      // mp3.playTrackNumber(5, 30);  // Play failure sound
      if (!ACTION_SUCCESS) {
        updateDisplay(thresh_met, a, g);
        showFailurePattern();
        loseLife(&thresh_met);
      }
      // Check for Health Status and advance state
      if (health <= 0) loseGame();
      else currentState = ACTION_SELECT_STATE;

    } else if (currentState == SWING_STATE) {
      updateDisplay(thresh_met, a, g);
      while (millis() - actionTime <= ACTION_TIMEOUT) {
        delay(10);  // Debounce
        imu.update();
        imu.getAccel(&a);
        imu.getGyro(&g);
        if (abs(a.accelX) > SWING_THRESHOLD) {
          // mp3.playTrackNumber(4, 30);  // Play success sound
          showSuccessPattern(CRGB::White);
          action_count++;
          currentState = ACTION_SELECT_STATE;
          thresh_met = true;
          ACTION_SUCCESS = true;
          // player.play(SUCCESS); delay(10000); 
          break;
        }
      }
      if (!ACTION_SUCCESS) {
        updateDisplay(thresh_met, a, g);
        showFailurePattern();
        loseLife(&thresh_met);
      }
      // Check for Health Status and advance state
      if (health <= 0) loseGame();
      else currentState = ACTION_SELECT_STATE;
    } else {
      level = 111;
      health = -111;
      currentState = START_STATE;
    }
    prev_currentTime = currentTime;
  }
}