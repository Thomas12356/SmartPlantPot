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

String PercentText(int value) {
  if (value < 0) {
    return "Calibration error";
  }

  return String(value) + "%";
}

String TemperatureText() {
  if (!latestState.am2320Ok) {
    return "Sensor warning";
  }

  return String(latestState.temperature, 1) + " &deg;C";
}

String HumidityText() {
  if (!latestState.am2320Ok) {
    return "Sensor warning";
  }

  return String(latestState.humidity, 1) + "%";
}

String SystemToneClass() {
  if (latestState.sensorError || pumpFault) {
    return "danger";
  }

  if (latestState.waterIsLow || latestState.soilIsDry || !latestState.am2320Ok) {
    return "warning";
  }

  return "ok";
}

String SoilToneClass() {
  if (latestState.soilMoisture < 0) {
    return "danger";
  }

  return latestState.soilIsDry ? "warning" : "ok";
}

String WaterToneClass() {
  if (latestState.waterLevel < 0) {
    return "danger";
  }

  return latestState.waterIsLow ? "danger" : "ok";
}

String TempToneClass() {
  return latestState.am2320Ok ? "ok" : "warning";
}

String PumpToneClass() {
  if (pumpFault) {
    return "danger";
  }

  return isPumpOn ? "warning" : "ok";
}

String PumpText() {
  if (pumpFault) {
    return "FAULT";
  }

  return isPumpOn ? "ON" : "OFF";
}

void AppendPageHeader(String& html, const String& title, const String& subtitle) {
  html += "<header class='page-header'><div><h1>";
  html += EscapeHtml(title);
  html += "</h1>";

  if (subtitle.length() > 0) {
    html += "<p>";
    html += EscapeHtml(subtitle);
    html += "</p>";
  }

  html += R"rawliteral(</div>
  <nav class='nav'>
    <a href='/'>Dashboard</a>
    <a href='/history'>Sensor History</a>
    <a href='/wifi'>WiFi</a>
    <a href='/calibration'>Calibration</a>
  </nav>
</header>
)rawliteral";
}

String BuildPageStart(String title, int refreshSeconds = 0, String refreshUrl = "") {
  String html;
  html.reserve(HTML_RESERVE_SIZE);

  html += R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
)rawliteral";

  if (refreshSeconds > 0) {
    html += "<meta http-equiv='refresh' content='";
    html += String(refreshSeconds);

    if (refreshUrl.length() > 0) {
      html += ";url=";
      html += EscapeHtml(refreshUrl);
    }

    html += "'>";
  }

  html += "<title>";
  html += EscapeHtml(title);
  html += "</title>";

  html += R"rawliteral(
  <style>
    :root {
      --bg: #f4f7f2;
      --card: #ffffff;
      --border: #d7e3d4;
      --text: #1f2a1f;
      --muted: #61705f;
      --green: #0b6b2a;
      --green-dark: #06491d;
      --green-soft: #e8f5ea;
      --amber: #b36b00;
      --amber-soft: #fff5df;
      --red: #b42318;
      --red-soft: #fff1ef;
      --space-1: 4px;
      --space-2: 8px;
      --space-3: 12px;
      --space-4: 16px;
      --space-5: 20px;
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      background: var(--bg);
      color: var(--text);
      font-family: Arial, sans-serif;
      line-height: 1.4;
    }

    .page {
      width: min(980px, 100%);
      margin: 0 auto;
      padding: var(--space-4);
    }

    .page-header {
      display: flex;
      justify-content: space-between;
      gap: var(--space-4);
      align-items: flex-start;
      margin-bottom: var(--space-4);
      padding: var(--space-3) 0;
    }

    h1, h2, h3, p {
      margin-top: 0;
    }

    h1 {
      color: var(--green-dark);
      margin-bottom: var(--space-1);
      font-size: 1.8rem;
      line-height: 1.15;
    }

    h2 {
      color: var(--green-dark);
      font-size: 1.1rem;
      margin-bottom: var(--space-3);
      line-height: 1.2;
    }

    h3 {
      font-size: 1rem;
      margin-bottom: var(--space-2);
      line-height: 1.2;
    }

    p {
      margin-bottom: var(--space-3);
    }

    .page-header p,
    .muted {
      color: var(--muted);
    }

    .page-header p {
      margin-bottom: 0;
    }

    .card p {
      margin-bottom: var(--space-2);
    }

    .card .pill + h2 {
      margin-top: 0;
    }

    .value + .muted {
      margin-top: var(--space-2);
    }

    .nav, .actions {
      display: flex;
      flex-wrap: wrap;
      gap: var(--space-2);
      align-items: center;
    }

    .actions {
      margin-top: var(--space-4);
    }

    .actions form {
      margin: 0;
    }

    a, button {
      display: inline-block;
      background: var(--green);
      color: white;
      padding: 10px 12px;
      border: 1px solid var(--green);
      border-radius: 6px;
      text-decoration: none;
      font: inherit;
      cursor: pointer;
    }

    a.secondary, button.secondary {
      background: white;
      color: var(--green-dark);
      border-color: var(--border);
    }

    button.danger {
      background: var(--red);
      border-color: var(--red);
    }

    a:focus, button:focus, input:focus {
      outline: 3px solid rgba(11, 107, 42, 0.25);
      outline-offset: 2px;
    }

    .card {
      background: var(--card);
      border: 1px solid var(--border);
      padding: var(--space-4);
      margin-bottom: var(--space-4);
      border-radius: 8px;
    }

    .card > :last-child,
    .metric > :last-child {
      margin-bottom: 0;
    }

    .hero {
      border-left: 6px solid var(--green);
    }

    .hero.warning {
      border-left-color: var(--amber);
      background: var(--amber-soft);
    }

    .hero.danger {
      border-left-color: var(--red);
      background: var(--red-soft);
    }

    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: var(--space-4);
      margin-bottom: var(--space-4);
    }

    .metric-grid {
      grid-template-columns: repeat(4, minmax(0, 1fr));
    }

    .grid > .card,
    .grid > .metric {
      margin-bottom: 0;
    }

    .metric {
      background: #fbfdfb;
      border: 1px solid var(--border);
      border-left: 6px solid var(--green);
      border-radius: 8px;
      padding: var(--space-4);
      min-width: 0;
    }

    .metric.warning {
      border-left-color: var(--amber);
      background: var(--amber-soft);
    }

    .metric.danger {
      border-left-color: var(--red);
      background: var(--red-soft);
    }

    .label {
      display: block;
      color: var(--muted);
      font-size: 0.85rem;
      margin-bottom: var(--space-2);
    }

    .value {
      display: block;
      font-size: 1.55rem;
      font-weight: 700;
      overflow-wrap: anywhere;
    }

    .hint {
      display: block;
      color: var(--muted);
      font-size: 0.82rem;
      margin-top: var(--space-2);
    }

    .pill {
      display: inline-flex;
      align-items: center;
      padding: 4px 8px;
      border-radius: 999px;
      font-size: 0.82rem;
      font-weight: 700;
      color: var(--green-dark);
      background: var(--green-soft);
      border: 1px solid #b7d7bf;
      margin-bottom: var(--space-3);
    }

    .pill.warning {
      color: #6b3d00;
      background: var(--amber-soft);
      border-color: #e5c476;
    }

    .pill.danger {
      color: #7a120d;
      background: var(--red-soft);
      border-color: #efb3ad;
    }

    input {
      width: 100%;
      padding: 10px;
      border: 1px solid var(--border);
      border-radius: 6px;
      font: inherit;
    }

    label {
      display: block;
      font-weight: 700;
      margin-bottom: var(--space-2);
    }

    .form-row {
      margin-bottom: var(--space-4);
    }

    .config-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
      gap: 0;
      margin: var(--space-3) 0;
      border-top: 1px solid var(--border);
      border-left: 1px solid var(--border);
    }

    .config-item {
      padding: var(--space-3);
      background: transparent;
      border-right: 1px solid var(--border);
      border-bottom: 1px solid var(--border);
    }

    svg {
      width: 100%;
      max-width: 760px;
      height: auto;
      display: block;
      border: 1px solid var(--border);
      border-radius: 8px;
      background: white;
    }

    .notice {
      padding: 10px;
      border-radius: 8px;
      margin-bottom: var(--space-4);
      border: 1px solid var(--border);
      background: #fbfdfb;
    }

    .notice.danger {
      border-color: #efb3ad;
      background: var(--red-soft);
      color: #7a120d;
    }

    [hidden] {
      display: none;
    }

    @media (max-width: 640px) {
      .page {
        padding: var(--space-3);
      }

      .page-header {
        display: block;
      }

      .nav {
        margin-top: var(--space-3);
      }

      .metric-grid {
        grid-template-columns: repeat(2, minmax(0, 1fr));
        gap: var(--space-3);
      }

      .metric-grid .metric {
        padding: var(--space-3);
      }

      .metric-grid .value {
        font-size: 1.25rem;
      }

      .metric-grid .label,
      .metric-grid .hint {
        font-size: 0.78rem;
      }

      a, button {
        width: 100%;
        text-align: center;
      }

      .actions form,
      .actions button {
        width: 100%;
      }
    }
  </style>
</head>
<body>
<main class='page'>
)rawliteral";

  return html;
}

String BuildPageEnd() {
  return R"rawliteral(
</main>
</body>
</html>
)rawliteral";
}

// -------------------- WiFi Setup Page -------------------- //

String BuildWifiPageHtml() {
  String html = BuildPageStart("WiFi Setup");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "WiFi Setup", "Connect the pot to local WiFi while keeping the setup network available.");

  html += R"rawliteral(
<div class='grid'>
  <div class='card'>
    <h2>Setup Network</h2>
    <span class='pill ok'>Always available</span>
    <p><strong>SSID:</strong> )rawliteral";

  html += EscapeHtml(String(SSID));

  html += R"rawliteral(</p>
    <p><strong>IP:</strong> )rawliteral";

  html += WiFi.softAPIP().toString();

  html += R"rawliteral(</p>
  </div>

  <div class='card'>
    <h2>Local WiFi</h2>
)rawliteral";

  if (WiFi.status() == WL_CONNECTED) {
    html += R"rawliteral(    <span class='pill ok'>Connected</span>
    <p><strong>SSID:</strong> )rawliteral";

    html += EscapeHtml(WiFi.SSID());

    html += R"rawliteral(</p>
    <p><strong>IP:</strong> )rawliteral";

    html += WiFi.localIP().toString();

    html += "</p>";
  } else if (localWiFiConnecting) {
    html += R"rawliteral(    <span class='pill warning'>Connecting</span>
)rawliteral";

    if (savedSSID.length() > 0) {
      html += R"rawliteral(    <p><strong>Trying SSID:</strong> )rawliteral";
      html += EscapeHtml(savedSSID);
      html += "</p>";
    }
  } else {
    html += R"rawliteral(    <span class='pill warning'>Not connected</span>
)rawliteral";

    if (savedSSID.length() > 0) {
      html += R"rawliteral(    <p><strong>Saved SSID:</strong> )rawliteral";
      html += EscapeHtml(savedSSID);
      html += "</p>";
    } else {
      html += "    <p class='muted'>No saved local network.</p>";
    }
  }

  html += R"rawliteral(
  </div>
</div>

<div class='card'>
  <h2>Connect to Local WiFi</h2>
  <form method='POST' action='/wifi/save'>
    <div class='form-row'>
      <label for='ssid'>SSID</label>
      <input id='ssid' type='text' name='ssid' required value=')rawliteral";

  html += "'";
  html += EscapeHtml(savedSSID);

  html += R"rawliteral('>
    </div>

    <div class='form-row'>
      <label for='password'>Password</label>
      <input id='password' type='password' name='password'>
    </div>

    <button type='submit'>Save and Connect</button>
  </form>
</div>

<div class='card'>
  <h2>Forget Local WiFi</h2>
  <p class='muted'>This removes the saved home network. The setup network will stay available.</p>
  <form method='POST' action='/wifi/forget' onsubmit="return confirm('Forget saved WiFi details?');">
    <button class='danger' type='submit'>Forget Local WiFi</button>
  </form>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- WiFi Connecting Page -------------------- //

String BuildWifiConnectingHtml() {
  String html = BuildPageStart("WiFi Saved", 5, "/wifi");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "WiFi Details Saved", "The ESP32 is trying to connect in the background.");

  html += R"rawliteral(
<div class='card hero warning'>
  <span class='pill warning'>Connecting</span>
  <h2>Checking the new network</h2>
  <p>This page will return to WiFi setup in 5 seconds.</p>
  <div class='actions'>
    <a href='/wifi'>View WiFi Status</a>
    <a class='secondary' href='/'>Dashboard</a>
  </div>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- WiFi Result Page -------------------- //

String BuildWifiResultHtml(bool connected) {
  String html = BuildPageStart("WiFi Result");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "WiFi Result", "");

  html += "<div class='card hero ";
  html += connected ? "ok" : "danger";
  html += "'>";

  if (connected) {
    html += R"rawliteral(
  <span class='pill ok'>Connected</span>
  <h2>Local WiFi is connected</h2>
  <p><strong>SSID:</strong> )rawliteral";

    html += EscapeHtml(WiFi.SSID());

    html += R"rawliteral(</p>
  <p><strong>IP:</strong> )rawliteral";

    html += WiFi.localIP().toString();
    html += "</p>";
  } else {
    html += R"rawliteral(
  <span class='pill danger'>Failed</span>
  <h2>Could not connect to local WiFi</h2>
  <p>Check the SSID and password, then try again.</p>
)rawliteral";
  }

  html += R"rawliteral(
  <div class='actions'>
    <a href='/wifi'>Back to WiFi Setup</a>
    <a class='secondary' href='/'>Dashboard</a>
  </div>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- WiFi Forgotten Page -------------------- //

String BuildWifiForgottenHtml() {
  String html = BuildPageStart("WiFi Forgotten");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "WiFi Forgotten", "");

  html += R"rawliteral(
<div class='card hero warning'>
  <span class='pill warning'>Removed</span>
  <h2>Saved WiFi details have been removed</h2>
  <p>The setup network is still available for configuration.</p>
  <div class='actions'>
    <a href='/wifi'>Back to WiFi Setup</a>
    <a class='secondary' href='/'>Dashboard</a>
  </div>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- Calibration Pages -------------------- //

String BuildCalibrationSavedHtml(String message) {
  String html = BuildPageStart("Calibration Saved");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "Calibration Saved", "");

  html += R"rawliteral(
<div class='card hero ok'>
  <span class='pill ok'>Saved</span>
  <h2>)rawliteral";

  html += EscapeHtml(message);

  html += R"rawliteral(</h2>
  <div class='actions'>
    <a href='/calibration'>Back to Calibration</a>
    <a class='secondary' href='/'>Dashboard</a>
  </div>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

String BuildCalibrationPageHtml() {
  int currentSoilRaw = AnalogReadAverage(SOIL_MOISTURE_PIN);
  int currentWaterRaw = AnalogReadAverage(WATER_LEVEL_PIN);
  bool soilCalibrationOk = dryCalibration - wetCalibration >= MIN_CALIBRATION_SPAN;
  bool waterCalibrationOk = fullWaterCalibration - emptyWaterCalibration >= MIN_CALIBRATION_SPAN;

  String html = BuildPageStart("Calibration");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "Calibration", "Save the current raw readings as known dry, wet, empty, or full values.");

  html += R"rawliteral(
<div class='grid'>
  <div class='metric'>
    <span class='label'>Current Soil Raw</span>
    <span class='value'>)rawliteral";

  html += String(currentSoilRaw);

  html += R"rawliteral(</span>
  </div>

  <div class='metric'>
    <span class='label'>Current Water Raw</span>
    <span class='value'>)rawliteral";

  html += String(currentWaterRaw);

  html += R"rawliteral(</span>
  </div>
</div>

<div class='grid'>
  <div class='card'>
    <h2>Soil Calibration</h2>
)rawliteral";

  html += soilCalibrationOk ? "<span class='pill ok'>Valid</span>" : "<span class='pill danger'>Needs calibration</span>";

  html += R"rawliteral(
    <div class='config-grid'>
      <div class='config-item'>
        <span class='label'>Dry value</span>
        <strong>)rawliteral";

  html += String(dryCalibration);

  html += R"rawliteral(</strong>
      </div>
      <div class='config-item'>
        <span class='label'>Wet value</span>
        <strong>)rawliteral";

  html += String(wetCalibration);

  html += R"rawliteral(</strong>
      </div>
    </div>
    <p class='muted'>Dry should read at least )rawliteral";

  html += String(MIN_CALIBRATION_SPAN);

  html += R"rawliteral( raw counts higher than wet.</p>
    <div class='actions'>
      <form method='POST' action='/calibration/soil-dry'>
        <button type='submit'>Save Soil as Dry</button>
      </form>
      <form method='POST' action='/calibration/soil-wet'>
        <button class='secondary' type='submit'>Save Soil as Wet</button>
      </form>
    </div>
  </div>

  <div class='card'>
    <h2>Water Level Calibration</h2>
)rawliteral";

  html += waterCalibrationOk ? "<span class='pill ok'>Valid</span>" : "<span class='pill danger'>Needs calibration</span>";

  html += R"rawliteral(
    <div class='config-grid'>
      <div class='config-item'>
        <span class='label'>Empty value</span>
        <strong>)rawliteral";

  html += String(emptyWaterCalibration);

  html += R"rawliteral(</strong>
      </div>
      <div class='config-item'>
        <span class='label'>Full value</span>
        <strong>)rawliteral";

  html += String(fullWaterCalibration);

  html += R"rawliteral(</strong>
      </div>
    </div>
    <p class='muted'>Full should read at least )rawliteral";

  html += String(MIN_CALIBRATION_SPAN);

  html += R"rawliteral( raw counts higher than empty.</p>
    <div class='actions'>
      <form method='POST' action='/calibration/water-empty'>
        <button type='submit'>Save Water as Empty</button>
      </form>
      <form method='POST' action='/calibration/water-full'>
        <button class='secondary' type='submit'>Save Water as Full</button>
      </form>
    </div>
  </div>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- Pump Result Page -------------------- //

String BuildPumpResultHtml(String message) {
  String html = BuildPageStart("Pump Control");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "Pump Control", "");

  html += "<div class='card hero ";
  html += PumpToneClass();
  html += "'><span class='pill ";
  html += PumpToneClass();
  html += "'>";
  html += EscapeHtml(PumpText());
  html += "</span><h2>";
  html += EscapeHtml(message);
  html += R"rawliteral(</h2>
  <div class='actions'>
    <a href='/'>Back to Dashboard</a>
  </div>
</div>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- Dashboard Page -------------------- //

String BuildRootPageHtml() {
  String html = BuildPageStart("Smart Plant Pot");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "Smart Plant Pot", "Live readings and watering controls.");

  html += "<div id='statusHero' class='card hero ";
  html += SystemToneClass();
  html += "'><span id='statusPill' class='pill ";
  html += SystemToneClass();
  html += "'>";
  html += EscapeHtml(SystemToneClass() == "ok" ? "OK" : (SystemToneClass() == "warning" ? "Warning" : "Needs attention"));
  html += "</span><h2 id='statusText'>";
  html += EscapeHtml(GetSystemStatusText());
  html += R"rawliteral(</h2>
  <p class='muted'>Last reading: <span id='lastReading'>)rawliteral";

  html += String(latestState.TimeStampMs);

  html += R"rawliteral( ms after startup</span></p>
</div>

<div class='grid metric-grid'>
  <div id='soilCard' class='metric )rawliteral";

  html += SoilToneClass();

  html += R"rawliteral('>
    <span class='label'>Soil Moisture</span>
    <span id='soilMoisture' class='value'>)rawliteral";

  html += EscapeHtml(PercentText(latestState.soilMoisture));

  html += R"rawliteral(</span>
    <span class='hint'>Watering starts below )rawliteral";

  html += String(startWateringPercentage);

  html += R"rawliteral(%</span>
  </div>

  <div id='waterCard' class='metric )rawliteral";

  html += WaterToneClass();

  html += R"rawliteral('>
    <span class='label'>Water Level</span>
    <span id='waterLevel' class='value'>)rawliteral";

  html += EscapeHtml(PercentText(latestState.waterLevel));

  html += R"rawliteral(</span>
    <span class='hint'>Warning below )rawliteral";

  html += String(lowWaterLevelPercentage);

  html += R"rawliteral(%</span>
  </div>

  <div id='tempCard' class='metric )rawliteral";

  html += TempToneClass();

  html += R"rawliteral('>
    <span class='label'>Temperature</span>
    <span id='temperatureValue' class='value'>)rawliteral";

  html += TemperatureText();

  html += R"rawliteral(</span>
    <span class='hint'>AM2320</span>
  </div>

  <div id='humidityCard' class='metric )rawliteral";

  html += TempToneClass();

  html += R"rawliteral('>
    <span class='label'>Humidity</span>
    <span id='humidityValue' class='value'>)rawliteral";

  html += HumidityText();

  html += R"rawliteral(</span>
    <span class='hint'>AM2320</span>
  </div>
</div>

<div class='grid'>
  <div id='pumpCard' class='card hero )rawliteral";

  html += PumpToneClass();

  html += R"rawliteral('>
    <h2>Pump</h2>
    <span id='pumpValue' class='value'>)rawliteral";

  html += EscapeHtml(PumpText());

  html += R"rawliteral(</span>
    <p id='pumpStatus' class='muted'>)rawliteral";

  html += EscapeHtml(pumpStatusString);

  html += R"rawliteral(</p>
    <div class='actions'>
      <form method='POST' action='/pump/test'>
        <button type='submit'>Test Pump Pulse</button>
      </form>
      <form method='POST' action='/pump/off'>
        <button class='danger' type='submit'>Stop / Clear Fault</button>
      </form>
    </div>
  </div>

  <div class='card'>
    <h2>Network</h2>
    <div class='config-grid'>
      <div class='config-item'>
        <span class='label'>Setup IP</span>
        <strong id='setupIp'>)rawliteral";

  html += WiFi.softAPIP().toString();

  html += R"rawliteral(</strong>
      </div>
      <div class='config-item'>
        <span class='label'>Local WiFi</span>
        <strong id='homeWifi'>)rawliteral";

  html += WiFi.status() == WL_CONNECTED ? "Connected" : "Not connected";

  html += R"rawliteral(</strong>
      </div>
      <div class='config-item'>
        <span class='label'>Local SSID</span>
        <strong id='homeSsid'>)rawliteral";

  if (WiFi.status() == WL_CONNECTED) {
    html += EscapeHtml(WiFi.SSID());
  } else {
    html += "-";
  }

  html += R"rawliteral(</strong>
      </div>
      <div class='config-item'>
        <span class='label'>Local IP</span>
        <strong id='homeIp'>)rawliteral";

  if (WiFi.status() == WL_CONNECTED) {
    html += WiFi.localIP().toString();
  } else {
    html += "-";
  }

  html += R"rawliteral(</strong>
      </div>
    </div>
  </div>
</div>

<div class='card'>
  <h2>Configuration</h2>
  <div class='config-grid'>
    <div class='config-item'>
      <span class='label'>Watering starts</span>
      <strong>)rawliteral";

  html += String(startWateringPercentage);

  html += R"rawliteral(%</strong>
    </div>
    <div class='config-item'>
      <span class='label'>Watering stops</span>
      <strong>)rawliteral";

  html += String(stopWateringPercentage);

  html += R"rawliteral(%</strong>
    </div>
    <div class='config-item'>
      <span class='label'>Low water warning</span>
      <strong>)rawliteral";

  html += String(lowWaterLevelPercentage);

  html += R"rawliteral(%</strong>
    </div>
    <div class='config-item'>
      <span class='label'>Water level OK</span>
      <strong>)rawliteral";

  html += String(goodWaterLevelPercentage);

  html += R"rawliteral(%</strong>
    </div>
  </div>
</div>

<script>
  function setText(id, value) {
    const element = document.getElementById(id);
    if (element) {
      element.textContent = value;
    }
  }

  function setClass(id, baseClass, tone) {
    const element = document.getElementById(id);
    if (element) {
      element.className = baseClass + ' ' + tone;
    }
  }

  function statusTone(data) {
    if (data.sensor_error || data.pump_fault) {
      return 'danger';
    }

    if (data.water_low || data.soil_is_dry || !data.am2320_ok) {
      return 'warning';
    }

    return 'ok';
  }

  function toneLabel(tone) {
    if (tone === 'danger') {
      return 'Needs attention';
    }

    if (tone === 'warning') {
      return 'Warning';
    }

    return 'OK';
  }

  function percentText(value) {
    return value >= 0 ? value + '%' : 'Calibration error';
  }

  async function refreshDashboard() {
    try {
      const response = await fetch('/api/status');
      if (!response.ok) {
        throw new Error('Status request failed');
      }

      const data = await response.json();
      const tone = statusTone(data);

      setClass('statusHero', 'card hero', tone);
      setClass('statusPill', 'pill', tone);
      setText('statusPill', toneLabel(tone));
      setText('statusText', data.status);
      setText('lastReading', data.last_reading_ms + ' ms after startup');

      setClass('soilCard', 'metric', data.soil_moisture < 0 ? 'danger' : (data.soil_is_dry ? 'warning' : 'ok'));
      setText('soilMoisture', percentText(data.soil_moisture));

      setClass('waterCard', 'metric', data.water_level < 0 || data.water_low ? 'danger' : 'ok');
      setText('waterLevel', percentText(data.water_level));

      setClass('tempCard', 'metric', data.am2320_ok ? 'ok' : 'warning');
      setClass('humidityCard', 'metric', data.am2320_ok ? 'ok' : 'warning');
      setText('temperatureValue', data.temperature === null ? 'Sensor warning' : data.temperature + ' C');
      setText('humidityValue', data.humidity === null ? 'Sensor warning' : data.humidity + '%');

      setClass('pumpCard', 'card hero', data.pump_fault ? 'danger' : (data.pump_running ? 'warning' : 'ok'));
      setText('pumpValue', data.pump_fault ? 'FAULT' : (data.pump_running ? 'ON' : 'OFF'));
      setText('pumpStatus', data.pump_status);

      setText('homeWifi', data.home_wifi_connected ? 'Connected' : (data.home_wifi_connecting ? 'Connecting' : 'Not connected'));
      setText('homeSsid', data.home_wifi_ssid || '-');
      setText('homeIp', data.home_wifi_ip || '-');
    } catch (error) {
      setClass('statusHero', 'card hero', 'danger');
      setClass('statusPill', 'pill', 'danger');
      setText('statusPill', 'Offline');
      setText('statusText', 'Dashboard update failed');
    }
  }

  setInterval(refreshDashboard, 5000);
</script>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

// -------------------- History Graph Page -------------------- //

String BuildHistoryPageHtml() {
  String html = BuildPageStart("24 Hour History");
  html.reserve(HTML_RESERVE_SIZE);

  AppendPageHeader(html, "24 Hour History", "Soil moisture, temperature, and humidity trends.");

  html += R"rawliteral(
<div id='historyError' class='notice danger' hidden>Unable to load history data.</div>

<div class='grid'>
  <div class='metric'>
    <span class='label'>Stored Points</span>
    <span id='storedPoints' class='value'>0</span>
  </div>
  <div class='metric'>
    <span class='label'>Sample Interval</span>
    <span class='value'><span id='sampleInterval'>0</span> min</span>
  </div>
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
  function addPolyline(svg, pointString) {
    const points = pointString.trim();

    if (points.split(' ').filter(Boolean).length < 2) {
      return;
    }

    svg.innerHTML +=
      "<polyline points='" + points +
      "' fill='none' stroke='#0b6b2a' stroke-width='2' />";
  }

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
        "<text x='20' y='40' fill='#61705f'>Not enough data yet</text>";
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

    svg.innerHTML +=
      "<rect x='0' y='0' width='" + width + "' height='" + height + "' fill='white' />";

    svg.innerHTML +=
      "<line x1='" + paddingLeft + "' y1='" + paddingTop +
      "' x2='" + paddingLeft + "' y2='" + (paddingTop + graphHeight) +
      "' stroke='#61705f' stroke-width='1' />";

    svg.innerHTML +=
      "<line x1='" + paddingLeft + "' y1='" + (paddingTop + graphHeight) +
      "' x2='" + (paddingLeft + graphWidth) + "' y2='" + (paddingTop + graphHeight) +
      "' stroke='#61705f' stroke-width='1' />";

    svg.innerHTML +=
      "<text x='5' y='" + (paddingTop + 5) +
      "' fill='#61705f' font-size='12'>" + max + "</text>";

    svg.innerHTML +=
      "<text x='5' y='" + (paddingTop + graphHeight) +
      "' fill='#61705f' font-size='12'>" + min + "</text>";

    svg.innerHTML +=
      "<text x='" + paddingLeft + "' y='" + (height - 8) +
      "' fill='#61705f' font-size='12'>oldest</text>";

    svg.innerHTML +=
      "<text x='" + (width - 45) + "' y='" + (height - 8) +
      "' fill='#61705f' font-size='12'>now</text>";

    let pointString = "";

    for (let i = 0; i < points.length; i++) {
      const value = points[i][key];

      if (value === null || isNaN(value)) {
        addPolyline(svg, pointString);
        pointString = "";
        continue;
      }

      const x = getX(i);
      const y = getY(value);

      pointString += x + "," + y + " ";
    }

    addPolyline(svg, pointString);
  }

  async function loadHistory() {
    const error = document.getElementById('historyError');

    try {
      const response = await fetch('/api/history');
      if (!response.ok) {
        throw new Error('History request failed');
      }

      const data = await response.json();
      error.hidden = true;

      document.getElementById('storedPoints').textContent = data.stored_points;
      document.getElementById('sampleInterval').textContent = data.sample_interval_minutes;

      drawGraph('moistureGraph', data.points, 'soil_moisture', 0, 100);
      drawGraph('temperatureGraph', data.points, 'temperature', null, null);
      drawGraph('humidityGraph', data.points, 'humidity', 0, 100);
    } catch (requestError) {
      error.hidden = false;
    }
  }

  loadHistory();
  setInterval(loadHistory, 30000);
</script>
)rawliteral";

  html += BuildPageEnd();

  return html;
}

#endif
