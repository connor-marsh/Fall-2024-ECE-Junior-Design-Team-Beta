#include <Wire.h>              // For I2C communication
#include <FastIMU.h>           // For IMU
#include <SPI.h>               // For SPI communication
#include <Adafruit_GFX.h>      // Graphics library for OLED
#include <Adafruit_SSD1306.h>  // OLED display library
#include <EEPROM.h>            // For saving calibration data

// OLED CONFIGURATION
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 32     // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin (-1 if sharing Arduino reset pin)
#define OLED_MOSI  11    // SPI MOSI
#define OLED_CLK   13    // SPI SCK
#define OLED_DC    8     // Data/Command
#define OLED_CS    10    // Chip Select
#define OLED_RESET 9     // Reset
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// IMU CONFIGURATION
#define IMU_ADDRESS 0x68
// #define PERFORM_CALIBRATION    // Comment this out after getting calibration values
MPU6050 imu;
const calData calibrationData = {
    true,                     // valid flag
    {0.091, 0.001, 0.006},    // accelBias
    {-0.313, -0.412, -1.443}, // gyroBias
    {0.0, 0.0, 0.0},          // magBias (unused for MPU6050)
    {1.0, 1.0, 1.0}           // magScale (unused for MPU6050)
};
bool hitThreshold = false;


// Function declarations
void displayIMUData(AccelData accel, GyroData gyro);
void performCalibration();

void setup() {
  // Initialize Serial for debugging
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // Wait for serial port to connect
  }
  Serial.println("Starting IMU Debug Program");

  // Initialize I2C
  Wire.begin();
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }  

  // Initial display setup
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

  #ifdef PERFORM_CALIBRATION
    performCalibration();
  #endif

  // Set accelerometer range (after calibration)
  err = imu.setAccelRange(8); // ±8g range
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

void loop() {
  static unsigned long lastUpdate = 0;
  const unsigned long UPDATE_INTERVAL = 100; // Update every 100ms
  
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    AccelData accel;
    GyroData gyro;
    
    imu.update();
    imu.getAccel(&accel);
    imu.getGyro(&gyro);
    displayIMUData(accel, gyro);
    
    lastUpdate = millis();
  }
}

void performCalibration() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("Place IMU flat &"));
  display.println(F("keep still"));
  display.display();
  delay(5000);
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("Calibrating..."));
  display.display();
  
  imu.calibrateAccelGyro(&calibrationData);
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("Calibration done!"));
  display.display();
  delay(2000);
  
  // Print calibration values continuously
  while(1) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("Accel/Gyro Bias (XYZ)"));
    display.print(calibrationData.accelBias[0], 3); display.print(" ");
    display.print(calibrationData.accelBias[1], 3); display.print(" ");
    display.println(calibrationData.accelBias[2], 3);
    // display.println(F("\nGyro Bias:"));
    display.print(calibrationData.gyroBias[0], 3); display.print(" ");
    display.print(calibrationData.gyroBias[1], 3); display.print(" ");
    display.println(calibrationData.gyroBias[2], 3);
    
    display.display();
    
    // Print to Serial for easy copying
    Serial.println(F("\nCopy these calibration values:"));
    Serial.println(F("Accel Bias:"));
    for(int i = 0; i < 3; i++) {
      Serial.print(calibrationData.accelBias[i], 6);
      Serial.print(", ");
    }
    Serial.println(F("\nGyro Bias:"));
    for(int i = 0; i < 3; i++) {
      Serial.print(calibrationData.gyroBias[i], 6);
      Serial.print(", ");
    }
    Serial.println("\n");
    
    delay(5000);
  }
}

void displayIMUData(AccelData accel, GyroData gyro) {
  display.clearDisplay();
  display.setCursor(0,0);
  
  // // Display accelerometer data
  // display.print(F("aX:")); display.println(accel.accelX, 2);
  // display.print(F("aY:")); display.println(accel.accelY, 2);
  // display.print(F("aZ:")); display.println(accel.accelZ, 2);
  
  // Display gyro data
  display.print(F("gX:")); display.println(gyro.gyroX, 2);
  display.print(F("gY:")); display.println(gyro.gyroY, 2);
  display.print(F("gZ:")); display.println(gyro.gyroZ, 2);

  if (abs(accel.accelY) > 1.6) hitThreshold = true;
  else hitThreshold = false;
  display.print(F("HitThresh: ")); display.println(hitThreshold);

  // Print to Serial for debugging
  Serial.print(F("IMU Data - "));
  Serial.print(F("aX: ")); Serial.print(accel.accelX, 2);
  Serial.print(F(" aY: ")); Serial.print(accel.accelY, 2);
  Serial.print(F(" aZ: ")); Serial.print(accel.accelZ, 2);
  Serial.print(F(" gX: ")); Serial.print(gyro.gyroX, 2);
  Serial.print(F(" gY: ")); Serial.print(gyro.gyroY, 2);
  Serial.print(F(" gZ: ")); Serial.println(gyro.gyroZ, 2);
  
  display.display();
}