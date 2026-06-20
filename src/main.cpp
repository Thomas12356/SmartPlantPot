// te215@kent.ac.uk, comp6015

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>
#include <Preferences.h>

#include "app_state.h"

// -------------------- Esp AP WiFi Config (AP) -------------------- //

const char* SSID = "SmartPlantPot";
const char* WIFI_PASSWORD = "password";
const IPAddress AP_IP(192, 168, 10, 1);
const IPAddress AP_GATEWAY(192, 168, 10, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

// -------------------- Pin Config -------------------- //

const int SOIL_MOISTURE_PIN = 34;
const int WATER_LEVEL_PIN = 33;
const int LED_PIN = 14;

// -------------------- I2C Pin Config -------------------- //

const int I2C_SDA = 23;
const int I2C_SCL = 22;

// -------------------- I2C Address Config -------------------- //

const uint8_t PUMP_MOTOR_ADDRESS = 0x60;

// -------------------- 5V Water Pump Config -------------------- //

const int PUMP_SPEED = 40;  //0-63
const unsigned long MAX_PUMP_RUN_TIME = 5000;
const unsigned long PUMP_OFF_TIME = 10000;
const unsigned long MANUAL_PUMP_TEST_RUN_TIME = 1000;

// -------------------- System Config -------------------- //

int startWateringPercentage = 30;
int stopWateringPercentage = 40;

int lowWaterLevelPercentage = 20;
int goodWaterLevelPercentage = 30;

const unsigned long sensorUpdateInterval = 2000;
const unsigned long ledFlashInterval = 100;

const unsigned long WIFI_CONNECTION_ATTEMPT_TIMEOUT = 15000;
const unsigned long WIFI_CONNECTION_ATTEMPT_INTERVAL = 30000;

// -------------------- Preferences Init -------------------- //

Preferences preferences;

// -------------------- Web Server Init -------------------- //

AsyncWebServer server(80);

// -------------------- AM2320 Init -------------------- //

Adafruit_AM2320 am2320 = Adafruit_AM2320();
bool am2320Available = false;

// -------------------- Local WiFi (STA) -------------------- //

String savedSSID = "";
String savedWiFiPassword = "";

bool localWiFiConnected = false;
bool localWiFiConnecting = false;
bool localWiFiConnectPending = false;
bool accessPointRestartPending = false;

unsigned long localWiFiConnectionAttemptStartTime = 0;
unsigned long lastWiFiReconnectAttemptTime = 0;

// -------------------- Calibration -------------------- //

int dryCalibration = 4095;
int wetCalibration = 1265;

int emptyWaterCalibration = 0;
int fullWaterCalibration = 1800;
const int MIN_CALIBRATION_SPAN = 50;

// -------------------- Groove - Mini I2C Motor Driver V1.1 -------------------- //

const uint8_t CONTROL_REGISTER = 0x00;
const uint8_t FAULT_REGISTER = 0x01;
const uint8_t CLEAR_FAULT = 0x80;
const uint8_t STOP = 0x00;
const uint8_t ON = 0x02;

// -------------------- 24 Hour History -------------------- //

// Testing : 10UL * 1000UL Production : every 5 mins 5UL * 60UL * 1000UL
const unsigned long HISTORY_INTERVAL = 5UL * 60UL * 1000UL;
// save every 5 mins, last 24 hours -> 288 max
const int MAX_HISTORY = 288;

unsigned long lastHistory = 0;

struct History {
  unsigned long time;
  int soilMoisture;
  float temperature;
  float humidity;
};

// History Circular Memory
int historyPointer = 0;
int historyCount = 0;

History history[MAX_HISTORY];

// -------------------- State -------------------- //

unsigned long lastSensorUpdateTime = 0;
unsigned long lastLedFlashTime = 0;

SystemState latestState = {
  0,
  -1,
  false,
  -1,
  false,
  NAN,
  NAN,
  false,
  true
};

// -------------------- LED State -------------------- //

bool isLedOn = false;
bool ledFlashState = false;

// -------------------- Pump State -------------------- //

bool isPumpOn = false;
bool pumpFault = false;
bool manualPumpTestRunning = false;
unsigned long pumpStartTime = 0;
unsigned long lastPumpStopTime = 0;
String pumpStatusString = "Pump off";


// ---------- Calibration Functions ---------- //

void LoadCalibrationConfig() {
  preferences.begin("calibration", true);

  dryCalibration = preferences.getInt("dryCalibration", dryCalibration);
  wetCalibration = preferences.getInt("wetCalibration", wetCalibration);
  emptyWaterCalibration = preferences.getInt("emptyWaterCalibration", emptyWaterCalibration);
  fullWaterCalibration = preferences.getInt("fullWaterCalibration", fullWaterCalibration);

  preferences.end();
}

void SaveCalibrationConfig() {
  preferences.begin("calibration", false);

  preferences.putInt("dryCalibration", dryCalibration);
  preferences.putInt("wetCalibration", wetCalibration);
  preferences.putInt("emptyWaterCalibration", emptyWaterCalibration);
  preferences.putInt("fullWaterCalibration", fullWaterCalibration);

  preferences.end();
}

// -------------------- WiFi Functions -------------------- //

void LoadLocalWiFiConfig() {
  preferences.begin("wifi", true);

  savedSSID = preferences.getString("ssid", "");
  savedWiFiPassword = preferences.getString("password", "");

  preferences.end();
}

void SaveLocalWiFiConfig(String newSSID, String newWiFiPassword) {
  preferences.begin("wifi", false);

  preferences.putString("ssid", newSSID);
  preferences.putString("password", newWiFiPassword);

  preferences.end();

  savedSSID = newSSID;
  savedWiFiPassword = newWiFiPassword;
}

void ClearLocalWifiConfig() {
  preferences.begin("wifi", false);

  preferences.remove("ssid");
  preferences.remove("password");

  preferences.end();

  savedSSID = "";
  savedWiFiPassword = "";
}

void attemptLocalWiFi() {
  if (savedSSID.length() == 0) {
    localWiFiConnected = false;
    localWiFiConnecting = false;
    return;
  }

  Serial.print("Attempting to connect to local WiFi: ");
  Serial.println(savedSSID);

  localWiFiConnected = false;
  localWiFiConnecting = true;

  localWiFiConnectionAttemptStartTime = millis();
  lastWiFiReconnectAttemptTime = millis();

  WiFi.begin(savedSSID.c_str(), savedWiFiPassword.c_str());
}

void StartAccessPoint() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);

  WiFi.softAP(SSID, WIFI_PASSWORD);

  Serial.println("ESP WiFi AP is active ");

  Serial.print("ESP WiFi SSID: ");
  Serial.println(SSID);

  Serial.print("ESP WiFi IP Address: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("ESP WiFi Password: ");
  Serial.println(WIFI_PASSWORD);
}

void WiFiSetup() {
  StartAccessPoint();

  LoadLocalWiFiConfig();

  if (savedSSID.length() > 0) {
    localWiFiConnectPending = true;
  } else {
    Serial.println("No saved local WiFi. (STA)");
  }
}

void HandleWifiConnection() {
  if (accessPointRestartPending) {
    accessPointRestartPending = false;
    WiFi.disconnect(false, false);
    StartAccessPoint();
  }

  if (localWiFiConnectPending) {
    localWiFiConnectPending = false;
    attemptLocalWiFi();
  }

  if (savedSSID.length() == 0) {
    localWiFiConnected = false;
    localWiFiConnecting = false;
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!localWiFiConnected) {
      Serial.println("Connected to local WiFi!");
      Serial.print("Local WiFi IP Address: ");
      Serial.println(WiFi.localIP());
    }

    localWiFiConnected = true;
    localWiFiConnecting = false;
    return;
  }

  localWiFiConnected = false;

  if (localWiFiConnecting) {
    if (millis() - localWiFiConnectionAttemptStartTime > WIFI_CONNECTION_ATTEMPT_TIMEOUT) {
      Serial.println("Local WiFi connection attempt timed out.");
      localWiFiConnecting = false;
      accessPointRestartPending = true;
    }

    return;
  }

  if (millis() - lastWiFiReconnectAttemptTime >= WIFI_CONNECTION_ATTEMPT_INTERVAL) {
    Serial.println("Connection Failed, trying again in the background.");
    attemptLocalWiFi();
  }
}

// -------------------- General Sensor Functions -------------------- //

String EscapeJsonString(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);

    switch (c) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if ((uint8_t)c < 0x20) {
          char buffer[7];
          snprintf(buffer, sizeof(buffer), "\\u%04x", (uint8_t)c);
          escaped += buffer;
        } else {
          escaped += c;
        }
        break;
    }
  }

  return escaped;
}

int AnalogReadAverage(int pin) {
  long sum = 0;

  for (int i = 0; i < 20; i++) {
    sum += analogRead(pin);
    delay(5);
  }

  return sum / 20;
}

// -------------------- Soil Sensor Functions -------------------- //

int GetSoilMoisturePercentage() {
  int averageSoilMoistureRaw = AnalogReadAverage(SOIL_MOISTURE_PIN);

  if (dryCalibration - wetCalibration < MIN_CALIBRATION_SPAN) {
    return -1;
  }

  int percentage = (dryCalibration - averageSoilMoistureRaw) * 100 / (dryCalibration - wetCalibration);
  percentage = constrain(percentage, 0, 100);

  Serial.print("Soil raw: ");
  Serial.print(averageSoilMoistureRaw);
  Serial.print(" Moisture: ");
  Serial.print(percentage);
  Serial.println("%");

  return percentage;
}

bool UpdateSoilDryState(int moisturePercentage) {
  if (moisturePercentage < 0) {
    return latestState.soilIsDry;
  }

  if (!latestState.soilIsDry && moisturePercentage < startWateringPercentage) {
    latestState.soilIsDry = true;
  }

  if (latestState.soilIsDry && moisturePercentage > stopWateringPercentage) {
    latestState.soilIsDry = false;
  }

  return latestState.soilIsDry;
}

// -------------------- Water Level Sensor Functions -------------------- //

int GetWaterLevelPercentage() {
  int averageWaterLevelRaw = AnalogReadAverage(WATER_LEVEL_PIN);

  if (fullWaterCalibration - emptyWaterCalibration < MIN_CALIBRATION_SPAN) {
    return -1;
  }

  int percentage = (averageWaterLevelRaw - emptyWaterCalibration) * 100 / (fullWaterCalibration - emptyWaterCalibration);

  percentage = constrain(percentage, 0, 100);

  Serial.print("Water raw: ");
  Serial.print(averageWaterLevelRaw);
  Serial.print(" Water level: ");
  Serial.print(percentage);
  Serial.println("%");

  return percentage;
}

bool UpdateWaterLevelState(int waterLevelPercentage) {
  if (waterLevelPercentage < 0) {
    return latestState.waterIsLow;
  }

  if (!latestState.waterIsLow && waterLevelPercentage < lowWaterLevelPercentage) {
    latestState.waterIsLow = true;
  }

  if (latestState.waterIsLow && waterLevelPercentage > goodWaterLevelPercentage) {
    latestState.waterIsLow = false;
  }

  return latestState.waterIsLow;
}

// -------------------- AM2320 Functions -------------------- //

bool ReadAM2320(float &temperature, float &humidity) {
  temperature = am2320.readTemperature();
  humidity = am2320.readHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    return true;
  }

  // ? Is this still needed ??
  delay(100);

  temperature = am2320.readTemperature();
  humidity = am2320.readHumidity();

  return !isnan(temperature) && !isnan(humidity);
}

// -------------------- Pump Functions -------------------- //

bool MotorWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(PUMP_MOTOR_ADDRESS);
  Wire.write(reg);
  Wire.write(value);

  byte error = Wire.endTransmission();

  if (error != 0) {
    Serial.print("Pump write error: ");
    Serial.println(error);
    return false;
  }

  return true;
}

bool SetPumpMotor(int speed, uint8_t mode) {
  speed = constrain(speed, 0, 63);

  uint8_t value = speed << 2;
  value |= mode;

  return MotorWrite(CONTROL_REGISTER, value);
}

bool PumpOn() {
  if (isPumpOn) {
    pumpStatusString = "Pump already running";
    return true;
  }

  if (!MotorWrite(FAULT_REGISTER, CLEAR_FAULT)) {
    isPumpOn = false;
    pumpFault = true;
    pumpStatusString = "Pump fault: motor driver write failed";
    Serial.println("Pump ON failed");
    return false;
  }

  if (!SetPumpMotor(PUMP_SPEED, ON)) {
    isPumpOn = true;
    pumpFault = true;
    pumpStartTime = millis();
    pumpStatusString = "Pump fault: start command uncertain";
    Serial.println("Pump ON uncertain, attempting to stop");
    return false;
  }

  isPumpOn = true;
  pumpFault = false;
  pumpStartTime = millis();
  pumpStatusString = "Pump running";

  Serial.println("Pump ON");
  return true;
}

bool PumpOff() {
  if (!SetPumpMotor(0, STOP)) {
    pumpFault = true;
    pumpStatusString = "Pump fault: failed to stop motor";
    Serial.println("Pump OFF failed");
    return false;
  }

  if (isPumpOn) {
    lastPumpStopTime = millis();
  }

  isPumpOn = false;
  manualPumpTestRunning = false;

  Serial.println("Pump OFF");
  return true;
}

void HandlePumpControl() {
  if (pumpFault) {
    if (isPumpOn) {
      PumpOff();
    }

    return;
  }

  if (isPumpOn) {
    if (manualPumpTestRunning) {
      if (latestState.waterLevel < 0) {
        if (PumpOff()) {
          pumpStatusString = "Manual test stopped: water level sensor error";
        }
        return;
      }

      if (latestState.waterIsLow) {
        if (PumpOff()) {
          pumpStatusString = "Manual test stopped: water level low";
        }
        return;
      }

      if (millis() - pumpStartTime >= MANUAL_PUMP_TEST_RUN_TIME) {
        if (PumpOff()) {
          pumpStatusString = "Manual pump test complete";
        }
        return;
      }

      pumpStatusString = "Manual pump test running";
      return;
    }

    if (millis() - pumpStartTime >= MAX_PUMP_RUN_TIME) {
      if (PumpOff()) {
        pumpStatusString = "Pump stopped: max run time";
      }
      return;
    }

    if (latestState.sensorError) {
      if (PumpOff()) {
        pumpStatusString = "Pump stopped: soil/water sensor error";
      }
      return;
    }

    if (latestState.waterIsLow) {
      if (PumpOff()) {
        pumpStatusString = "Pump stopped: water level low";
      }
      return;
    }

    if (!latestState.soilIsDry) {
      if (PumpOff()) {
        pumpStatusString = "Pump stopped: soil moisture is good";
      }
      return;
    }

    pumpStatusString = "Pump running: soil is too dry";
    return;
  }

  if (latestState.sensorError) {
    pumpStatusString = "Pump blocked: soil/water sensor error";
    return;
  }

  if (latestState.waterIsLow) {
    pumpStatusString = "Pump Error: water level low";
    return;
  }

  if (!latestState.soilIsDry) {
    pumpStatusString = "Pump off: soil moisture is Good";
    return;
  }

  if (millis() - lastPumpStopTime < PUMP_OFF_TIME) {
    pumpStatusString = "Pump paused";
    return;
  }

  if (PumpOn()) {
    pumpStatusString = "Pump running: soil is too dry";
  }
}

// -------------------- Status / Error Functions -------------------- //

bool GetSensorErrorState() {
  if (latestState.soilMoisture < 0) {
    return true;
  }

  if (latestState.waterLevel < 0) {
    return true;
  }

  return false;
}

String GetSystemStatusText() {
  if (pumpFault) {
    return "Pump fault";
  }

  if (latestState.soilMoisture < 0 || latestState.waterLevel < 0) {
    return "Calibration error";
  }

  if (latestState.waterIsLow) {
    return "Water reservoir is low";
  }

  if (latestState.soilIsDry) {
    return "Soil is dry";
  }

  if (!latestState.am2320Ok) {
    return "Temperature and humidity sensor warning";
  }

  return "Soil moisture is Good";
}

// ! Must be after global variables and GetSystemStatusText()
#include "web_pages.h" // imports the web_page code.

// -------------------- LED Functions -------------------- //

void UpdateLED() {
  if (latestState.sensorError) {
    if (millis() - lastLedFlashTime >= ledFlashInterval) {
      lastLedFlashTime = millis();
      ledFlashState = !ledFlashState;

      digitalWrite(LED_PIN, ledFlashState ? HIGH : LOW);
      isLedOn = ledFlashState;
    }

    return;
  }

  if (!latestState.am2320Ok || latestState.waterIsLow || latestState.soilIsDry || pumpFault) {
    isLedOn = true;
    ledFlashState = false;
    digitalWrite(LED_PIN, HIGH);
    return;
  }

  isLedOn = false;
  ledFlashState = false;
  digitalWrite(LED_PIN, LOW);
}

// -------------------- Sensor Update Function -------------------- //

void UpdateSensorReadings() {
  latestState.TimeStampMs = millis();

  latestState.soilMoisture = GetSoilMoisturePercentage();
  latestState.soilIsDry = UpdateSoilDryState(latestState.soilMoisture);

  latestState.waterLevel = GetWaterLevelPercentage();
  latestState.waterIsLow = UpdateWaterLevelState(latestState.waterLevel);

  latestState.am2320Ok = ReadAM2320( latestState.temperature, latestState.humidity);

  Serial.print("Temperature: ");
  if (latestState.am2320Ok) {
    Serial.print(latestState.temperature);
    Serial.print("C | Humidity: ");
    Serial.print(latestState.humidity);
    Serial.println("%");
  } else {
    Serial.println("AM2320 read failed");
  }

  latestState.sensorError = GetSensorErrorState();

  Serial.print("Sensor error: ");
  Serial.println(latestState.sensorError ? "YES" : "NO");

  Serial.print("AM2320 warning: ");
  Serial.println(latestState.am2320Ok ? "NO" : "YES");

  Serial.print("Warning LED: ");
  if (latestState.sensorError) {
    Serial.println("FLASHING");
  } else {
    Serial.println(isLedOn ? "ON" : "OFF");
  }

  Serial.print("Status: ");
  Serial.println(GetSystemStatusText());
}

// -------------------- 24 Hour History Functions -------------------- //

void AddHistory() {
  History& newHistory = history[historyPointer];

  newHistory.time = millis();
  newHistory.soilMoisture = latestState.soilMoisture;
  newHistory.temperature = latestState.temperature;
  newHistory.humidity = latestState.humidity;

  historyPointer = (historyPointer + 1) % MAX_HISTORY;

  if (historyCount < MAX_HISTORY) {
    historyCount++;
  }

  Serial.print("New History saved. Total History count: ");
  Serial.println(historyCount);
}

String BuildHistoryJson() {
  String json;
  json.reserve(JSON_RESERVE_SIZE);

  json = "{";

  json += "\"sample_interval_minutes\":";
  json += String(HISTORY_INTERVAL / 60000UL);
  json += ",";

  json += "\"max_points\":";
  json += String(MAX_HISTORY);
  json += ",";

  json += "\"stored_points\":";
  json += String(historyCount);
  json += ",";

  json += "\"points\":[";

  int startIndex = historyPointer - historyCount;

  if (startIndex < 0) {
    startIndex += MAX_HISTORY;
  }

  for (int i = 0; i < historyCount; i++) {
    int index = (startIndex + i) % MAX_HISTORY;
    History& historyInstance = history[index];

    if (i > 0) {
      json += ",";
    }

    unsigned long ageMinutes = (millis() - historyInstance.time) / 60000UL;

    json += "{";

    json += "\"age_minutes\":";
    json += String(ageMinutes);
    json += ",";

    json += "\"soil_moisture\":";
    if (historyInstance.soilMoisture >= 0) {
      json += String(historyInstance.soilMoisture);
    } else {
      json += "null";
    }
    json += ",";

    json += "\"temperature\":";
    if (!isnan(historyInstance.temperature)) {
      json += String(historyInstance.temperature, 1);
    } else {
      json += "null";
    }
    json += ",";

    json += "\"humidity\":";
    if (!isnan(historyInstance.humidity)) {
      json += String(historyInstance.humidity, 1);
    } else {
      json += "null";
    }

    json += "}";
  }

  json += "]";
  json += "}";

  return json;
}

// -------------------- JSON Function -------------------- //

String BuildStatusJson() {
  String json;
  json.reserve(JSON_RESERVE_SIZE);

  json = "{";

  json += "\"uptime_ms\":";
  json += String(millis());
  json += ",";

  json += "\"last_reading_ms\":";
  json += String(latestState.TimeStampMs);
  json += ",";

  json += "\"setup_wifi_ip\":\"";
  json += WiFi.softAPIP().toString();
  json += "\",";

  json += "\"home_wifi_connected\":";
  json += WiFi.status() == WL_CONNECTED ? "true" : "false";
  json += ",";

  json += "\"home_wifi_connecting\":";
  json += localWiFiConnecting ? "true" : "false";
  json += ",";

  json += "\"home_wifi_ssid\":\"";
  if (WiFi.status() == WL_CONNECTED) {
    json += EscapeJsonString(WiFi.SSID());
  }
  json += "\",";

  json += "\"home_wifi_ip\":\"";
  if (WiFi.status() == WL_CONNECTED) {
    json += WiFi.localIP().toString();
  }
  json += "\",";

  json += "\"soil_moisture\":";
  json += String(latestState.soilMoisture);
  json += ",";

  json += "\"soil_is_dry\":";
  json += latestState.soilIsDry ? "true" : "false";
  json += ",";

  json += "\"water_level\":";
  json += String(latestState.waterLevel);
  json += ",";

  json += "\"water_low\":";
  json += latestState.waterIsLow ? "true" : "false";
  json += ",";

  json += "\"temperature\":";
  if (latestState.am2320Ok) {
    json += String(latestState.temperature, 1); // 1 db
  } else {
    json += "null";
  }
  json += ",";

  json += "\"humidity\":";
  if (latestState.am2320Ok) {
    json += String(latestState.humidity, 1); // 1 db
  } else {
    json += "null";
  }
  json += ",";

  json += "\"am2320_ok\":";
  json += latestState.am2320Ok ? "true" : "false";
  json += ",";

  json += "\"sensor_error\":";
  json += latestState.sensorError ? "true" : "false";
  json += ",";

  json += "\"warning_led_on\":";
  json += isLedOn ? "true" : "false";
  json += ",";

  json += "\"warning_led_mode\":\"";
  if (latestState.sensorError) {
    json += "flashing";
  } else if (isLedOn) {
    json += "solid";
  } else {
    json += "off";
  }
  json += "\",";

  json += "\"pump_running\":";
  json += isPumpOn ? "true" : "false";
  json += ",";

  json += "\"pump_fault\":";
  json += pumpFault ? "true" : "false";
  json += ",";

  json += "\"pump_status\":\"";
  json += EscapeJsonString(pumpStatusString);
  json += "\",";

  json += "\"pump_speed\":";
  json += String(PUMP_SPEED);
  json += ",";

  json += "\"history_points\":";
  json += String(historyCount);
  json += ",";

  json += "\"history_sample_interval_minutes\":";
  json += String(HISTORY_INTERVAL / 60000UL);
  json += ",";

  json += "\"status\":\"";
  json += EscapeJsonString(GetSystemStatusText());
  json += "\",";

  json += "\"config\":{";

  json += "\"start_watering\":";
  json += String(startWateringPercentage);
  json += ",";

  json += "\"stop_watering\":";
  json += String(stopWateringPercentage);
  json += ",";

  json += "\"low_water_level\":";
  json += String(lowWaterLevelPercentage);
  json += ",";

  json += "\"water_level_ok\":";
  json += String(goodWaterLevelPercentage);
  json += ",";

  json += "\"dry_cal\":";
  json += String(dryCalibration);
  json += ",";

  json += "\"wet_cal\":";
  json += String(wetCalibration);
  json += ",";

  json += "\"empty_water_cal\":";
  json += String(emptyWaterCalibration);
  json += ",";

  json += "\"full_water_cal\":";
  json += String(fullWaterCalibration);

  json += "}";

  json += "}";

  return json;
}

// -------------------- Webserver Setup -------------------- //

void SetupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", BuildRootPageHtml());
  });

  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", BuildHistoryPageHtml());
  });

  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", BuildWifiPageHtml());
  });

  server.on("/calibration", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", BuildCalibrationPageHtml());
  });

  server.on("/calibration/soil-dry", HTTP_POST, [](AsyncWebServerRequest *request) {
    dryCalibration = AnalogReadAverage(SOIL_MOISTURE_PIN);
    SaveCalibrationConfig();
    request->send(200, "text/html", BuildCalibrationSavedHtml("Soil dry value saved"));
  });

  server.on("/calibration/soil-wet", HTTP_POST, [](AsyncWebServerRequest *request) {
    wetCalibration = AnalogReadAverage(SOIL_MOISTURE_PIN);
    SaveCalibrationConfig();
    request->send(200, "text/html", BuildCalibrationSavedHtml("Soil wet value saved"));
  });

  server.on("/calibration/water-empty", HTTP_POST, [](AsyncWebServerRequest *request) {
    emptyWaterCalibration = AnalogReadAverage(WATER_LEVEL_PIN);
    SaveCalibrationConfig();
    request->send(200, "text/html", BuildCalibrationSavedHtml("Empty water value saved"));
  });

  server.on("/calibration/water-full", HTTP_POST, [](AsyncWebServerRequest *request) {
    fullWaterCalibration = AnalogReadAverage(WATER_LEVEL_PIN);
    SaveCalibrationConfig();
    request->send(200, "text/html", BuildCalibrationSavedHtml("Full water value saved"));
  });

  server.on("/wifi/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true)) {
      request->send(400, "text/plain", "Missing SSID");
      return;
    }

    String newSSID = request->getParam("ssid", true)->value();
    String newPassword = "";

    if (request->hasParam("password", true)) {
      newPassword = request->getParam("password", true)->value();
    }

    newSSID.trim();

    if (newSSID.length() == 0) {
      request->send(400, "text/plain", "SSID cannot be empty");
      return;
    }

    SaveLocalWiFiConfig(newSSID, newPassword);
    localWiFiConnectPending = true;

    request->send(200, "text/html", BuildWifiConnectingHtml());
  });

  server.on("/wifi/forget", HTTP_POST, [](AsyncWebServerRequest *request) {
    ClearLocalWifiConfig();

    localWiFiConnectPending = false;
    localWiFiConnected = false;
    localWiFiConnecting = false;
    accessPointRestartPending = true;

    request->send(200, "text/html", BuildWifiForgottenHtml());
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", BuildStatusJson());
  });

  server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", BuildHistoryJson());
  });

  server.on("/pump/test", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (pumpFault) {
      pumpStatusString = "Manual test blocked: pump fault";
      request->send(200, "text/html", BuildPumpResultHtml("Pump test blocked: pump fault"));
      return;
    }

    if (isPumpOn) {
      pumpStatusString = "Manual test ignored: pump already running";
      request->send(200, "text/html", BuildPumpResultHtml("Pump is already running"));
      return;
    }

    if (latestState.waterLevel < 0) {
      pumpStatusString = "Manual test blocked: water level sensor error";
      request->send(200, "text/html", BuildPumpResultHtml("Pump test blocked: water level sensor error"));
      return;
    }

    if (latestState.waterIsLow) {
      pumpStatusString = "Manual test blocked: water level low";
      request->send(200, "text/html", BuildPumpResultHtml("Pump test blocked: water level low"));
      return;
    }

    if (PumpOn()) {
      manualPumpTestRunning = true;
      pumpStatusString = "Manual pump test running";
      request->send(200, "text/html", BuildPumpResultHtml("Pump test started: short pulse"));
    } else {
      request->send(500, "text/html", BuildPumpResultHtml("Pump test failed: motor driver write failed"));
    }
  });

  server.on("/pump/off", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (PumpOff()) {
      pumpFault = false;
      pumpStatusString = "Pump manually stopped";
    }

    request->send(200, "text/html", BuildPumpResultHtml(pumpStatusString));
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
}

// -------------------- Setup -------------------- //

void setup() {
  Serial.begin(115200);
  Serial.println("Connected to Serial!");

  analogReadResolution(12);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  am2320Available = am2320.begin();

  if (am2320Available) {
    Serial.println("AM2320 detected.");
  } else {
    Serial.println("AM2320 not detected.");
  }

  LoadCalibrationConfig();

  WiFiSetup();
  Serial.println("WiFi Running!");

  SetupWebServer();
  Serial.println("Async Web Server Running!");

  UpdateSensorReadings();
  AddHistory();
  lastHistory = millis();


}

// -------------------- Main Loop -------------------- //

void loop() {
  UpdateLED();

  HandleWifiConnection();

  HandlePumpControl();

  if (millis() - lastSensorUpdateTime >= sensorUpdateInterval) {
    lastSensorUpdateTime = millis();
    UpdateSensorReadings();

    if (millis() - lastHistory >= HISTORY_INTERVAL) {
      lastHistory = millis();
      AddHistory();
    }
  }
}
