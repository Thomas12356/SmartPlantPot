# SmartPlantPot

An IoT plant pot system that monitors a plant and helps keep it watered.

<img width="1013" height="701" alt="image" src="https://github.com/user-attachments/assets/9b67d1c8-c47e-4dd6-ba1a-6bf553e2e07c" />


## About

SmartPlantPot is built using an ESP32. It reads soil moisture, water level, temperature, and humidity, then shows the results on a local web page.

The system can also control a small water pump when the soil is dry.

## Features

* Soil moisture monitoring
* Water level monitoring
* Temperature and humidity readings
* Local web dashboard
* Automatic watering pump
* Warning LED
* Wi-Fi setup page
* Sensor error detection

## Hardware

* ESP32 / Adafruit Feather HUZZAH32
* AM2320 temperature and humidity sensor
* Soil moisture sensor
* Water level sensor
* I2C motor driver
* 5V water pump
* LED

## Software

* C++
* Arduino framework
* PlatformIO
* ESPAsyncWebServer
* HTML, CSS, and JavaScript

## How It Works

The ESP32 reads the sensors and converts the raw values into useful percentages.

The web dashboard shows the current plant status. A user can open the page on a phone or computer to check if the plant needs water.

If the soil moisture is too low and there is enough water available, the pump can turn on to water the plant.

## Dashboard

The dashboard shows:

* Soil moisture
* Water level
* Temperature
* Humidity
* Pump status
* Wi-Fi status
* System warnings

## Status

Working prototype completed.

## Future Improvements

* Add graphs for sensor history
* Add remote access
* Improve the case
* Add mobile notifications
* Support more than one plant

## Author

Thomas Eardley
