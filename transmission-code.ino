```cpp
/*
=========================================================
LUMISAT CubeSat Telemetry Transmission System
=========================================================

Project:
LUMISAT – Monitoring Artificial Light Pollution

Description:
ESP32-S3 firmware for telemetry acquisition and
real-time cloud transmission using Firebase.

Sensors:
- MPU6050 IMU
- BMP581 Pressure & Temperature Sensor
- LDR / Ambient Light Sensor

Features:
- Environmental monitoring
- Altitude estimation
- Motion sensing
- Real-time telemetry
- Firebase integration

Author:
Laxmipriya Murmu

=========================================================
*/

#include "WiFi.h"
#include "Firebase_ESP_Client.h"
#include "time.h"
#include "Adafruit_MPU6050.h"
#include "Adafruit_Sensor.h"
#include "SPI.h"
#include "SparkFun_BMP581_Arduino_Library.h"
#include "ESP32Servo.h"

// =====================================================
// Wi-Fi Configuration
// =====================================================
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// =====================================================
// Firebase Configuration
// =====================================================
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"

#define USER_EMAIL "YOUR_EMAIL"
#define USER_PASSWORD "YOUR_PASSWORD"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// MPU6050 object
Adafruit_MPU6050 mpu;

// BMP581 object and SPI pins
BMP581 bmp;

#define SCK_PIN 36
#define MISO_PIN 37
#define MOSI_PIN 35
#define CSN_PIN 34
#define LDR_PIN 4

Servo myServo;

// Standard sea-level pressure in Pascals
const float SEA_LEVEL_PRESSURE = 101325.0;

void setup() {

  Serial.begin(115200);

  Wire.begin(8, 9);

  myServo.attach(2);

  pinMode(LDR_PIN, INPUT);

  // MPU6050 Initialization
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("MPU6050 init failed");
    while (1) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // BMP581 Initialization
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);

  while (bmp.beginSPI(CSN_PIN, 100000) != BMP5_OK) {
    Serial.println("BMP581 not connected");
    delay(1000);
  }

  // Wi-Fi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }

  // Firebase Configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Time Synchronization
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // Clear Existing Database Node
  if (Firebase.ready()) {

    if (Firebase.RTDB.deleteNode(&fbdo, "/lumisat/data")) {
      Serial.println("Firebase node cleared");
    }
  }
}

void loop() {

  struct tm timeinfo;

  String timestamp = "N/A";

  sensors_event_t accel, gyro, temp;

  bmp5_sensor_data bmpData = {0};

  mpu.getEvent(&accel, &gyro, &temp);

  bmp.getSensorData(&bmpData);

  if (getLocalTime(&timeinfo)) {

    char buf[20];

    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);

    timestamp = String(buf);
  }

  // Altitude Calculation
  float pressure = bmpData.pressure;

  float altitude =
      44330.0 * (1.0 - pow(pressure / SEA_LEVEL_PRESSURE, 0.1903));

  // Light Sensor Reading
  int lightValue = analogRead(LDR_PIN);

  if (Firebase.ready()) {

    FirebaseJson json;

    json.set("t", timestamp);

    json.set("ax", accel.acceleration.x);
    json.set("ay", accel.acceleration.y);
    json.set("az", accel.acceleration.z);

    json.set("gx", gyro.gyro.x);
    json.set("gy", gyro.gyro.y);
    json.set("gz", gyro.gyro.z);

    json.set("tmp", bmpData.temperature);

    json.set("p", pressure);

    json.set("alt", altitude);

    json.set("li", lightValue);

    Firebase.RTDB.pushJSON(
      &fbdo,
      "/lumisat/data",
      &json
    );
  }

  delay(50);
}
```
