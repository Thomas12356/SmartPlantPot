#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>

// -------------------- String Reserve Sizes -------------------- //

const size_t HTML_RESERVE_SIZE = 30000;
const size_t JSON_RESERVE_SIZE = 25000;

// -------------------- Shared State Struct -------------------- //

struct SystemState {
  unsigned long TimeStampMs;

  int soilMoisture;
  bool soilIsDry;

  int waterLevel;
  bool waterIsLow;

  float temperature;
  float humidity;
  bool am2320Ok;

  bool sensorError;
};

// -------------------- Shared Variables From main.cpp -------------------- //

// WiFi
extern const char* SSID;
extern const char* WIFI_PASSWORD;
extern String savedSSID;
extern String savedWiFiPassword;
extern bool localWiFiConnected;
extern bool localWiFiConnecting;

// Current sensor state
extern SystemState latestState;
extern bool isLedOn;

// Calibration values
extern int dryCalibration;
extern int wetCalibration;
extern int emptyWaterCalibration;
extern int fullWaterCalibration;

// Config
extern int startWateringPercentage;
extern int stopWateringPercentage;
extern int lowWaterLevelPercentage;
extern int goodWaterLevelPercentage;

// Pins used by the calibration page
extern const int SOIL_MOISTURE_PIN;
extern const int WATER_LEVEL_PIN;

// Pump state
extern bool isPumpOn;
extern bool pumpFault;
extern String pumpStatusString;

// -------------------- Shared Functions From main.cpp -------------------- //

String GetSystemStatusText();
int AnalogReadAverage(int pin);

#endif
