// INCLUDES
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include "mp3tf16p.h"

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
  START_BUTTON_STATE
};

// GAME VARIABLES
GameState currentState = MENU_STATE;
int health = 3;
int level = 0;
unsigned long actionTimer = 0;
const unsigned long ACTION_TIMEOUT = 5000; // 5 seconds in milliseconds
bool ACTION_SUCCESS = false;

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
  else display.println(F("INVALID STATE!"));
  display.print(F("Level: ")); display.println(level);
  display.print(F("Health: ")); display.println(health);
  if (currentState == FIREBALL_STATE) {
    display.println(F("SHOOT FIREBALL!"));
  } else if(currentState == START_BUTTON_STATE) {
    display.println(F("PRESS START BUTTON!"));
  } else {
    display.println(F("NA"));
  }
  display.display();
}

void runLEDAnimation(CRGB color) {
  for (int i = -4; i < NUM_LEDS; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      if (j >= i && j < i + 5) {
        leds[j] = color;
      } else {
        leds[j] = CRGB::Black;
      }
    }
    FastLED.show();
    delay(10);
  }
}

void showFailurePattern(CRGB color) {
  for (int i = -4; i < NUM_LEDS; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      if (j >= i && j < i + 5) {
        leds[j] = color;
      } else {
        leds[j] = CRGB::Black;
      }
    }
    FastLED.show();
    delay(10);
  }
}

void loop() {
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
      // display.clearDisplay();
      // display.setTextSize(1);
      // display.setTextColor(SSD1306_WHITE);
      // display.setCursor(0, 0);
      // display.println(F("SHOOT FIREBALL!"));
      // display.display();
    } else {
      currentState = START_BUTTON_STATE;
      // mp3.playTrackNumber(3, 30);  // Play start button prompt
      // display.clearDisplay();
      // display.setTextSize(1);
      // display.setTextColor(SSD1306_WHITE);
      // display.setCursor(0, 0);
      // display.println(F("HIT START BUTTON!"));
      // display.display();
    }
    ACTION_SUCCESS = false;
    actionTimer = millis();  // Start the timer
    

  } else if (currentState == FIREBALL_STATE) {
    updateDisplay();
    while (millis() - actionTimer <= ACTION_TIMEOUT) {
      if (digitalRead(FIREBALL_PIN) == HIGH) {
        // mp3.playTrackNumber(4, 30);  // Play success sound
        runLEDAnimation(CRGB::Red);
        level++;
        currentState = ACTION_SELECT_STATE;
        ACTION_SUCCESS = true;
        delay(100);  // Debounce
      }
    }
    // mp3.playTrackNumber(5, 30);  // Play failure sound
    if (!ACTION_SUCCESS) {
      health--;
      updateDisplay();
      showFailurePattern(CRGB::Blue);
      if (health <= 0) {
        currentState = START_STATE;
      } else {
        currentState = ACTION_SELECT_STATE;
      }
      delay(1000);
    }

  } else if (currentState == START_BUTTON_STATE) {
    updateDisplay();
    while (millis() - actionTimer <= ACTION_TIMEOUT) {
      if (digitalRead(START_PIN) == HIGH) {
        // mp3.playTrackNumber(4, 30);  // Play success sound
        runLEDAnimation(CRGB::Green);
        level++;
        currentState = ACTION_SELECT_STATE;
        ACTION_SUCCESS = true;
        delay(100);  // Debounce
      }
    }
    // mp3.playTrackNumber(5, 30);  // Play failure sound
    if (!ACTION_SUCCESS) {
      health--;
      updateDisplay();
      showFailurePattern(CRGB::Orange);
      if (health <= 0) {
        currentState = START_STATE;
      } else {
        currentState = ACTION_SELECT_STATE;
      }
      delay(1000);
    }
  } else {
    level = 111;
    health = -111;
  }
}