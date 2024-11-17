// INCLUDES
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// OLED CONFIGURATION
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
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

// DFPlayer pins
#define DFPLAYER_RX 5
#define DFPLAYER_TX 3

// Initialize Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
SoftwareSerial mySoftwareSerial(DFPLAYER_RX, DFPLAYER_TX); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

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
  
  // Initialize DFPlayer
  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("DFPlayer Mini failed!"));
    while(true);
  }
  myDFPlayer.setTimeOut(500);
  myDFPlayer.volume(30);  // Set volume (0-30)
  
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
  
  if (currentState == START_STATE) {
    display.setCursor(40, 20);
    display.println(F("START"));
    display.setCursor(30, 35);
    display.print(F("Level: "));
    display.println(level);
    display.setCursor(30, 45);
    display.print(F("Health: "));
    display.println(health);
  } else {
    display.setCursor(30, 25);
    display.print(F("Level: "));
    display.println(level);
    display.setCursor(30, 35);
    display.print(F("Health: "));
    display.println(health);
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
    delay(5);
  }
}

void showFailurePattern(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i += 2) {
    leds[i] = color;
    leds[i + 1] = CRGB::Black;
  }
  FastLED.show();
  delay(500);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  // Check power toggle at all times
  if (digitalRead(POWER_PIN) == LOW) {
    currentState = MENU_STATE;
  }

  switch (currentState) {
    case MENU_STATE:
      FastLED.clear();
      FastLED.show();
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(20, 25);
      display.println(F("MENU"));
      display.display();
      myDFPlayer.play(1);  // Play track 1 (menu sound)
      while (digitalRead(POWER_PIN) == LOW) {
        delay(100);  // Wait for toggle to be set to HIGH
      }
      currentState = START_STATE;
      break;

    case START_STATE:
      health = 3;
      level = 0;
      updateDisplay();
      if (digitalRead(START_PIN) == HIGH) {
        myDFPlayer.play(2);  // Play track 2 (start sound)
        delay(500);  // Debounce
        currentState = ACTION_SELECT_STATE;
      }
      break;

    case ACTION_SELECT_STATE:
      updateDisplay();
      // Randomly select next action
      if (random(2) == 0) {
        currentState = FIREBALL_STATE;
        myDFPlayer.play(3);  // Play track 3 (fireball prompt)
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, 25);
        display.println(F("FIREBALL!"));
        display.display();
      } else {
        currentState = START_BUTTON_STATE;
        myDFPlayer.play(4);  // Play track 4 (start button prompt)
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(20, 25);
        display.println(F("START!"));
        display.display();
      }
      actionTimer = millis();  // Start the timer
      break;

    case FIREBALL_STATE:
      if (millis() - actionTimer <= ACTION_TIMEOUT) {
        if (digitalRead(FIREBALL_PIN) == HIGH) {
          myDFPlayer.play(5);  // Play track 5 (success sound)
          runLEDAnimation(CRGB::Red);
          level++;
          currentState = ACTION_SELECT_STATE;
          delay(500);  // Debounce
        }
      } else {
        myDFPlayer.play(6);  // Play track 6 (failure sound)
        showFailurePattern(CRGB::Blue);
        health--;
        if (health <= 0) {
          currentState = START_STATE;
        } else {
          currentState = ACTION_SELECT_STATE;
        }
      }
      break;

    case START_BUTTON_STATE:
      if (millis() - actionTimer <= ACTION_TIMEOUT) {
        if (digitalRead(START_PIN) == HIGH) {
          myDFPlayer.play(5);  // Play track 5 (success sound)
          runLEDAnimation(CRGB::Green);
          level++;
          currentState = ACTION_SELECT_STATE;
          delay(500);  // Debounce
        }
      } else {
        myDFPlayer.play(6);  // Play track 6 (failure sound)
        showFailurePattern(CRGB::Purple);
        health--;
        if (health <= 0) {
          currentState = START_STATE;
        } else {
          currentState = ACTION_SELECT_STATE;
        }
      }
      break;
  }
}
