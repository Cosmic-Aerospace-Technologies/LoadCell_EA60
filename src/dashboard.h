#pragma once
#include <Arduino.h>

static const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#081017">
<title>THRUST // LAB</title>
<style>
:root{--bg:#080e14;--panel:#101a22;--border:#23343f;--text:#e8f2f5;--muted:#7895a3;--green:#43f4c0;--amber:#f4bc65;--red:#ff5f69;--blue:#55c8ff}
*{box-sizing:border-box}
body{margin:0;background:radial-gradient(circle at 70% -20%,#20343f 0,transparent 42%),var(--bg);color:var(--text);font-family:Inter,Segoe UI,Arial,sans-serif}
.wrap{max-width:1400px;margin:auto;padding:28px 22px 50px}
.top{display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--border);padding-bottom:20px}
.brand{font-weight:850;letter-spacing:.14em;font-size:20px}.sub{font:10px Consolas,monospace;color:var(--muted);letter-spacing:.13em;margin-top:4px}
.link{font:11px Consolas,monospace;color:var(--muted)}.dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:var(--red);margin-right:7px}.dot.ok{background:var(--green);box-shadow:0 0 12px #43f4c077}
h1{font-size:clamp(28px,4vw,42px);margin:30px 0 4px}.lead{color:var(--muted);margin:0 0 18px}
.grid{display:grid;grid-template-columns:repeat(12,1fr);gap:16px}
.card{background:linear-gradient(155deg,#14212a,#0e171f);border:1px solid var(--border);border-radius:12px;padding:20px;min-width:0;transition:border-color .25s,box-shadow .25s,background .25s}
.live,.temperature,.controls{grid-column:span 5}.graph,.tempgraph,.endurance{grid-column:span 7}
.title{display:flex;justify-content:space-between;gap:12px;color:var(--muted);font:700 11px Consolas,monospace;letter-spacing:.12em;text-transform:uppercase;margin-bottom:18px}
.big{font-size:clamp(56px,7vw,88px);font-weight:750;color:var(--green);letter-spacing:-.07em;font-variant-numeric:tabular-nums;transition:color .25s,text-shadow .25s}
.tempbig{font-size:clamp(62px,8vw,96px);font-weight:760;letter-spacing:-.06em;font-variant-numeric:tabular-nums;transition:color .25s,text-shadow .25s}
.unit{font:700 14px Consolas,monospace;color:var(--muted);margin-left:10px}.updated{font:10px Consolas,monospace;color:var(--muted);margin-top:6px}
.metrics{display:grid;grid-template-columns:1fr 1fr;gap:20px 12px;margin-top:24px;padding-top:20px;border-top:1px solid var(--border)}
.ml{font:10px Consolas,monospace;color:var(--muted);text-transform:uppercase;letter-spacing:.08em}.mv{font:600 24px Consolas,monospace;margin-top:6px}
.chartbox{height:300px}canvas{width:100%;height:100%;display:block}.foot{display:flex;justify-content:space-between;color:var(--muted);font:10px Consolas,monospace;margin-top:10px}
button{padding:12px 15px;border-radius:7px;border:1px solid #39505d;background:#152630;color:var(--text);font:700 11px Consolas,monospace;cursor:pointer}
button:hover{border-color:var(--green)}button:disabled{opacity:.35;cursor:not-allowed}.primary{background:var(--green);color:#04140e;border-color:var(--green)}.dangerbtn{background:#351c27;color:var(--red);border-color:#87404d}
.actions{display:grid;grid-template-columns:1fr 1fr;gap:9px}.section{border-top:1px solid var(--border);margin-top:17px;padding-top:17px}
.row{display:flex;gap:8px}.row input{min-width:0;flex:1;background:#080f15;color:var(--text);border:1px solid #39505d;border-radius:7px;padding:11px;font:14px Consolas,monospace}
.label{font:10px Consolas,monospace;color:var(--muted);text-transform:uppercase;letter-spacing:.1em;margin-bottom:9px}
.msg{font:11px Consolas,monospace;color:var(--amber);margin-top:14px;min-height:34px;line-height:1.5}.help{color:var(--muted);font-size:12px;line-height:1.65}
.pc{font:700 12px Consolas,monospace;color:var(--red)}.pc.on{color:var(--green)}.record{color:var(--muted);font:11px Consolas,monospace}.record.on{color:var(--red)}
.tempbar{height:8px;border-radius:999px;margin-top:20px;background:linear-gradient(90deg,#55c8ff 0%,#43f4c0 38%,#f4bc65 70%,#ff5f69 100%);position:relative;overflow:visible}
.tempmarker{position:absolute;top:-4px;width:4px;height:16px;background:#fff;border-radius:2px;box-shadow:0 0 8px #fff8;transform:translateX(-50%);transition:left .35s ease}
.thresholds{display:grid;grid-template-columns:1fr 1fr;gap:9px}.thresholds label{font:10px Consolas,monospace;color:var(--muted)}.thresholds input{width:100%;margin-top:5px;background:#080f15;color:var(--text);border:1px solid #39505d;border-radius:7px;padding:10px;font:14px Consolas,monospace}
.timer{font:700 clamp(44px,7vw,78px) Consolas,monospace;letter-spacing:-.06em;color:var(--text);font-variant-numeric:tabular-nums}
.alarm{display:inline-block;padding:7px 9px;border:1px solid var(--border);border-radius:6px;font:700 11px Consolas,monospace;color:var(--muted)}
.alarm.warn{color:var(--amber);border-color:#775b2f;background:#2d2618}.alarm.danger{color:#fff;border-color:var(--red);background:#5c1820}
.temperature.warn{border-color:#8b6832;box-shadow:0 0 24px #f4bc6518}
.temperature.danger{border-color:var(--red);box-shadow:0 0 35px #ff5f6940;animation:dangerPulse .8s infinite alternate}
body.dangerAlarm::before{content:"MOTOR TEMPERATURE DANGER";position:fixed;z-index:20;top:0;left:0;right:0;text-align:center;padding:7px;background:var(--red);color:#170307;font:900 12px Consolas,monospace;letter-spacing:.16em;animation:bannerBlink .65s infinite alternate}
@keyframes dangerPulse{from{box-shadow:0 0 18px #ff5f6928}to{box-shadow:0 0 55px #ff5f6980}}
@keyframes bannerBlink{from{opacity:1}to{opacity:.55}}
@media(max-width:900px){.live,.graph,.temperature,.tempgraph,.controls,.endurance{grid-column:span 12}}
@media(max-width:520px){.wrap{padding:18px 12px 36px}.card{padding:15px}.metrics{grid-template-columns:1fr 1fr}.chartbox{height:250px}.thresholds{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div><div class="brand">THRUST // LAB</div><div class="sub">ESP32 · HX711 · DS18B20 · ENDURANCE TELEMETRY</div></div>
    <div class="link"><span id="connectionDot" class="dot"></span><span id="connectionLabel">CONNECTING</span> · <span id="deviceIP"></span></div>
  </div>

  <h1>Motor Endurance Test Bench</h1>
  <p class="lead">Standalone ESP32 Wi-Fi · live thrust + temperature · PC-side CSV recording</p>

  <div class="grid">
    <section class="card live">
      <div class="title"><span>01 / Live force</span><span id="sensorLabel">CHECKING</span></div>
      <div><span id="thrust" class="big">--</span><span class="unit">gf</span></div>
      <div id="updatedLabel" class="updated">WAITING FOR ESP32</div>
      <div class="metrics">
        <div><div class="ml">Kilograms-force</div><div class="mv"><span id="kgf">--</span> <small>kgf</small></div></div>
        <div><div class="ml">Pounds-force</div><div class="mv"><span id="lbf">--</span> <small>lbf</small></div></div>
        <div><div class="ml">Newtons</div><div class="mv"><span id="newtons">--</span> <small>N</small></div></div>
        <div><div class="ml">Peak |force|</div><div class="mv"><span id="peak">--</span> <small>gf</small></div></div>
        <div><div class="ml">HX711 sample rate</div><div class="mv"><span id="sps">--</span> <small>SPS</small></div></div>
        <div><div class="ml">Raw ADC</div><div class="mv" id="raw">--</div></div>
      </div>
    </section>

    <section class="card graph">
      <div class="title"><span>02 / Force waveform</span><span>LAST 30 S</span></div>
      <div class="chartbox"><canvas id="forcePlot"></canvas></div>
      <div class="foot"><span>FORCE [gf]</span><span>TIME [s] · AUTO SCALE</span></div>
    </section>

    <section id="temperatureCard" class="card temperature">
      <div class="title"><span>03 / Motor temperature</span><span id="tempSensorLabel">CHECKING</span></div>
      <div><span id="tempValue" class="tempbig">--</span><span class="unit">°C</span></div>
      <div class="updated"><span id="tempF">--</span> °F · SURFACE PROBE</div>
      <div class="tempbar"><span id="tempMarker" class="tempmarker" style="left:0%"></span></div>
      <div class="metrics">
        <div><div class="ml">Minimum this run</div><div class="mv"><span id="tempMin">--</span> <small>°C</small></div></div>
        <div><div class="ml">Maximum this run</div><div class="mv"><span id="tempMax">--</span> <small>°C</small></div></div>
        <div><div class="ml">Warning threshold</div><div class="mv"><span id="warnReadout">60</span> <small>°C</small></div></div>
        <div><div class="ml">Danger threshold</div><div class="mv"><span id="dangerReadout">70</span> <small>°C</small></div></div>
      </div>
    </section>

    <section class="card tempgraph">
      <div class="title"><span>04 / Thermal history</span><span>LAST 30 MIN</span></div>
      <div class="chartbox"><canvas id="tempPlot"></canvas></div>
      <div class="foot"><span>TEMPERATURE [°C]</span><span>TIME [min] · WARNING/DANGER LINES</span></div>
    </section>

    <section class="card controls">
      <div class="title"><span>05 / Test control</span><span id="recordLabel" class="record">STANDBY</span></div>
      <div class="actions">
        <button id="startBtn" class="primary">● START ENDURANCE TEST</button>
        <button id="stopBtn" class="dangerbtn" disabled>■ STOP & SAVE</button>
      </div>

      <div class="section">
        <div class="label">Scale</div>
        <button id="tareBtn">TARE</button>
        <button id="peakResetBtn">RESET PEAK</button>
      </div>

      <div class="section">
        <div class="label">Known calibration load [gf]</div>
        <div class="row">
          <input id="calMass" type="number" min="1" max="100000" value="207">
          <button id="calBtn">CALIBRATE</button>
        </div>
      </div>

      <div class="section">
        <div class="label">Temperature alarms [°C]</div>
        <div class="thresholds">
          <label>WARNING<input id="warningInput" type="number" min="20" max="115" step="1" value="60"></label>
          <label>DANGER<input id="dangerInput" type="number" min="22" max="120" step="1" value="70"></label>
        </div>
        <div class="row" style="margin-top:9px">
          <button id="thresholdBtn">SAVE THRESHOLDS</button>
          <button id="soundBtn">ENABLE ALARM SOUND</button>
        </div>
      </div>

      <div id="statusMessage" class="msg">SYSTEM / Initializing...</div>
    </section>

    <section class="card endurance">
      <div class="title"><span>06 / Endurance status</span><span id="alarmState" class="alarm">THERMAL NORMAL</span></div>
      <div class="ml">Elapsed test time</div>
      <div id="elapsed" class="timer">00:00:00</div>
      <div class="metrics">
        <div><div class="ml">PC logger</div><div id="pcStatus" class="mv pc">OFFLINE</div></div>
        <div><div class="ml">Run</div><div class="mv">#<span id="runId">0</span></div></div>
        <div><div class="ml">Recorded samples</div><div id="samplesLabel" class="mv">0</div></div>
        <div><div class="ml">Audio alarm</div><div id="soundStatus" class="mv">MUTED</div></div>
      </div>
      <p class="help">Connect this computer to <b>ThrustBench-ESP32</b> and run <code>py -3 pc_logger.py</code>. CSV files are written directly to the computer. Keep this dashboard open for visual/audio alarms.</p>
      <p id="sessionLabel" class="help">IDLE / NO ACTIVE TEST</p>
    </section>
  </div>
</div>

<script>
(() => {
  "use strict";

  const $ = id => document.getElementById(id);
  const forceCanvas = $("forcePlot");
  const tempCanvas = $("tempPlot");
  const fctx = forceCanvas.getContext("2d");
  const tctx = tempCanvas.getContext("2d");

  const forceData = [];
  const tempData = [];

  const state = {
    connected:false, ready:false, recording:false, busy:false, pcOnline:false,
    targetGF:0, shownGF:null, peakGF:0,
    tempOnline:false, tempC:null, tempMin:null, tempMax:null,
    warningC:60, dangerC:70,
    boot:null, epoch:null, forceSeq:0, tempSeq:0,
    forceSensorTime:0, forceReceivedAt:0,
    tempSensorTime:0, tempReceivedAt:0,
    lastStatusAt:0, lastFrame:performance.now(),
    forceMin:null, forceMax:null, tempScaleMin:null, tempScaleMax:null,
    lastMessage:null, statusCount:0, error:null,
    runId:0, elapsedBase:0, elapsedReceivedAt:0,
    soundEnabled:false, audioContext:null, lastAlarmSoundAt:0
  };

  $("deviceIP").textContent = location.host;

  const setText = (id, value) => {
    const el = $(id);
    if (el) el.textContent = value;
  };

  const fmt = (value, digits) =>
    Number.isFinite(Number(value)) ? Number(value).toFixed(digits) : "--";

  function showMessage(text, isError=false) {
    setText("statusMessage", (isError ? "ERROR / " : "SYSTEM / ") + text);
    $("statusMessage").style.color = isError ? "var(--red)" : "var(--amber)";
  }

  async function api(path, options={}, timeoutMs=2000) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), timeoutMs);

    try {
      const separator = path.includes("?") ? "&" : "?";
      const response = await fetch(path + separator + "_=" + Date.now(), {
        ...options,
        cache:"no-store",
        signal:controller.signal,
        headers:{"Cache-Control":"no-cache", ...(options.headers || {})}
      });

      let body = {};
      try { body = await response.json(); } catch (_) {}

      if (!response.ok) {
        throw new Error(body.error || ("HTTP " + response.status));
      }

      return body;
    } finally {
      clearTimeout(timer);
    }
  }

  function renderControls() {
    $("startBtn").disabled =
      state.busy || !state.connected || !state.ready ||
      state.recording || !state.pcOnline;

    $("stopBtn").disabled =
      state.busy || !state.connected || !state.recording;

    $("tareBtn").disabled =
      state.busy || !state.connected || state.recording;

    $("calBtn").disabled =
      state.busy || !state.connected || state.recording;

    $("peakResetBtn").disabled =
      state.busy || !state.connected;

    $("thresholdBtn").disabled =
      state.busy || !state.connected;

    $("recordLabel").textContent =
      state.recording ? "● RECORDING" : "STANDBY";

    $("recordLabel").classList.toggle("on", state.recording);
  }

  async function command(path) {
    if (state.busy) return;

    state.busy = true;
    renderControls();

    try {
      await api(path, {method:"POST"});
      await updateStatus();
    } catch (err) {
      showMessage(err.message || String(err), true);
    } finally {
      state.busy = false;
      renderControls();
    }
  }

  $("startBtn").onclick = () => command("/api/start");
  $("stopBtn").onclick = () => command("/api/stop");
  $("tareBtn").onclick = () => command("/api/tare");
  $("peakResetBtn").onclick = () => command("/api/resetpeak");

  $("calBtn").onclick = () => {
    const grams = Number($("calMass").value);

    if (!Number.isFinite(grams) || grams < 1 || grams > 100000) {
      showMessage("Calibration reference must be between 1 and 100000 gf.", true);
      return;
    }

    command("/api/calibrate?grams=" + encodeURIComponent(grams));
  };

  $("thresholdBtn").onclick = () => {
    const warning = Number($("warningInput").value);
    const danger = Number($("dangerInput").value);

    if (!Number.isFinite(warning) || !Number.isFinite(danger) ||
        warning < 20 || danger > 120 || danger <= warning + 2) {
      showMessage("Use 20-120 °C and keep danger at least 2 °C above warning.", true);
      return;
    }

    command(
      "/api/temperature/thresholds?warning=" +
      encodeURIComponent(warning) +
      "&danger=" +
      encodeURIComponent(danger)
    );
  };

  $("soundBtn").onclick = async () => {
    try {
      if (!state.audioContext) {
        const AudioCtor = window.AudioContext || window.webkitAudioContext;
        if (!AudioCtor) throw new Error("Browser audio is unavailable.");
        state.audioContext = new AudioCtor();
      }

      await state.audioContext.resume();
      state.soundEnabled = true;
      setText("soundStatus", "ARMED");
      $("soundBtn").textContent = "ALARM SOUND ENABLED";
      beep(660, 90, 0.05);
      showMessage("Browser temperature alarm sound enabled.");
    } catch (err) {
      showMessage(err.message || String(err), true);
    }
  };

  function beep(frequency=880, duration=120, volume=0.09) {
    if (!state.soundEnabled || !state.audioContext) return;

    const ctx = state.audioContext;
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();

    osc.type = "square";
    osc.frequency.value = frequency;
    gain.gain.value = volume;

    osc.connect(gain);
    gain.connect(ctx.destination);

    const now = ctx.currentTime;
    gain.gain.setValueAtTime(volume, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + duration / 1000);

    osc.start(now);
    osc.stop(now + duration / 1000);
  }

  function resetForceChart() {
    forceData.length = 0;
    state.forceSeq = 0;
    state.forceSensorTime = 0;
    state.forceMin = null;
    state.forceMax = null;
  }

  function resetTempChart() {
    tempData.length = 0;
    state.tempSeq = 0;
    state.tempSensorTime = 0;
    state.tempScaleMin = null;
    state.tempScaleMax = null;
  }

  function tempColor(c) {
    if (!Number.isFinite(c)) return "var(--muted)";

    if (c <= 20) return "hsl(200 90% 66%)";

    if (c < state.warningC) {
      const span = Math.max(1, state.warningC - 20);
      const t = Math.min(1, Math.max(0, (c - 20) / span));
      const hue = 200 + (48 - 200) * t;
      return "hsl(" + hue.toFixed(0) + " 88% 62%)";
    }

    if (c < state.dangerC) {
      const t = (c - state.warningC) / Math.max(1, state.dangerC - state.warningC);
      const hue = 48 * (1 - t);
      return "hsl(" + hue.toFixed(0) + " 92% 61%)";
    }

    return "hsl(0 95% 66%)";
  }

  function updateThermalAlarm(now) {
    const card = $("temperatureCard");
    const alarm = $("alarmState");

    card.classList.remove("warn", "danger");
    alarm.classList.remove("warn", "danger");
    document.body.classList.remove("dangerAlarm");

    if (!state.tempOnline || !Number.isFinite(state.tempC)) {
      alarm.textContent = "TEMP SENSOR OFFLINE";
      return;
    }

    const color = tempColor(state.tempC);
    $("tempValue").style.color = color;
    $("tempValue").style.textShadow = "0 0 26px " + color + "40";

    const maxScale = Math.max(100, state.dangerC + 20);
    const marker = Math.max(0, Math.min(100, (state.tempC / maxScale) * 100));
    $("tempMarker").style.left = marker + "%";

    if (state.tempC >= state.dangerC) {
      card.classList.add("danger");
      alarm.classList.add("danger");
      document.body.classList.add("dangerAlarm");
      alarm.textContent = "DANGER / REDUCE POWER";

      if (state.soundEnabled && now - state.lastAlarmSoundAt > 1200) {
        beep(1040, 170, 0.12);
        setTimeout(() => beep(820, 170, 0.12), 230);
        state.lastAlarmSoundAt = now;
      }
    } else if (state.tempC >= state.warningC) {
      card.classList.add("warn");
      alarm.classList.add("warn");
      alarm.textContent = "WARNING / HOT";

      if (state.soundEnabled && now - state.lastAlarmSoundAt > 4000) {
        beep(760, 120, 0.07);
        state.lastAlarmSoundAt = now;
      }
    } else {
      alarm.textContent = "THERMAL NORMAL";
    }
  }

  async function updateStatus() {
    const d = await api("/api/status");

    state.connected = true;
    state.lastStatusAt = performance.now();
    state.statusCount++;
    state.recording = !!d.recording;
    state.pcOnline = !!d.pcOnline;
    state.ready = !!(d.sensor && d.calibrated && d.tared);
    state.targetGF = Number(d.gf);
    state.peakGF = Number(d.peakgf);

    state.tempOnline = !!d.tempOnline;
    state.tempC = d.tempC == null ? null : Number(d.tempC);
    state.tempMin = d.tempMinC == null ? null : Number(d.tempMinC);
    state.tempMax = d.tempMaxC == null ? null : Number(d.tempMaxC);
    state.warningC = Number(d.tempWarningC);
    state.dangerC = Number(d.tempDangerC);

    state.runId = Number(d.run);
    state.elapsedBase = Number(d.runElapsedMs || 0);
    state.elapsedReceivedAt = performance.now();

    if (state.boot !== d.boot || state.epoch !== d.epoch) {
      state.boot = d.boot;
      state.epoch = d.epoch;
      state.shownGF = null;
      resetForceChart();
      resetTempChart();
    }

    $("connectionDot").classList.add("ok");
    setText("connectionLabel", "LINK ONLINE");

    setText(
      "sensorLabel",
      !d.sensor ? "SENSOR OFFLINE" :
      !d.calibrated ? "UNCALIBRATED" :
      !d.tared ? "TARE REQUIRED" :
      "SENSOR ONLINE"
    );

    setText(
      "tempSensorLabel",
      state.tempOnline ? "PROBE ONLINE" : "PROBE OFFLINE"
    );

    setText("sps", fmt(d.sps, 1));
    setText("raw", d.sensor ? Number(d.raw).toLocaleString() : "--");
    setText("peak", state.ready ? fmt(state.peakGF, 1) : "--");

    setText("tempValue", state.tempOnline ? fmt(state.tempC, 1) : "--");
    setText("tempF", state.tempOnline ? fmt(d.tempF, 1) : "--");
    setText("tempMin", state.tempMin == null ? "--" : fmt(state.tempMin, 1));
    setText("tempMax", state.tempMax == null ? "--" : fmt(state.tempMax, 1));

    setText("warnReadout", fmt(state.warningC, 0));
    setText("dangerReadout", fmt(state.dangerC, 0));

    if (document.activeElement !== $("warningInput")) {
      $("warningInput").value = fmt(state.warningC, 0);
    }

    if (document.activeElement !== $("dangerInput")) {
      $("dangerInput").value = fmt(state.dangerC, 0);
    }

    setText("samplesLabel", Number(d.count).toLocaleString());
    setText("runId", state.runId.toLocaleString());

    setText(
      "sessionLabel",
      state.recording
        ? "TEST LIVE / DATA STREAMING OVER ESP32 WI-FI"
        : state.runId > 0
          ? "LAST TEST COMPLETE / TIMER FROZEN AT STOP"
          : "IDLE / NO ACTIVE TEST"
    );

    setText("pcStatus", state.pcOnline ? "CONNECTED" : "OFFLINE");
    $("pcStatus").classList.toggle("on", state.pcOnline);

    if (!state.ready) {
      state.shownGF = null;
      ["thrust","kgf","lbf","newtons"].forEach(id => setText(id, "--"));
    }

    if (d.message !== state.lastMessage) {
      state.lastMessage = d.message;
      showMessage(d.message);
    }

    state.error = null;
    renderControls();
  }

  function setOffline(reason) {
    state.connected = false;
    state.ready = false;

    $("connectionDot").classList.remove("ok");
    setText("connectionLabel", "LINK OFFLINE");
    setText("sensorLabel", "CONNECTION LOST");
    setText("updatedLabel", "STALE DATA / DISCONNECTED");

    if (state.error !== reason) {
      showMessage(reason, true);
      state.error = reason;
    }

    renderControls();
  }

  async function statusLoop() {
    try {
      await updateStatus();
    } catch (err) {
      setOffline("Telemetry request failed: " + (err.message || err));
    } finally {
      setTimeout(statusLoop, 250);
    }
  }

  async function forceLoop() {
    try {
      if (state.connected && state.ready) {
        const d = await api(
          "/api/samples?after=" + state.forceSeq + "&limit=96",
          {},
          2200
        );

        if (d.boot !== state.boot || d.epoch !== state.epoch) {
          resetForceChart();
        } else {
          if (state.forceSeq > 0 && Number(d.oldestSeq) > state.forceSeq + 1) {
            state.forceSeq = Number(d.oldestSeq) - 1;
          }

          for (const p of (Array.isArray(d.points) ? d.points : [])) {
            const id = Number(p[0]);
            const t = Number(p[1]);
            const gf = Number(p[2]);

            if (id <= state.forceSeq) continue;

            forceData.push([t, gf]);
            state.forceSeq = id;
            state.forceSensorTime = t;
            state.forceReceivedAt = performance.now();
          }

          if (forceData.length) {
            const cutoff = forceData[forceData.length - 1][0] - 32000;
            while (forceData.length && forceData[0][0] < cutoff) forceData.shift();
          }
        }
      }
    } catch (_) {
    } finally {
      setTimeout(forceLoop, 150);
    }
  }

  async function temperatureLoop() {
    try {
      if (state.connected) {
        const d = await api(
          "/api/temperature?after=" + state.tempSeq + "&limit=120",
          {},
          2200
        );

        if (d.boot !== state.boot) {
          resetTempChart();
        } else {
          if (state.tempSeq > 0 && Number(d.oldestSeq) > state.tempSeq + 1) {
            state.tempSeq = Number(d.oldestSeq) - 1;
          }

          for (const p of (Array.isArray(d.points) ? d.points : [])) {
            const id = Number(p[0]);
            const t = Number(p[1]);
            const c = Number(p[2]);

            if (id <= state.tempSeq) continue;

            tempData.push([t, c]);
            state.tempSeq = id;
            state.tempSensorTime = t;
            state.tempReceivedAt = performance.now();
          }

          if (tempData.length) {
            const cutoff = tempData[tempData.length - 1][0] - 1800000;
            while (tempData.length && tempData[0][0] < cutoff) tempData.shift();
          }
        }
      }
    } catch (_) {
    } finally {
      setTimeout(temperatureLoop, 500);
    }
  }

  function resizeCanvas(canvas, context) {
    const rect = canvas.getBoundingClientRect();
    const dpr = Math.min(window.devicePixelRatio || 1, 2);

    canvas.width = Math.round(rect.width * dpr);
    canvas.height = Math.round(rect.height * dpr);
    context.setTransform(dpr, 0, 0, dpr, 0, 0);
  }

  function resizeAll() {
    resizeCanvas(forceCanvas, fctx);
    resizeCanvas(tempCanvas, tctx);
  }

  addEventListener("resize", resizeAll);
  resizeAll();

  function drawGrid(ctx, w, h, left, right, top, bottom, minV, maxV, xLabels, xFormatter) {
    const pw = w - left - right;
    const ph = h - top - bottom;

    ctx.font = "10px Consolas,monospace";
    ctx.lineWidth = 1;

    for (let i = 0; i <= 4; i++) {
      const yy = top + i * ph / 4;
      ctx.strokeStyle = "#29404b";
      ctx.beginPath();
      ctx.moveTo(left, yy);
      ctx.lineTo(w - right, yy);
      ctx.stroke();

      ctx.fillStyle = "#7895a3";
      ctx.textAlign = "right";
      const value = maxV - i * (maxV - minV) / 4;
      ctx.fillText(value.toFixed(0), left - 7, yy + 3);
    }

    for (let i = 0; i <= xLabels; i++) {
      const xx = left + i * pw / xLabels;
      ctx.strokeStyle = "#21343e";
      ctx.beginPath();
      ctx.moveTo(xx, top);
      ctx.lineTo(xx, h - bottom);
      ctx.stroke();

      ctx.fillStyle = "#7895a3";
      ctx.textAlign = "center";
      ctx.fillText(xFormatter(i), xx, h - 5);
    }
  }

  function drawForce(now, dt) {
    const w = forceCanvas.clientWidth;
    const h = forceCanvas.clientHeight;
    if (w < 50 || h < 50) return;

    fctx.clearRect(0, 0, w, h);

    const left=54,right=12,top=16,bottom=24,pw=w-left-right,ph=h-top-bottom;
    const end = state.forceSensorTime
      ? state.forceSensorTime + Math.min(250, now - state.forceReceivedAt)
      : 30000;
    const begin = end - 30000;
    const visible = forceData.filter(p => p[0] >= begin - 250);

    let low=-20,high=20;
    for (const p of visible) { low=Math.min(low,p[1]); high=Math.max(high,p[1]); }

    const pad=Math.max(20,high-low)*0.12;
    low-=pad; high+=pad;
    const alpha=1-Math.exp(-dt/260);

    if (state.forceMin==null) { state.forceMin=low; state.forceMax=high; }
    else {
      state.forceMin=low<state.forceMin?low:state.forceMin+(low-state.forceMin)*alpha;
      state.forceMax=high>state.forceMax?high:state.forceMax+(high-state.forceMax)*alpha;
    }

    drawGrid(fctx,w,h,left,right,top,bottom,state.forceMin,state.forceMax,5,i=>String(-30+i*6));

    if (visible.length<2) return;

    const x=t=>left+(t-begin)/30000*pw;
    const y=v=>top+(state.forceMax-v)/(state.forceMax-state.forceMin)*ph;

    fctx.save();
    fctx.beginPath(); fctx.rect(left,top,pw,ph); fctx.clip();
    fctx.strokeStyle="#43f4c0"; fctx.lineWidth=2; fctx.lineJoin="round"; fctx.lineCap="round";
    fctx.beginPath(); fctx.moveTo(x(visible[0][0]),y(visible[0][1]));

    for(let i=1;i<visible.length;i++){
      const a=visible[i-1],b=visible[i];
      fctx.quadraticCurveTo(x(a[0]),y(a[1]),(x(a[0])+x(b[0]))/2,(y(a[1])+y(b[1]))/2);
    }

    const last=visible[visible.length-1];
    fctx.lineTo(x(last[0]),y(last[1])); fctx.stroke();
    fctx.fillStyle="#43f4c0"; fctx.beginPath(); fctx.arc(x(last[0]),y(last[1]),3,0,Math.PI*2); fctx.fill();
    fctx.restore();
  }

  function drawTemperature(now, dt) {
    const w=tempCanvas.clientWidth,h=tempCanvas.clientHeight;
    if(w<50||h<50)return;

    tctx.clearRect(0,0,w,h);

    const left=54,right=12,top=16,bottom=24,pw=w-left-right,ph=h-top-bottom;
    const end=state.tempSensorTime
      ? state.tempSensorTime + Math.min(1000,now-state.tempReceivedAt)
      : 1800000;
    const begin=end-1800000;
    const visible=tempData.filter(p=>p[0]>=begin-1000);

    let low=15,high=Math.max(80,state.dangerC+10);
    for(const p of visible){low=Math.min(low,p[1]);high=Math.max(high,p[1]);}

    low-=3;high+=3;
    const alpha=1-Math.exp(-dt/500);

    if(state.tempScaleMin==null){state.tempScaleMin=low;state.tempScaleMax=high;}
    else{
      state.tempScaleMin+= (low-state.tempScaleMin)*alpha;
      state.tempScaleMax+= (high-state.tempScaleMax)*alpha;
    }

    drawGrid(tctx,w,h,left,right,top,bottom,state.tempScaleMin,state.tempScaleMax,6,i=>String(-30+i*5));

    const x=t=>left+(t-begin)/1800000*pw;
    const y=v=>top+(state.tempScaleMax-v)/(state.tempScaleMax-state.tempScaleMin)*ph;

    function thresholdLine(value,color,label){
      if(value<state.tempScaleMin||value>state.tempScaleMax)return;
      const yy=y(value);
      tctx.save();
      tctx.setLineDash([6,5]);
      tctx.strokeStyle=color;
      tctx.lineWidth=1.2;
      tctx.beginPath();tctx.moveTo(left,yy);tctx.lineTo(w-right,yy);tctx.stroke();
      tctx.setLineDash([]);
      tctx.fillStyle=color;tctx.textAlign="right";tctx.font="10px Consolas,monospace";
      tctx.fillText(label+" "+value.toFixed(0)+"°C",w-right-4,yy-5);
      tctx.restore();
    }

    thresholdLine(state.warningC,"#f4bc65","WARN");
    thresholdLine(state.dangerC,"#ff5f69","DANGER");

    if(visible.length<2)return;

    tctx.save();
    tctx.beginPath();tctx.rect(left,top,pw,ph);tctx.clip();
    const gradient=tctx.createLinearGradient(0,top,0,top+ph);
    gradient.addColorStop(0,"#ff5f69");
    gradient.addColorStop(.5,"#f4bc65");
    gradient.addColorStop(1,"#55c8ff");
    tctx.strokeStyle=gradient;tctx.lineWidth=2.3;tctx.lineJoin="round";tctx.lineCap="round";
    tctx.beginPath();tctx.moveTo(x(visible[0][0]),y(visible[0][1]));

    for(let i=1;i<visible.length;i++){
      const a=visible[i-1],b=visible[i];
      tctx.quadraticCurveTo(x(a[0]),y(a[1]),(x(a[0])+x(b[0]))/2,(y(a[1])+y(b[1]))/2);
    }

    const last=visible[visible.length-1];
    tctx.lineTo(x(last[0]),y(last[1]));tctx.stroke();
    tctx.fillStyle=tempColor(last[1]);tctx.beginPath();tctx.arc(x(last[0]),y(last[1]),3.2,0,Math.PI*2);tctx.fill();
    tctx.restore();
  }

  function formatElapsed(ms) {
    const total=Math.max(0,Math.floor(ms/1000));
    const h=Math.floor(total/3600);
    const m=Math.floor((total%3600)/60);
    const s=total%60;
    return [h,m,s].map(v=>String(v).padStart(2,"0")).join(":");
  }

  function frame(now) {
    const dt=Math.min(100,now-state.lastFrame);
    state.lastFrame=now;

    if(state.connected&&state.ready&&Number.isFinite(state.targetGF)){
      if(state.shownGF==null)state.shownGF=state.targetGF;
      const alpha=1-Math.exp(-dt/170);
      state.shownGF+=(state.targetGF-state.shownGF)*alpha;

      setText("thrust",fmt(state.shownGF,1));
      setText("kgf",fmt(state.shownGF/1000,4));
      setText("lbf",fmt(state.shownGF/453.59237,4));
      setText("newtons",fmt(state.shownGF*0.00980665,3));
    }

    let elapsed=state.elapsedBase;
    if(state.recording)elapsed+=Math.max(0,now-state.elapsedReceivedAt);
    setText("elapsed",formatElapsed(elapsed));

    if(state.connected&&now-state.lastStatusAt>2300){
      setOffline("No fresh telemetry received from ESP32.");
    }

    if(state.connected){
      setText("updatedLabel","LAST UPDATE "+Math.max(0,(now-state.lastStatusAt)/1000).toFixed(1)+" S AGO");
    }

    updateThermalAlarm(now);
    drawForce(now,dt);
    drawTemperature(now,dt);
    requestAnimationFrame(frame);
  }

  window.ThrustLabDebug=()=>({
    connected:state.connected,
    ready:state.ready,
    temperatureOnline:state.tempOnline,
    temperatureC:state.tempC,
    warningC:state.warningC,
    dangerC:state.dangerC,
    statusResponses:state.statusCount,
    forceSequence:state.forceSeq,
    temperatureSequence:state.tempSeq,
    lastStatusAgeMs:Math.round(performance.now()-state.lastStatusAt),
    error:state.error
  });

  renderControls();
  statusLoop();
  forceLoop();
  temperatureLoop();
  requestAnimationFrame(frame);
})();
</script>
</body>
</html>
)HTML";
