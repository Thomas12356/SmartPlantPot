# SmartPlantPot

An IoT plant pot system that monitors a plant and helps keep it watered.

<img width="134.72" height="240" alt="SmartPlantPot prototype" src="https://github.com/user-attachments/assets/3a9313e8-80b1-45fb-8dbc-7015405a2105" />

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
