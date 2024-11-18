// INCLUDES
#include "Wire.h" // This library allows you to communicate with I2C devices.
#include <Adafruit_GFX.h> // For graphics for the OLED
// #include <Adafruit_SSD1331.h> // For the OLED
#include <SPI.h> // For SPI comms, duh lol
#include <FastLED.h> // For Led strip


// TODO add code for speaker using DFPlayer device

// LED STRIP STUFF (WS2812B)
// https://randomnerdtutorials.com/guide-for-ws2812b-addressable-rgb-led-strip-with-arduino/
#define NUM_LEDS 144
#define DATA_PIN 6
CRGB leds[NUM_LEDS];


// I2C IMU STUFF
// heres a link to the tutorial i got the code from
// in that tutorial there's also a link to a more complicated tutorial/library for intepreting the IMU values
// The code I got from this tutorial is just the simple stuff that just reads the IMU values and doesn't interpret
// I (Connor) think the interpretation we will need to should be simple enough where we won't need the more complex library
// but who knows
// https://mschoeffler.com/2017/10/05/tutorial-how-to-use-the-gy-521-module-mpu-6050-breakout-board-with-the-arduino-uno/
const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.
int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data
char tmp_str[7]; // temporary variable used in convert function
char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

// OLED Device stuff
// Heres a link to the tutorial i got the code from
// https://arduino-tutorials.net/tutorial/96x64-i2c-oled-ssd1331-on-arduino
// You can use any (4 or) 5 pins
// #define SCLK 13
// #define MOSI 11
// #define CS   10
// #define RST  9
// #define DC   8
// // Color definitions
// #define BLACK           0x0000
// #define BLUE            0x001F
// #define RED             0xF800
// #define GREEN           0x07E0
// #define CYAN            0x07FF
// #define MAGENTA         0xF81F
// #define YELLOW          0xFFE0
// #define WHITE           0xFFFF
// Adafruit_SSD1331 display = Adafruit_SSD1331(&SPI, CS, DC, RST);
// float p = 3.1415926;

// NON-LIBRARY STUFF
// pins for the 4 digital inputs will all be pulled down and active high
#define POWER_PIN 2
#define START_PIN 4
#define FIREBALL_PIN 7
#define SHEATH_PIN 12
#define SPEAKER_PIN 3 // this isn't actually used anywhere other than PCM.c
#define VIBRATION_PIN 5
#define NEOPIXEL_PIN 6
// Currently free pins are 0 (RX), 1 (TX), 12, and A0, A1, A2, A3

void lcdTestPattern(void);

void setup(void) {

  // setting up pinmodes for non-library stuff
  pinMode(POWER_PIN, INPUT);
  pinMode(START_PIN, INPUT);
  pinMode(FIREBALL_PIN, INPUT);
  pinMode(SHEATH_PIN, INPUT);
  pinMode(VIBRATION_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  // LED Strip setup
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);

  Serial.println("Setting up IMU on I2C");
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  Serial.println("Done setting up IMU on I2C");

  // Serial.begin(9600);
  // Serial.println("Testing OLED!");
  // display.begin();
  // display.fillScreen(BLACK);
  // delay(2000);
  // lcdTestPattern();
  // Serial.println("OLED Test complete");
  // delay(2000);

  //  Serial.println("Testing Speaker!");
  //  startPlayback(audioSample, sizeof(audioSample));
  //  delay(2000);
  //  Serial.println("Speaker Test complete!");

  Serial.println("Done setup");
  delay(100);
}

// Hey connor here, gonna include this program mode variable for now for when we test stuff during class on 10/22
// Only useful for testing things in loop. Stuff that just runs in setup can just all run in one go who cares
// just switch the mode around to the different possible values in loop() to test stuff
// current modes are: IMU, BUTTON_MOTOR, SPEAKER_TONE, LED_STRIP
const String PROGRAM_MODE = "LED_STRIP";
void loop() {
  if (PROGRAM_MODE == "IMU") {
    // This should print IMU data to Serial every second
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
    Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
    Wire.requestFrom(MPU_ADDR, 7 * 2, true); // request a total of 7*2=14 registers

    // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
    accelerometer_x = Wire.read() << 8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
    accelerometer_y = Wire.read() << 8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
    accelerometer_z = Wire.read() << 8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
    temperature = Wire.read() << 8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
    gyro_x = Wire.read() << 8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
    gyro_y = Wire.read() << 8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
    gyro_z = Wire.read() << 8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
    // The values being cast to int16_t are the offsets from the IMU default values
    // (Idea is when IMU is at rest everything should be 0's)
    accelerometer_x -= int16_t(1350);
    accelerometer_y -= int16_t(-50);
    accelerometer_z -= int16_t(16650);
    gyro_x -= int16_t(-60);
    gyro_y -= int16_t(-20);
    gyro_z -= int16_t(-180);

    // print out data
    Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
    Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
    Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
    // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
    Serial.print(" | tmp = "); Serial.print(temperature / 340.00 + 36.53);
    Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
    Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
    Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
    Serial.println();

    // delay
    delay(200); // longer delay so you can read (like with your eyes) Serial in between time steps
  }
  //  **********************************************************
  else if (PROGRAM_MODE == "BUTTON_MOTOR") {
    // This mode will turn the vibration motor on if any of the buttons/toggles are on
    // will also print to Serial
    bool power_pressed = digitalRead(POWER_PIN) == HIGH;
    bool start_pressed = digitalRead(START_PIN) == HIGH;
    bool fireball_pressed = digitalRead(FIREBALL_PIN) == HIGH;
    bool sheath_pressed = digitalRead(SHEATH_PIN) == HIGH;
    if (power_pressed) Serial.println("POWER PRESSED");
    if (start_pressed) Serial.println("START PRESSED");
    if (fireball_pressed) Serial.println("FIREBALL PRESSED");
    if (sheath_pressed) Serial.println("SHEATH PRESSED");
    if (power_pressed || start_pressed || fireball_pressed || sheath_pressed) {
      digitalWrite(VIBRATION_PIN, HIGH);
    }
    else {
      digitalWrite(VIBRATION_PIN, LOW);
    }
    delay(500); // shorter delay for quick responses
  }
  else if (PROGRAM_MODE == "SPEAKER_TONE") {
    tone(SPEAKER_PIN, 440, 2000);
    delay(1000);
    tone(SPEAKER_PIN, 220, 2000);
    delay(1000);
  }
  else if (PROGRAM_MODE == "LED_STRIP") {
    // this makes red leds shoot across which is similar to what we would want for the fireball
    for (int i = -4; i < NUM_LEDS; i++) {
      for (int j = 0; j < NUM_LEDS; j++) {
        if (j >= i && j < i+5) {
          leds[j] = CRGB::White;
        }
        else {
          leds[j] = CRGB::Black;
        }
      }
      FastLED.show();
      delay(5);
    }
    delay(500);
  }
}

// OLED test function from example. To see more test functions and functions to draw shapes etc
// go to File -> Examples -> Arduino SSD1331 [etc] -> test
/**************************************************************************/
/*!
    @brief  Renders a simple test pattern on the OLED
*/
/**************************************************************************/
// void lcdTestPattern(void)
// {
//   uint8_t w, h;
//   display.setAddrWindow(0, 0, 96, 64);

//   for (h = 0; h < 64; h++) {
//     for (w = 0; w < 96; w++) {
//       if (w > 83) {
//         display.writePixel(w, h, WHITE);
//       } else if (w > 71) {
//         display.writePixel(w, h, BLUE);
//       } else if (w > 59) {
//         display.writePixel(w, h, GREEN);
//       } else if (w > 47) {
//         display.writePixel(w, h, CYAN);
//       } else if (w > 35) {
//         display.writePixel(w, h, RED);
//       } else if (w > 23) {
//         display.writePixel(w, h, MAGENTA);
//       } else if (w > 11) {
//         display.writePixel(w, h, YELLOW);
//       } else {
//         display.writePixel(w, h, BLACK);
//       }
//     }
//   }
//   display.endWrite();
// }
