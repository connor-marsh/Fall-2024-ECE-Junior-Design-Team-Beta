// INCLUDES
#include "Wire.h"             // For I2C communication devices
#include <Adafruit_MPU6050.h> // for IMU
#include <Adafruit_Sensor.h>  // for IMU
#include <SPI.h>              // For SPI communication devices
#include <Adafruit_GFX.h>     // For graphics on OLED
#include <Adafruit_SSD1306.h> // For the OLED itself
#include <FastLED.h>          // For the LED Light Strip
#include "mp3tf16p.h"         // For the DFPlayer

// OLED CONFIGURATION
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_MOSI  11    // SPI MOSI
#define OLED_CLK   13    // SPI SCK
#define OLED_DC    8     // Data/Command
#define OLED_CS    10    // Chip Select
#define OLED_RESET 9     // Reset

// LED STRIP CONFIGURATION
#define NUM_LEDS 144
#define DATA_PIN 6
CRGB leds[NUM_LEDS];

// IMU CONFIGURATION
const int MPU_ADDR = 0x68;          // I2C address of MPU-6050, if AD0 == HIGH, then ADDR = 0x69
int16_t* values = (int16_t*) malloc(6 * sizeof(int16_t)); // array for raw IMU data (accel xyz, gyroxyz)
uint16_t aX, aY, aZ, gX, gY, gZ = 0;
sensors_event_t a, g, temp;         // accelerometer, gyro, temp sensors
char tmp_str[7];                    // temp variable used for conversion
char* int16_to_str(int16_t i) {
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}
Adafruit_MPU6050 mpu;

// PIN DEFINITIONS
#define POWER_PIN 2      // Toggle switch
#define START_PIN 4
#define FIREBALL_PIN 7

// Initialize Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
MP3Player mp3(10, 11);  // Using the custom MP3Player class

// GAME STATES
enum GameState {
  MENU_STATE,
  START_STATE,
  ACTION_SELECT_STATE,
  FIREBALL_STATE,
  START_BUTTON_STATE,
  SWING_STATE
};

// GAME VARIABLES
GameState currentState = SWING_STATE;
int health = 3;
int level = 0;
unsigned long actionTime = 0;
unsigned long prev_imuTime = 0;
unsigned long prev_currentTime = 0;
const unsigned long ACTION_TIMEOUT = 5000; // 5 seconds in milliseconds
bool ACTION_SUCCESS = false;
const unsigned long IMU_POLLING_TIME = 250;
const unsigned long STATE_POLLING_TIME = 100;

void setup() {
  // Initialize pins
  pinMode(POWER_PIN, INPUT);
  pinMode(START_PIN, INPUT);
  pinMode(FIREBALL_PIN, INPUT);
  
  // Initialize LED strip
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
  
  // Initialize Serial for debugging
  Serial.begin(9600);
  
  // Initialize MP3 player
  // mp3.initialize();
  
  // Initialize display with SPI
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }  
  display.clearDisplay();
  display.display();
  // Set text properties for display
  display.setTextColor(SSD1306_WHITE);

  // Initialize IMU on I2C
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins transmission to the I2C slave (GY-521 Board)
  Wire.write(0x6B);                 // PWR_MGT_1 register
  Wire.write(0);                    // Set to 0 (wakes up the MPU-6050)
  Wire.endTransmission(true);
  // if (!mpu.begin()) {
  //   for(;;);  // loop forever
  // }
  // mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  // mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  // mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  // delay(100);
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  if (currentState == MENU_STATE) display.println(F("MENU"));
  else if (currentState == START_STATE) display.println(F("START"));
  else if (currentState == ACTION_SELECT_STATE) display.println(F("ACTION SELECT"));
  else if (currentState == FIREBALL_STATE) display.println(F("FIREBALL"));
  else if (currentState == START_BUTTON_STATE) display.println(F("START_BUTTON"));
  else if (currentState == SWING_STATE) display.println(F("SWING")); 
  else display.println(F("INVALID STATE!"));
  if (currentState != SWING_STATE) {
    display.print(F("Level: ")); display.println(level);
    display.print(F("Health: ")); display.println(health);
  } else {
    display.setCursor(0, 8);
    // mpu.getEvent(&a, &g, &temp);
    display.print(F("aX: "));
    display.print(int16_to_str(aX));

    display.setCursor(0, 16);
    display.print(F("aY: "));
    display.print(int16_to_str(aY));

    display.setCursor(0, 24);
    display.print(F("aZ: "));
    display.println(int16_to_str(aZ));

  }
  if (currentState == FIREBALL_STATE) {
    display.println(F("SHOOT FIREBALL!"));
  } else if (currentState == START_BUTTON_STATE) {
    display.println(F("PRESS START BUTTON!"));
  } else if (currentState == SWING_STATE) {
    // display.setCursor(0, 24);
    // // mpu.getEvent(&a, &g, &temp);
    // display.print(F("aX: "));
    // display.print(int16_to_str(aX));
    // display.print(F(", aY: "));
    // display.print(int16_to_str(aY));
    // display.print(F(", aZ: "));
    // display.println(int16_to_str(aZ));
  } else {
    display.println(F("NA"));
  }
  display.display();
}

void runLEDAnimation(CRGB color) {
  FastLED.clear();
  for (int i = -4; i < NUM_LEDS; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      if (j >= i && j < i + 5) {
        leds[j] = color;
      } else {
        leds[j] = CRGB::Black;
      }
    }
    FastLED.show();
    delay(5);
  }
}

void showFailurePattern(CRGB color) {
  FastLED.clear();
  for (int i = -4; i < NUM_LEDS; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      if (j >= i && j < i + 5) {
        leds[j] = color;
      } else {
        leds[j] = CRGB::Black;
      }
    }
    FastLED.show();
    delay(5);
  }
}

// void getIMUValues() {
//   // This should print IMU data to Serial every second
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
//   Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
//   Wire.requestFrom(MPU_ADDR, 7 * 2, true); // request a total of 7*2=14 registers

//   // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
//   values[0] = Wire.read() << 8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
//   values[1] = Wire.read() << 8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
//   values[2] = Wire.read() << 8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
//   values[3] = Wire.read() << 8 | Wire.read();  // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
//   values[4] = Wire.read() << 8 | Wire.read();  // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
//   values[5] = Wire.read() << 8 | Wire.read();  // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
//   // The values being cast to int16_t are the offsets from the IMU default values
//   // (Idea is when IMU is at rest everything should be 0's)
//   values[0] -= int16_t(1350);
//   values[1] -= int16_t(-50);
//   values[2] -= int16_t(16650);
//   values[3] -= int16_t(-60);
//   values[4] -= int16_t(-20);
//   values[5] -= int16_t(-180);

//   Wire.endTransmission(true);
// }

void loop() {
  // start event timers
  unsigned long currentTime = millis();
  // This should print IMU data to Serial every second
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7 * 2, true); // request a total of 7*2=14 registers

  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  aX = Wire.read() << 8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  aY = Wire.read() << 8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  aZ = Wire.read() << 8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  gX = Wire.read() << 8 | Wire.read();  // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gY = Wire.read() << 8 | Wire.read();  // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gZ = Wire.read() << 8 | Wire.read();  // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
  // The values being cast to int16_t are the offsets from the IMU default values
  // (Idea is when IMU is at rest everything should be 0's)
  aX -= int16_t(1350);
  aY -= int16_t(-50);
  aZ -= int16_t(16650);
  gX -= int16_t(-60);
  gY -= int16_t(-20);
  gZ -= int16_t(-180);

  if (currentTime - prev_currentTime >= STATE_POLLING_TIME) {
    // Check power toggle at all times
    if (digitalRead(POWER_PIN) == LOW) {
      currentState = MENU_STATE;
    }

    if (currentState == MENU_STATE) {
      FastLED.clear();
      FastLED.show();
      updateDisplay();
      // mp3.playTrackNumber(0, 30);  // Play menu sound
      while (digitalRead(POWER_PIN) == LOW) {
        delay(100);  // Wait for toggle to be set to HIGH
      }
      currentState = START_STATE;
    
    } else if (currentState == START_STATE) {
      health = 3;
      level = 0;
      updateDisplay();
      while(digitalRead(START_PIN) == LOW) {
        // mp3.playTrackNumber(1, 30);  // Play start sound
        delay(100);  // Debounce
      }
      currentState = ACTION_SELECT_STATE;

    } else if (currentState == ACTION_SELECT_STATE) {
      updateDisplay();
      delay(1000);
      // Randomly select next action
      if (random(2) == 0) {
        currentState = FIREBALL_STATE;
        // mp3.playTrackNumber(2, 30);  // Play fireball prompt
      } else {
        currentState = START_BUTTON_STATE;
        // mp3.playTrackNumber(3, 30);  // Play start button prompt
      }
      ACTION_SUCCESS = false;
      actionTime = millis();  // Start the timer
      

    } else if (currentState == FIREBALL_STATE) {
      updateDisplay();
      while (millis() - actionTime <= ACTION_TIMEOUT) {
        delay(10); // debounce
        if (digitalRead(FIREBALL_PIN) == HIGH) {
          // mp3.playTrackNumber(4, 30);  // Play success sound
          runLEDAnimation(CRGB::Green);
          level++;
          currentState = ACTION_SELECT_STATE;
          ACTION_SUCCESS = true;
          break;
        }
      }
      // mp3.playTrackNumber(5, 30);  // Play failure sound
      if (!ACTION_SUCCESS) {
        health--;
        updateDisplay();
        showFailurePattern(CRGB::Blue);
      }
      // Check for Health Status and advance state
      if (health <= 0) {currentState = START_STATE;}
      else {currentState = ACTION_SELECT_STATE;}

    } else if (currentState == START_BUTTON_STATE) {
      updateDisplay();
      while (millis() - actionTime <= ACTION_TIMEOUT) {
        delay(10);  // Debounce
        if (digitalRead(START_PIN) == HIGH) {
          // mp3.playTrackNumber(4, 30);  // Play success sound
          runLEDAnimation(CRGB::Red);
          level++;
          currentState = ACTION_SELECT_STATE;
          ACTION_SUCCESS = true;
          break;
        }
      }
      // mp3.playTrackNumber(5, 30);  // Play failure sound
      if (!ACTION_SUCCESS) {
        health--;
        updateDisplay();
        showFailurePattern(CRGB::Orange);
      }
      // Check for Health Status and advance state
      if (health <= 0) currentState = START_STATE;
      else currentState = ACTION_SELECT_STATE;

    } else if (currentState == SWING_STATE) {
      updateDisplay();
      delay(10);
    } else {
      level = 111;
      health = -111;
      currentState = START_STATE;
    }
    prev_currentTime = currentTime;
  }

  // // IMU POLLING
  // if (currentTime - prev_imuTime >= IMU_POLLING_TIME) {
  //   level++;
  //   updateDisplay();
  // }

}