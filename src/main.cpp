#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HX711.h>
#include <math.h>
#include "dashboard.h"

static constexpr uint8_t HX_DOUT = 19;
static constexpr uint8_t HX_SCK = 18;

static const char* AP_NAME = "ThrustBench-ESP32";
static const char* AP_PASSWORD = "ThrustTest123";

static constexpr uint32_t SENSOR_TIMEOUT_MS = 1500;
static constexpr uint32_t LOGGER_TIMEOUT_MS = 4500;
static constexpr size_t RAW_WINDOW = 16;
static constexpr size_t HISTORY_SIZE = 2048;
static constexpr size_t API_BATCH_SIZE = 96;

HX711 loadCell;
WebServer server(80);
Preferences preferences;

struct Reading {
  uint32_t id;
  uint32_t timeMs;
  int32_t raw;
  float gf;
};

Reading history[HISTORY_SIZE];
size_t historyHead = 0;
size_t historyCount = 0;
uint32_t sequence = 0;
uint32_t sensorEpoch = 1;
uint32_t bootId = 0;

int32_t rawWindow[RAW_WINDOW] = {};
size_t rawCount = 0;
size_t rawHead = 0;

bool seenReading = false;
bool tared = false;
bool recording = false;
bool loggerEverPinged = false;

int32_t latestRaw = 0;
int32_t tareRaw = 0;
float countsPerGF = 0.0f;
float latestGF = 0.0f;
float peakGF = 0.0f;
float samplesPerSecond = 0.0f;

uint32_t latestReadMs = 0;
uint32_t rateStartMs = 0;
uint32_t rateCount = 0;
uint32_t lastLoggerPing = 0;

uint32_t runId = 0;
uint32_t runStartMs = 0;
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

bool measurementReady() {
  return sensorOnline() && tared && calibrationValid();
}

bool loggerOnline() {
  return loggerEverPinged &&
         (uint32_t)(millis() - lastLoggerPing) < LOGGER_TIMEOUT_MS;
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
  history[historyHead] = {++sequence, now, raw, gf};
  historyHead = (historyHead + 1) % HISTORY_SIZE;

  if (historyCount < HISTORY_SIZE) {
    ++historyCount;
  }

  if (recording) {
    ++recordedCount;
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
  if (rawCount < RAW_WINDOW) ++rawCount;

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

void stopRun(const char* reason) {
  if (!recording) return;

  runEndSeq = sequence;
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
  runEndSeq = 0;
  recordedCount = 0;
  peakGF = 0.0f;
  recording = true;

  sendOK("Recording started. Samples are streaming to the PC over Wi-Fi.");
}

void handleStatus() {
  String out;
  out.reserve(900);

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
  out += ",\"uptimeMs\":";
  out += String(millis());
  out += ",\"latestReadMs\":";
  out += String(latestReadMs);
  out += ",\"epoch\":";
  out += String(sensorEpoch);
  out += ",\"boot\":";
  out += String(bootId);
  out += ",\"run\":";
  out += String(runId);
  out += ",\"runStartMs\":";
  out += String(runStartMs);
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
  out.reserve(6500);

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

  bootId = esp_random();

  loadCell.begin(HX_DOUT, HX_SCK);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  const bool started = WiFi.softAP(AP_NAME, AP_PASSWORD);

  Serial.printf(
    "AP: %s | Wi-Fi: %s | IP: %s\n",
    AP_NAME,
    started ? "OK" : "FAILED",
    WiFi.softAPIP().toString().c_str()
  );

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.send_P(200, "text/html", DASHBOARD_HTML);
  });

  // Lightweight status polling is deliberately separate from sample transfer.
  // This prevents chart/logger traffic from freezing the live numeric display.
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/samples", HTTP_GET, handleSamples);

  server.on("/api/logger/ping", HTTP_GET, handleLoggerPing);
  server.on("/api/tare", HTTP_POST, handleTare);
  server.on("/api/calibrate", HTTP_POST, handleCalibrate);
  server.on("/api/start", HTTP_POST, handleStart);

  server.on("/api/stop", HTTP_POST, []() {
    if (!recording) {
      sendError(409, "No active recording.");
      return;
    }

    stopRun("Recording stopped. PC logger is finalizing the CSV.");
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
}

void loop() {
  sampleHX711();

  const uint32_t now = millis();

  if ((uint32_t)(now - rateStartMs) >= 1000) {
    const uint32_t elapsed = now - rateStartMs;

    samplesPerSecond =
      elapsed ? (1000.0f * rateCount / elapsed) : 0.0f;

    rateCount = 0;
    rateStartMs = now;
  }

  if (recording && !sensorOnline()) {
    stopRun("Sensor stopped responding. Recording stopped.");
  }

  if (recording && !loggerOnline()) {
    stopRun("PC logger disconnected. Recording stopped; check CSV for gaps.");
  }

  server.handleClient();
  delay(1);
}
