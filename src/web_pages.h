#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>
#include <WiFi.h>
#include "app_state.h"

String EscapeHtml(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);

    switch (c) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&#39;";
        break;
      default:
        escaped += c;
        break;
    }
  }

  return escaped;
}

String BuildPageStart(String title) {
  String html;
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
)rawliteral";

  html += "<title>";
  html += EscapeHtml(title);
  html += "</title>";

  html += R"rawliteral(
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 15px;
      background: #e6e6e6;
    }

    .card {
      background: white;
      padding: 15px;
      margin-bottom: 15px;
      border-radius: 10px;
    }

    h1, h2 {
      color: #006d10;
    }

    a, button {
      background: #006d10;
      color: white;
      padding: 10px;
      border: none;
      border-radius: 5px;
      text-decoration: none;
      display: inline-block;
      margin: 4px 0;
    }

    input {
      width: 100%;
      padding: 10px;
      box-sizing: border-box;
    }

    svg {
      width: 100%;
      max-width: 700px;
      height: auto;
      display: block;
      border: 1px solid #006d10;
      background: white;
    }
  </style>
</head>
<body>
)rawliteral";

  return html;
}

String BuildPageEnd() {
  return R"rawliteral(
</body>
</html>
)rawliteral";
}

// -------------------- WiFi Setup Page -------------------- //

String BuildWifiPageHtml() {
  String html = BuildPageStart("WiFi Setup");
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(
<div class='card'>
  <h1>WiFi Setup</h1>
  <p><a href='/'>Back</a></p>
</div>

<div class='card'>
  <h2>Status</h2>
  <p><strong>Setup WiFi:</strong> )rawliteral";

  html += EscapeHtml(String(SSID));

  html += R"rawliteral(</p>
  <p><strong>Setup IP:</strong> )rawliteral";

  html += WiFi.softAPIP().toString();

  html += R"rawliteral(</p>
  <p><strong>Local WiFi:</strong> )rawliteral";

  if (WiFi.status() == WL_CONNECTED) {
    html += "Connected</p>";

    html += R"rawliteral(
  <p><strong>Local SSID:</strong> )rawliteral";

    html += EscapeHtml(WiFi.SSID());

    html += R"rawliteral(</p>
  <p><strong>Local IP:</strong> )rawliteral";

    html += WiFi.localIP().toString();

    html += "</p>";
  } else {
    html += "Not connected</p>";

    if (savedSSID.length() > 0) {
      html += R"rawliteral(
  <p><strong>Saved SSID:</strong> )rawliteral";

      html += EscapeHtml(savedSSID);
      html += "</p>";
    }
  }

  html += R"rawliteral(
</div>

<div class='card'>
  <h2>Connect to Local WiFi</h2>
  <form method='POST' action='/wifi/save'>
    <p>
      <strong>SSID:</strong><br>
      <input type='text' name='ssid' required>
    </p>

    <p>
      <strong>Password:</strong><br>
      <input type='password' name='password'>
    </p>

    <p><button type='submit'>Save and Connect</button></p>
  </form>
</div>

<div class='card'>
  <h2>Forget Local WiFi</h2>
  <form method='POST' action='/wifi/forget'>
    <button type='submit'>Forget Local WiFi</button>
  </form>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- WiFi Connecting Page -------------------- //

String BuildWifiConnectingHtml() {
  String html;
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <meta http-equiv='refresh' content='5;url=/wifi'>
  <title>WiFi Saved</title>
</head>
<body>
  <h1>WiFi details saved</h1>
  <p>The ESP32 is now trying to connect in the background.</p>
  <p>This page will refresh back to the WiFi page in 5 seconds.</p>
  <p><a href='/'>Dashboard</a></p>
  <p><a href='/wifi'>WiFi setup</a></p>
</body>
</html>
)rawliteral";

  return html;
}

// -------------------- WiFi Result Page -------------------- //

String BuildWifiResultHtml(bool connected) {
  String html = BuildPageStart("WiFi Result");
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(
<div class='card'>
  <h1>WiFi Result</h1>
)rawliteral";

  if (connected) {
    html += R"rawliteral(
  <p><strong>Connected successfully.</strong></p>
  <p><strong>SSID:</strong> )rawliteral";

    html += EscapeHtml(WiFi.SSID());

    html += R"rawliteral(</p>
  <p><strong>IP:</strong> )rawliteral";

    html += WiFi.localIP().toString();
    html += "</p>";
  } else {
    html += R"rawliteral(
  <p><strong>Failed to connect.</strong></p>
)rawliteral";
  }

  html += R"rawliteral(
  <p><a href='/wifi'>Back to WiFi setup</a></p>
  <p><a href='/'>Back to dashboard</a></p>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- WiFi Forgotten Page -------------------- //

String BuildWifiForgottenHtml() {
  String html = BuildPageStart("WiFi Forgotten");
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(
<div class='card'>
  <h1>WiFi Forgotten</h1>
  <p>Saved WiFi details have been removed.</p>
  <p><a href='/wifi'>Back to WiFi setup</a></p>
  <p><a href='/'>Back to dashboard</a></p>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- Calibration Pages -------------------- //

String BuildCalibrationSavedHtml(String message) {
  String html = BuildPageStart("Calibration Saved");
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(
<div class='card'>
  <h1>Calibration Saved</h1>
  <p><strong>)rawliteral";

  html += EscapeHtml(message);

  html += R"rawliteral(</strong></p>
  <p><a href='/calibration'>Back to calibration</a></p>
  <p><a href='/'>Back to dashboard</a></p>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

String BuildCalibrationPageHtml() {
  int currentSoilRaw = AnalogReadAverage(SOIL_MOISTURE_PIN);
  int currentWaterRaw = AnalogReadAverage(WATER_LEVEL_PIN);

  String html = BuildPageStart("Calibration");
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(
<div class='card'>
  <h1>Calibration</h1>
  <p>Save current sensor readings as calibration values.</p>
  <p><a href='/'>Back to Dashboard</a></p>
</div>

<div class='card'>
  <h2>Current Readings</h2>
  <p><strong>Current soil:</strong> )rawliteral";

  html += String(currentSoilRaw);

  html += R"rawliteral(</p>
  <p><strong>Current water:</strong> )rawliteral";

  html += String(currentWaterRaw);

  html += R"rawliteral(</p>
</div>

<div class='card'>
  <h2>Soil Calibration</h2>
  <p><strong>Saved dry value:</strong> )rawliteral";

  html += String(dryCalibration);

  html += R"rawliteral(</p>
  <p><strong>Saved wet value:</strong> )rawliteral";

  html += String(wetCalibration);

  html += R"rawliteral(</p>
  <form method='POST' action='/calibration/soil-dry'>
    <button type='submit'>Save Current Soil as Dry</button>
  </form>
  <form method='POST' action='/calibration/soil-wet'>
    <button type='submit'>Save Current Soil as Wet</button>
  </form>
</div>

<div class='card'>
  <h2>Water Level Calibration</h2>
  <p><strong>Saved empty value:</strong> )rawliteral";

  html += String(emptyWaterCalibration);

  html += R"rawliteral(</p>
  <p><strong>Saved full value:</strong> )rawliteral";

  html += String(fullWaterCalibration);

  html += R"rawliteral(</p>
  <form method='POST' action='/calibration/water-empty'>
    <button type='submit'>Save Current Water as Empty</button>
  </form>
  <form method='POST' action='/calibration/water-full'>
    <button type='submit'>Save Current Water as Full</button>
  </form>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- Pump Result Page -------------------- //

String BuildPumpResultHtml(String message) {
  String html = BuildPageStart("Pump Control");
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(
<div class='card'>
  <h1>Pump Control</h1>
  <p><strong>)rawliteral";

  html += EscapeHtml(message);

  html += R"rawliteral(</strong></p>
  <p><a href='/'>Back to dashboard</a></p>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- Dashboard Page -------------------- //

String BuildRootPageHtml() {
  String html = BuildPageStart("Smart Plant Pot");
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(
<div class='card'>
  <h1>Smart Plant Pot</h1>
  <p>Comp6015</p>
  <p>Thomas Eardley : te215@kent.ac.uk</p>
  <p><a href='/wifi'>WiFi Setup</a></p>
  <p><a href='/history'>24 Hour History</a></p>
  <p><a href='/calibration'>Calibration</a></p>
</div>

<div class='card'>
  <h2>Network</h2>
  <p><strong>Setup IP:</strong> )rawliteral";

  html += WiFi.softAPIP().toString();

  html += R"rawliteral(</p>
  <p><strong>Local WiFi:</strong> )rawliteral";

  if (WiFi.status() == WL_CONNECTED) {
    html += "Connected</p>";

    html += R"rawliteral(
  <p><strong>Local SSID:</strong> )rawliteral";

    html += EscapeHtml(WiFi.SSID());

    html += R"rawliteral(</p>
  <p><strong>Local IP:</strong> )rawliteral";

    html += WiFi.localIP().toString();
    html += "</p>";
  } else {
    html += "Not connected</p>";
  }

  html += R"rawliteral(
</div>

<div class='card'>
  <h2>Plant Sensor Readings</h2>
  <p><strong>Soil Moisture:</strong> )rawliteral";

  if (latestState.soilMoisture < 0) {
    html += "Calibration error";
  } else {
    html += String(latestState.soilMoisture);
    html += "%";
  }

  html += R"rawliteral(</p>
  <p><strong>Water Level:</strong> )rawliteral";

  if (latestState.waterLevel < 0) {
    html += "Calibration error";
  } else {
    html += String(latestState.waterLevel);
    html += "%";
  }

  html += R"rawliteral(</p>
  <p><strong>Temperature:</strong> )rawliteral";

  if (!latestState.am2320Ok) {
    html += "Sensor error";
  } else {
    html += String(latestState.temperature, 1);
    html += " &deg;C";
  }

  html += R"rawliteral(</p>
  <p><strong>Humidity:</strong> )rawliteral";

  if (!latestState.am2320Ok) {
    html += "Sensor error";
  } else {
    html += String(latestState.humidity, 1);
    html += "%";
  }

  html += R"rawliteral(</p>
</div>

<div class='card'>
  <h2>Status</h2>
  <p><strong>)rawliteral";

  html += EscapeHtml(GetSystemStatusText());

  html += R"rawliteral(</strong></p>
  <p><strong>LED:</strong> )rawliteral";

  if (latestState.sensorError) {
    html += "FLASHING - sensor error";
  } else if (isLedOn) {
    html += "ON";
  } else {
    html += "OFF";
  }

  html += R"rawliteral(</p>
  <p><strong>Last Reading:</strong> )rawliteral";

  html += String(latestState.TimeStampMs);

  html += R"rawliteral( ms after startup</p>
</div>

<div class='card'>
  <h2>Pump</h2>
  <p><strong>Pump:</strong> )rawliteral";

  html += isPumpOn ? "ON" : "OFF";

  html += R"rawliteral(</p>
  <p><strong>Status:</strong> )rawliteral";

  html += EscapeHtml(pumpStatusString);

  html += R"rawliteral(</p>
  <form method='POST' action='/pump/test'>
    <button type='submit'>Test Pump</button>
  </form>
  <form method='POST' action='/pump/off'>
    <button type='submit'>Stop Pump</button>
  </form>
</div>

<div class='card'>
  <h2>Configuration</h2>
  <p><strong>Watering:</strong> starts below )rawliteral";

  html += String(startWateringPercentage);

  html += R"rawliteral(%, Good above )rawliteral";

  html += String(stopWateringPercentage);

  html += R"rawliteral(%</p>
  <p><strong>Water warning:</strong> below )rawliteral";

  html += String(lowWaterLevelPercentage);

  html += R"rawliteral(%, Good above )rawliteral";

  html += String(goodWaterLevelPercentage);

  html += R"rawliteral(%</p>
  <p><strong>Dry Calibration:</strong> )rawliteral";

  html += String(dryCalibration);

  html += R"rawliteral(</p>
  <p><strong>Wet Calibration:</strong> )rawliteral";

  html += String(wetCalibration);

  html += R"rawliteral(</p>
  <p><strong>Empty Water Calibration:</strong> )rawliteral";

  html += String(emptyWaterCalibration);

  html += R"rawliteral(</p>
  <p><strong>Full Water Calibration:</strong> )rawliteral";

  html += String(fullWaterCalibration);

  html += R"rawliteral(</p>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- History Graph Page -------------------- //

String BuildHistoryPageHtml() {
  String html = BuildPageStart("24 Hour History");
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(
<div class='card'>
  <h1>24 Hour History</h1>
  <p>soil moisture, temperature and humidity readings.</p>
  <p>Data will be lost when device is powered off.</p>
  <p><a href='/'>Back to Dashboard</a></p>

</div>

<div class='card'>
  <h2>History Info</h2>
  <p><strong>History Count:</strong> <span id='storedPoints'>0</span></p>
  <p><strong>History Interval:</strong> <span id='sampleInterval'>0</span> minutes</p>
</div>

<div class='card'>
  <h2>Soil Moisture (%)</h2>
  <svg id='moistureGraph' viewBox='0 0 700 220' preserveAspectRatio='xMidYMid meet'></svg>
</div>

<div class='card'>
  <h2>Temperature (&deg;C)</h2>
  <svg id='temperatureGraph' viewBox='0 0 700 220' preserveAspectRatio='xMidYMid meet'></svg>
</div>

<div class='card'>
  <h2>Humidity (%)</h2>
  <svg id='humidityGraph' viewBox='0 0 700 220' preserveAspectRatio='xMidYMid meet'></svg>
</div>

<script>
  function drawGraph(svgId, points, key, minValue, maxValue) {
    const svg = document.getElementById(svgId);

    const width = 700;
    const height = 220;

    const paddingLeft = 55;
    const paddingRight = 35;
    const paddingTop = 15;
    const paddingBottom = 30;

    const graphWidth = width - paddingLeft - paddingRight;
    const graphHeight = height - paddingTop - paddingBottom;

    svg.innerHTML = "";

    const values = points
      .map(point => point[key])
      .filter(value => value !== null && !isNaN(value));

    if (values.length < 2) {
      svg.innerHTML =
        "<text x='20' y='40' fill='#003e04'>Not enough data</text>";
      return;
    }

    let min = minValue;
    let max = maxValue;

    if (min === null || max === null) {
      min = Math.min(...values);
      max = Math.max(...values);

      if (min === max) {
        min = min - 1;
        max = max + 1;
      }
    }

    function getX(index) {
      return paddingLeft + (index / (points.length - 1)) * graphWidth;
    }

    function getY(value) {
      return paddingTop + ((max - value) / (max - min)) * graphHeight;
    }


    // background
    svg.innerHTML +=
      "<rect x='0' y='0' width='" + width + "' height='" + height + "' fill='white' />";


    // axis
    svg.innerHTML +=
      "<line x1='" + paddingLeft + "' y1='" + paddingTop +
      "' x2='" + paddingLeft + "' y2='" + (paddingTop + graphHeight) +
      "' stroke='#2d6a4f' stroke-width='1' />";

    svg.innerHTML +=
      "<line x1='" + paddingLeft + "' y1='" + (paddingTop + graphHeight) +
      "' x2='" + (paddingLeft + graphWidth) + "' y2='" + (paddingTop + graphHeight) +
      "' stroke='#2d6a4f' stroke-width='1' />";

    // axis labels
    svg.innerHTML +=
      "<text x='5' y='" + (paddingTop + 5) +
      "' fill='#2d6a4f' font-size='12'>" + max + "</text>";

    svg.innerHTML +=
      "<text x='5' y='" + (paddingTop + graphHeight) +
      "' fill='#2d6a4f' font-size='12'>" + min + "</text>";

    svg.innerHTML +=
      "<text x='" + paddingLeft + "' y='" + (height - 8) +
      "' fill='#2d6a4f' font-size='12'>oldest</text>";

    svg.innerHTML +=
      "<text x='" + (width - 45) + "' y='" + (height - 8) +
      "' fill='#2d6a4f' font-size='12'>now</text>";

    let pointString = "";

    for (let i = 0; i < points.length; i++) {
      const value = points[i][key];

      if (value === null || isNaN(value)) {
        continue;
      }

      const x = getX(i);
      const y = getY(value);

      pointString += x + "," + y + " ";
    }

    svg.innerHTML +=
      "<polyline points='" + pointString +
      "' fill='none' stroke='#2d6a4f' stroke-width='2' />";
  }

  async function loadHistory() {
    const response = await fetch('/api/history');
    const data = await response.json();

    document.getElementById('storedPoints').textContent = data.stored_points;
    document.getElementById('sampleInterval').textContent = data.sample_interval_minutes;

    drawGraph('moistureGraph', data.points, 'soil_moisture', 0, 100);
    drawGraph('temperatureGraph', data.points, 'temperature', null, null);
    drawGraph('humidityGraph', data.points, 'humidity', 0, 100);
  }

  loadHistory();
  setInterval(loadHistory, 30000);
</script>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

#endif
