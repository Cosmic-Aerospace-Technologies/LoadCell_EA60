#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HX711.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include "dashboard.h"

static constexpr uint8_t HX_DOUT = 19;
static constexpr uint8_t HX_SCK = 18;
static constexpr uint8_t TEMP_PIN = 4;

static const char* AP_NAME = "ThrustBench-ESP32";
static const char* AP_PASSWORD = "ThrustTest123";

static constexpr uint32_t SENSOR_TIMEOUT_MS = 1500;
static constexpr uint32_t TEMP_TIMEOUT_MS = 3000;
static constexpr uint32_t LOGGER_TIMEOUT_MS = 4500;
static constexpr uint32_t TEMP_SAMPLE_PERIOD_MS = 1000;
static constexpr uint32_t TEMP_CONVERSION_MS = 400; // 11-bit DS18B20 ~= 375 ms

static constexpr size_t RAW_WINDOW = 16;
static constexpr size_t HISTORY_SIZE = 2048;
static constexpr size_t TEMP_HISTORY_SIZE = 1800; // ~30 min at 1 Hz
static constexpr size_t API_BATCH_SIZE = 96;
static constexpr size_t TEMP_API_BATCH_SIZE = 120;

HX711 loadCell;
OneWire oneWire(TEMP_PIN);
DallasTemperature temperatureBus(&oneWire);
WebServer server(80);
Preferences preferences;

struct Reading {
  uint32_t id;
  uint32_t timeMs;
  int32_t raw;
  float gf;
  float tempC;
};

struct TempReading {
  uint32_t id;
  uint32_t timeMs;
  float c;
};

Reading history[HISTORY_SIZE];
TempReading tempHistory[TEMP_HISTORY_SIZE];

size_t historyHead = 0;
size_t historyCount = 0;
size_t tempHead = 0;
size_t tempCount = 0;

uint32_t sequence = 0;
uint32_t tempSequence = 0;
uint32_t sensorEpoch = 1;
uint32_t bootId = 0;

int32_t rawWindow[RAW_WINDOW] = {};
size_t rawCount = 0;
size_t rawHead = 0;

bool seenReading = false;
bool seenTemperature = false;
bool tared = false;
bool recording = false;
bool loggerEverPinged = false;
bool tempConversionPending = false;

int32_t latestRaw = 0;
int32_t tareRaw = 0;
float countsPerGF = 0.0f;
float latestGF = 0.0f;
float peakGF = 0.0f;
float samplesPerSecond = 0.0f;

float latestTempC = NAN;
float minTempC = NAN;
float maxTempC = NAN;
float warningTempC = 60.0f;
float dangerTempC = 70.0f;

uint32_t latestReadMs = 0;
uint32_t latestTempMs = 0;
uint32_t lastTempRequestMs = 0;
uint32_t rateStartMs = 0;
uint32_t rateCount = 0;
uint32_t lastLoggerPing = 0;

uint32_t runId = 0;
uint32_t runStartMs = 0;
uint32_t runStopMs = 0;
uint32_t runStartSeq = 0;
uint32_t runEndSeq = 0;
uint32_t recordedCount = 0;

String message = "Ready. Start PC logger, then tare and calibrate.";

bool calibrationValid() {
  return isfinite(countsPerGF) && fabsf(countsPerGF) > 0.00001f;
}

bool sensorOnline() {
  return seenReading &&
         (uint32_t)(millis() - latestReadMs) < SENSOR_TIMEOUT_MS;
}

bool temperatureOnline() {
  return seenTemperature &&
         (uint32_t)(millis() - latestTempMs) < TEMP_TIMEOUT_MS &&
         isfinite(latestTempC);
}

bool measurementReady() {
  return sensorOnline() && tared && calibrationValid();
}

bool loggerOnline() {
  return loggerEverPinged &&
         (uint32_t)(millis() - lastLoggerPing) < LOGGER_TIMEOUT_MS;
}

uint32_t runElapsedMs() {
  if (runId == 0) return 0;
  return recording
    ? (uint32_t)(millis() - runStartMs)
    : (uint32_t)(runStopMs - runStartMs);
}

void jsonReply(int statusCode, const String& payload) {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.send(statusCode, "application/json", payload);
}

void sendError(int statusCode, const char* reason) {
  String out = "{\"ok\":false,\"error\":\"";
  out += reason;
  out += "\"}";
  jsonReply(statusCode, out);
}

void sendOK(const char* msg) {
  message = msg;
  jsonReply(200, "{\"ok\":true}");
}

int32_t rawMean() {
  if (rawCount == 0) return latestRaw;

  int64_t total = 0;
  for (size_t i = 0; i < rawCount; ++i) {
    total += rawWindow[i];
  }

  return (int32_t)(total / (int64_t)rawCount);
}

void pushHistory(uint32_t now, int32_t raw, float gf) {
  history[historyHead] = {
    ++sequence,
    now,
    raw,
    gf,
    temperatureOnline() ? latestTempC : NAN
  };

  historyHead = (historyHead + 1) % HISTORY_SIZE;

  if (historyCount < HISTORY_SIZE) {
    ++historyCount;
  }

  if (recording) {
    ++recordedCount;
  }
}

void pushTempHistory(uint32_t now, float c) {
  tempHistory[tempHead] = {++tempSequence, now, c};
  tempHead = (tempHead + 1) % TEMP_HISTORY_SIZE;

  if (tempCount < TEMP_HISTORY_SIZE) {
    ++tempCount;
  }
}

void sampleHX711() {
  if (!loadCell.is_ready()) return;

  const int32_t raw = (int32_t)loadCell.read();
  const uint32_t now = millis();

  seenReading = true;
  latestReadMs = now;
  latestRaw = raw;

  rawWindow[rawHead] = raw;
  rawHead = (rawHead + 1) % RAW_WINDOW;

  if (rawCount < RAW_WINDOW) {
    ++rawCount;
  }

  ++rateCount;

  if (!measurementReady()) return;

  latestGF = (float)((double(raw) - double(tareRaw)) / double(countsPerGF));

  if (!isfinite(latestGF)) return;

  const float magnitude = fabsf(latestGF);

  if (magnitude > peakGF) {
    peakGF = magnitude;
  }

  pushHistory(now, raw, latestGF);
}

void serviceTemperature() {
  const uint32_t now = millis();

  if (!tempConversionPending) {
    if ((uint32_t)(now - lastTempRequestMs) >= TEMP_SAMPLE_PERIOD_MS) {
      temperatureBus.requestTemperatures();
      lastTempRequestMs = now;
      tempConversionPending = true;
    }
    return;
  }

  if ((uint32_t)(now - lastTempRequestMs) < TEMP_CONVERSION_MS) {
    return;
  }

  tempConversionPending = false;

  const float c = temperatureBus.getTempCByIndex(0);

  if (
    c == DEVICE_DISCONNECTED_C ||
    !isfinite(c) ||
    c < -55.0f ||
    c > 125.0f
  ) {
    return;
  }

  latestTempC = c;
  latestTempMs = now;
  seenTemperature = true;

  if (!isfinite(minTempC) || c < minTempC) minTempC = c;
  if (!isfinite(maxTempC) || c > maxTempC) maxTempC = c;

  pushTempHistory(now, c);
}

void stopRun(const char* reason) {
  if (!recording) return;

  runEndSeq = sequence;
  runStopMs = millis();
  recording = false;
  message = reason;

  Serial.printf(
    "RUN %lu stopped: %s (%lu samples)\n",
    (unsigned long)runId,
    reason,
    (unsigned long)recordedCount
  );
}

void handleTare() {
  if (recording) {
    sendError(409, "Stop recording before taring.");
    return;
  }

  if (!sensorOnline() || rawCount < 8) {
    sendError(409, "HX711 is not ready.");
    return;
  }

  tareRaw = rawMean();
  tared = true;
  latestGF = 0.0f;
  peakGF = 0.0f;

  historyHead = 0;
  historyCount = 0;
  ++sensorEpoch;

  sendOK("Scale zeroed. Verify calibration using a known load.");
}

void handleCalibrate() {
  if (recording) {
    sendError(409, "Stop recording before calibration.");
    return;
  }

  if (!sensorOnline() || !tared || rawCount < 8) {
    sendError(409, "Tare the unloaded sensor first.");
    return;
  }

  if (!server.hasArg("grams")) {
    sendError(400, "Reference force required.");
    return;
  }

  const float knownGF = server.arg("grams").toFloat();

  if (!isfinite(knownGF) || knownGF < 1.0f || knownGF > 100000.0f) {
    sendError(400, "Reference must be between 1 and 100000 gf.");
    return;
  }

  const int32_t difference = rawMean() - tareRaw;

  if (abs(difference) < 100) {
    sendError(409, "Reference force too small or sensor disconnected.");
    return;
  }

  countsPerGF = (float)((double)difference / (double)knownGF);
  preferences.putFloat("factor", countsPerGF);

  peakGF = 0.0f;
  historyHead = 0;
  historyCount = 0;
  ++sensorEpoch;

  sendOK("Calibration saved. Remove reference load, then TARE again.");
}

void handleTemperatureThresholds() {
  if (!server.hasArg("warning") || !server.hasArg("danger")) {
    sendError(400, "Both warning and danger temperatures are required.");
    return;
  }

  const float warning = server.arg("warning").toFloat();
  const float danger = server.arg("danger").toFloat();

  if (
    !isfinite(warning) ||
    !isfinite(danger) ||
    warning < 20.0f ||
    danger > 120.0f ||
    danger <= warning + 2.0f
  ) {
    sendError(400, "Use 20-120 C and keep danger at least 2 C above warning.");
    return;
  }

  warningTempC = warning;
  dangerTempC = danger;

  preferences.putFloat("tempWarn", warningTempC);
  preferences.putFloat("tempDanger", dangerTempC);

  sendOK("Temperature alarm thresholds saved.");
}

void handleStart() {
  if (recording) {
    sendError(409, "Already recording.");
    return;
  }

  if (!measurementReady()) {
    sendError(409, "Sensor must be tared and calibrated.");
    return;
  }

  if (!loggerOnline()) {
    sendError(503, "PC logger offline. Run pc_logger.py on the computer.");
    return;
  }

  ++runId;
  runStartSeq = sequence + 1;
  runStartMs = millis();
  runStopMs = runStartMs;
  runEndSeq = 0;
  recordedCount = 0;
  peakGF = 0.0f;

  minTempC = temperatureOnline() ? latestTempC : NAN;
  maxTempC = temperatureOnline() ? latestTempC : NAN;

  recording = true;

  sendOK("Endurance recording started. Samples stream to the PC over Wi-Fi.");
}

void handleStatus() {
  String out;
  out.reserve(1400);

  out += "{\"ok\":true";

  out += ",\"sensor\":";
  out += sensorOnline() ? "true" : "false";

  out += ",\"calibrated\":";
  out += calibrationValid() ? "true" : "false";

  out += ",\"tared\":";
  out += tared ? "true" : "false";

  out += ",\"recording\":";
  out += recording ? "true" : "false";

  out += ",\"pcOnline\":";
  out += loggerOnline() ? "true" : "false";

  out += ",\"gf\":";
  out += String(latestGF, 3);

  out += ",\"peakgf\":";
  out += String(peakGF, 3);

  out += ",\"sps\":";
  out += String(samplesPerSecond, 1);

  out += ",\"raw\":";
  out += String(latestRaw);

  out += ",\"factor\":";
  out += String(countsPerGF, 6);

  out += ",\"tempOnline\":";
  out += temperatureOnline() ? "true" : "false";

  out += ",\"tempC\":";
  out += temperatureOnline() ? String(latestTempC, 2) : "null";

  out += ",\"tempF\":";
  out += temperatureOnline()
    ? String(latestTempC * 9.0f / 5.0f + 32.0f, 2)
    : "null";

  out += ",\"tempMinC\":";
  out += isfinite(minTempC) ? String(minTempC, 2) : "null";

  out += ",\"tempMaxC\":";
  out += isfinite(maxTempC) ? String(maxTempC, 2) : "null";

  out += ",\"tempWarningC\":";
  out += String(warningTempC, 1);

  out += ",\"tempDangerC\":";
  out += String(dangerTempC, 1);

  out += ",\"tempSeq\":";
  out += String(tempSequence);

  out += ",\"uptimeMs\":";
  out += String(millis());

  out += ",\"latestReadMs\":";
  out += String(latestReadMs);

  out += ",\"latestTempMs\":";
  out += String(latestTempMs);

  out += ",\"epoch\":";
  out += String(sensorEpoch);

  out += ",\"boot\":";
  out += String(bootId);

  out += ",\"run\":";
  out += String(runId);

  out += ",\"runStartMs\":";
  out += String(runStartMs);

  out += ",\"runElapsedMs\":";
  out += String(runElapsedMs());

  out += ",\"runStartSeq\":";
  out += String(runStartSeq);

  out += ",\"runEndSeq\":";
  out += String(runEndSeq);

  out += ",\"lastSeq\":";
  out += String(sequence);

  out += ",\"count\":";
  out += String(recordedCount);

  out += ",\"message\":\"";
  out += message;
  out += "\"}";

  jsonReply(200, out);
}

void handleSamples() {
  const uint32_t after =
    server.hasArg("after")
      ? (uint32_t)strtoul(server.arg("after").c_str(), nullptr, 10)
      : 0;

  size_t limit =
    server.hasArg("limit")
      ? (size_t)server.arg("limit").toInt()
      : API_BATCH_SIZE;

  if (limit < 1 || limit > API_BATCH_SIZE) {
    limit = API_BATCH_SIZE;
  }

  const size_t first =
    (historyHead + HISTORY_SIZE - historyCount) % HISTORY_SIZE;

  const uint32_t oldestSeq =
    historyCount ? history[first].id : sequence + 1;

  String out;
  out.reserve(8000);

  out += "{\"ok\":true,\"boot\":";
  out += String(bootId);

  out += ",\"epoch\":";
  out += String(sensorEpoch);

  out += ",\"oldestSeq\":";
  out += String(oldestSeq);

  out += ",\"lastSeq\":";
  out += String(sequence);

  out += ",\"points\":[";

  size_t emitted = 0;

  for (size_t i = 0; i < historyCount && emitted < limit; ++i) {
    const Reading& point =
      history[(first + i) % HISTORY_SIZE];

    if (point.id <= after) continue;

    if (emitted++) out += ',';

    out += '[';
    out += String(point.id);
    out += ',';
    out += String(point.timeMs);
    out += ',';
    out += String(point.gf, 3);
    out += ',';
    out += String(point.raw);
    out += ',';
    out += isfinite(point.tempC) ? String(point.tempC, 2) : "null";
    out += ']';
  }

  out += "]}";

  jsonReply(200, out);
}

void handleTemperatureHistory() {
  const uint32_t after =
    server.hasArg("after")
      ? (uint32_t)strtoul(server.arg("after").c_str(), nullptr, 10)
      : 0;

  size_t limit =
    server.hasArg("limit")
      ? (size_t)server.arg("limit").toInt()
      : TEMP_API_BATCH_SIZE;

  if (limit < 1 || limit > TEMP_API_BATCH_SIZE) {
    limit = TEMP_API_BATCH_SIZE;
  }

  const size_t first =
    (tempHead + TEMP_HISTORY_SIZE - tempCount) % TEMP_HISTORY_SIZE;

  const uint32_t oldestSeq =
    tempCount ? tempHistory[first].id : tempSequence + 1;

  String out;
  out.reserve(5200);

  out += "{\"ok\":true,\"boot\":";
  out += String(bootId);

  out += ",\"oldestSeq\":";
  out += String(oldestSeq);

  out += ",\"lastSeq\":";
  out += String(tempSequence);

  out += ",\"points\":[";

  size_t emitted = 0;

  for (size_t i = 0; i < tempCount && emitted < limit; ++i) {
    const TempReading& point =
      tempHistory[(first + i) % TEMP_HISTORY_SIZE];

    if (point.id <= after) continue;

    if (emitted++) out += ',';

    out += '[';
    out += String(point.id);
    out += ',';
    out += String(point.timeMs);
    out += ',';
    out += String(point.c, 2);
    out += ']';
  }

  out += "]}";

  jsonReply(200, out);
}

void handleLoggerPing() {
  lastLoggerPing = millis();
  loggerEverPinged = true;
  jsonReply(200, "{\"ok\":true}");
}

void setup() {
  Serial.begin(115200);

  preferences.begin("thrustbench", false);

  countsPerGF = preferences.getFloat("factor", 0.0f);
  warningTempC = preferences.getFloat("tempWarn", 60.0f);
  dangerTempC = preferences.getFloat("tempDanger", 70.0f);

  if (
    !isfinite(warningTempC) ||
    !isfinite(dangerTempC) ||
    warningTempC < 20.0f ||
    dangerTempC > 120.0f ||
    dangerTempC <= warningTempC + 2.0f
  ) {
    warningTempC = 60.0f;
    dangerTempC = 70.0f;
  }

  bootId = esp_random();

  loadCell.begin(HX_DOUT, HX_SCK);

  temperatureBus.begin();
  temperatureBus.setResolution(11);
  temperatureBus.setWaitForConversion(false);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  const bool started = WiFi.softAP(AP_NAME, AP_PASSWORD);

  Serial.printf(
    "AP: %s | Wi-Fi: %s | IP: %s\n",
    AP_NAME,
    started ? "OK" : "FAILED",
    WiFi.softAPIP().toString().c_str()
  );

  Serial.printf(
    "Temperature probe: DS18B20 on GPIO %u | devices=%u\n",
    TEMP_PIN,
    temperatureBus.getDeviceCount()
  );

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.send_P(200, "text/html", DASHBOARD_HTML);
  });

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/samples", HTTP_GET, handleSamples);
  server.on("/api/temperature", HTTP_GET, handleTemperatureHistory);

  server.on("/api/logger/ping", HTTP_GET, handleLoggerPing);
  server.on("/api/tare", HTTP_POST, handleTare);
  server.on("/api/calibrate", HTTP_POST, handleCalibrate);
  server.on("/api/temperature/thresholds", HTTP_POST, handleTemperatureThresholds);
  server.on("/api/start", HTTP_POST, handleStart);

  server.on("/api/stop", HTTP_POST, []() {
    if (!recording) {
      sendError(409, "No active recording.");
      return;
    }

    stopRun("Endurance recording stopped. PC logger is finalizing the CSV.");
    jsonReply(200, "{\"ok\":true}");
  });

  server.on("/api/resetpeak", HTTP_POST, []() {
    peakGF = 0.0f;
    sendOK("Peak cleared.");
  });

  server.onNotFound([]() {
    sendError(404, "Unknown route.");
  });

  server.begin();

  rateStartMs = millis();
  lastTempRequestMs = millis() - TEMP_SAMPLE_PERIOD_MS;
}

void loop() {
  sampleHX711();
  serviceTemperature();

  const uint32_t now = millis();

  if ((uint32_t)(now - rateStartMs) >= 1000) {
    const uint32_t elapsed = now - rateStartMs;

    samplesPerSecond =
      elapsed ? (1000.0f * rateCount / elapsed) : 0.0f;

    rateCount = 0;
    rateStartMs = now;
  }

  if (recording && !sensorOnline()) {
    stopRun("Load-cell sensor stopped responding. Recording stopped.");
  }

  if (recording && !loggerOnline()) {
    stopRun("PC logger disconnected. Recording stopped; check CSV for gaps.");
  }

  server.handleClient();
  delay(1);
}
