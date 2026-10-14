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
:root{--bg:#080e14;--panel:#101a22;--border:#23343f;--text:#e8f2f5;--muted:#7895a3;--green:#43f4c0;--amber:#f4bc65;--red:#ff6b74}
*{box-sizing:border-box}
body{margin:0;background:radial-gradient(circle at 70% -20%,#20343f 0,transparent 42%),var(--bg);color:var(--text);font-family:Inter,Segoe UI,Arial,sans-serif}
.wrap{max-width:1280px;margin:auto;padding:28px 22px 50px}
.top{display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--border);padding-bottom:20px}
.brand{font-weight:850;letter-spacing:.14em;font-size:20px}.sub{font:10px Consolas,monospace;color:var(--muted);letter-spacing:.13em;margin-top:4px}
.link{font:11px Consolas,monospace;color:var(--muted)}.dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:var(--red);margin-right:7px}.dot.ok{background:var(--green);box-shadow:0 0 12px #43f4c077}
h1{font-size:clamp(28px,4vw,42px);margin:30px 0 4px}.lead{color:var(--muted);margin:0 0 18px}
.grid{display:grid;grid-template-columns:repeat(12,1fr);gap:16px}.card{background:linear-gradient(155deg,#14212a,#0e171f);border:1px solid var(--border);border-radius:12px;padding:20px;min-width:0}.live{grid-column:span 5}.graph{grid-column:span 7}.controls{grid-column:span 5}.logger{grid-column:span 7}
.title{display:flex;justify-content:space-between;gap:12px;color:var(--muted);font:700 11px Consolas,monospace;letter-spacing:.12em;text-transform:uppercase;margin-bottom:18px}
.big{font-size:clamp(56px,7vw,88px);font-weight:750;color:var(--green);letter-spacing:-.07em;font-variant-numeric:tabular-nums}.unit{font:700 14px Consolas,monospace;color:var(--muted);margin-left:10px}.updated{font:10px Consolas,monospace;color:var(--muted);margin-top:6px}
.metrics{display:grid;grid-template-columns:1fr 1fr;gap:20px 12px;margin-top:24px;padding-top:20px;border-top:1px solid var(--border)}.ml{font:10px Consolas,monospace;color:var(--muted);text-transform:uppercase;letter-spacing:.08em}.mv{font:600 24px Consolas,monospace;margin-top:6px}
.chartbox{height:300px}canvas{width:100%;height:100%;display:block}.foot{display:flex;justify-content:space-between;color:var(--muted);font:10px Consolas,monospace;margin-top:10px}
button{padding:12px 15px;border-radius:7px;border:1px solid #39505d;background:#152630;color:var(--text);font:700 11px Consolas,monospace;cursor:pointer}button:hover{border-color:var(--green)}button:disabled{opacity:.35;cursor:not-allowed}.primary{background:var(--green);color:#04140e;border-color:var(--green)}.danger{background:#351c27;color:var(--red);border-color:#87404d}
.actions{display:grid;grid-template-columns:1fr 1fr;gap:9px}.section{border-top:1px solid var(--border);margin-top:17px;padding-top:17px}.row{display:flex;gap:8px}.row input{min-width:0;flex:1;background:#080f15;color:var(--text);border:1px solid #39505d;border-radius:7px;padding:11px;font:14px Consolas,monospace}.label{font:10px Consolas,monospace;color:var(--muted);text-transform:uppercase;letter-spacing:.1em;margin-bottom:9px}.msg{font:11px Consolas,monospace;color:var(--amber);margin-top:14px;min-height:34px;line-height:1.5}.help{color:var(--muted);font-size:12px;line-height:1.65}.pc{font:700 12px Consolas,monospace;color:var(--red)}.pc.on{color:var(--green)}.record{color:var(--muted);font:11px Consolas,monospace}.record.on{color:var(--red)}
@media(max-width:880px){.live,.graph,.controls,.logger{grid-column:span 12}}@media(max-width:520px){.wrap{padding:18px 12px 36px}.card{padding:15px}.metrics{grid-template-columns:1fr 1fr}.chartbox{height:250px}}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div><div class="brand">THRUST // LAB</div><div class="sub">ESP32 · HX711 · STATIC THRUST TELEMETRY</div></div>
    <div class="link"><span id="connectionDot" class="dot"></span><span id="connectionLabel">CONNECTING</span> · <span id="deviceIP"></span></div>
  </div>

  <h1>Static Thrust Test Bench</h1>
  <p class="lead">Standalone ESP32 Wi-Fi · live dashboard · PC-side CSV recording</p>

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
      <div class="chartbox"><canvas id="plot"></canvas></div>
      <div class="foot"><span>FORCE [gf]</span><span>TIME [s] · AUTO SCALE</span></div>
    </section>

    <section class="card controls">
      <div class="title"><span>03 / Test control</span><span id="recordLabel" class="record">STANDBY</span></div>
      <div class="actions">
        <button id="startBtn" class="primary">● START RECORDING</button>
        <button id="stopBtn" class="danger" disabled>■ STOP & SAVE</button>
      </div>

      <div class="section">
        <div class="label">Zero offset</div>
        <button id="tareBtn">TARE</button>
        <button id="peakResetBtn">RESET PEAK</button>
      </div>

      <div class="section">
        <div class="label">Known calibration load [gf]</div>
        <div class="row">
          <input id="calMass" type="number" min="1" max="100000" value="500">
          <button id="calBtn">CALIBRATE</button>
        </div>
      </div>

      <div id="statusMessage" class="msg">SYSTEM / Initializing...</div>
    </section>

    <section class="card logger">
      <div class="title"><span>04 / Wireless PC recording</span><span id="pcStatus" class="pc">PC LOGGER OFFLINE</span></div>
      <p class="help">Connect this computer to <b>ThrustBench-ESP32</b> and run <code>py -3 pc_logger.py</code>. CSV files are written directly into the <code>recordings/</code> folder on the computer.</p>
      <p class="help"><b id="samplesLabel">0 SAMPLES THIS RUN</b></p>
      <p class="help" id="sessionLabel">IDLE / NO ACTIVE TEST</p>
    </section>
  </div>
</div>

<script>
(() => {
  "use strict";

  const $ = id => document.getElementById(id);
  const canvas = $("plot");
  const ctx = canvas.getContext("2d");
  const chartData = [];

  const state = {
    connected:false,
    ready:false,
    recording:false,
    busy:false,
    pcOnline:false,
    targetGF:0,
    shownGF:null,
    peakGF:0,
    boot:null,
    epoch:null,
    chartSeq:0,
    sensorTime:0,
    chartReceivedAt:0,
    lastStatusAt:0,
    lastFrame:performance.now(),
    scaleMin:null,
    scaleMax:null,
    lastMessage:null,
    statusCount:0,
    error:null
  };

  $("deviceIP").textContent = location.host;

  const setText = (id, value) => {
    const el = $(id);
    if (el) el.textContent = value;
  };

  const fmt = (value, digits) =>
    Number.isFinite(value) ? value.toFixed(digits) : "--";

  function showMessage(text, isError=false) {
    setText("statusMessage", (isError ? "ERROR / " : "SYSTEM / ") + text);
    $("statusMessage").style.color = isError ? "var(--red)" : "var(--amber)";
  }

  async function api(path, options={}, timeoutMs=1800) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), timeoutMs);

    try {
      const separator = path.includes("?") ? "&" : "?";
      const url = path + separator + "_=" + Date.now();

      const response = await fetch(url, {
        ...options,
        cache:"no-store",
        signal:controller.signal,
        headers:{
          "Cache-Control":"no-cache",
          ...(options.headers || {})
        }
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

  function resetChart() {
    chartData.length = 0;
    state.chartSeq = 0;
    state.sensorTime = 0;
    state.scaleMin = null;
    state.scaleMax = null;
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

    if (state.boot !== d.boot || state.epoch !== d.epoch) {
      state.boot = d.boot;
      state.epoch = d.epoch;
      state.shownGF = null;
      resetChart();
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

    setText("sps", fmt(Number(d.sps), 1));
    setText("raw", d.sensor ? Number(d.raw).toLocaleString() : "--");
    setText("peak", state.ready ? fmt(state.peakGF, 1) : "--");
    setText("samplesLabel", Number(d.count).toLocaleString() + " SAMPLES THIS RUN");

    const elapsed =
      d.recording ? Math.max(0, Number(d.uptimeMs) - Number(d.runStartMs)) : 0;

    setText(
      "sessionLabel",
      d.recording
        ? "TEST LIVE / " + (elapsed / 1000).toFixed(1) + " s"
        : "IDLE / NO ACTIVE TEST"
    );

    $("pcStatus").textContent =
      state.pcOnline ? "PC LOGGER CONNECTED" : "PC LOGGER OFFLINE";

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
      // Critical: always restart polling, including after any fetch/DOM error.
      setTimeout(statusLoop, 250);
    }
  }

  async function chartLoop() {
    try {
      if (state.connected && state.ready) {
        const d = await api(
          "/api/samples?after=" + state.chartSeq + "&limit=96",
          {},
          2200
        );

        if (d.boot !== state.boot || d.epoch !== state.epoch) {
          resetChart();
        } else {
          if (
            state.chartSeq > 0 &&
            Number(d.oldestSeq) > state.chartSeq + 1
          ) {
            state.chartSeq = Number(d.oldestSeq) - 1;
          }

          const points = Array.isArray(d.points) ? d.points : [];

          for (const p of points) {
            const id = Number(p[0]);
            const t = Number(p[1]);
            const gf = Number(p[2]);

            if (id <= state.chartSeq) continue;

            chartData.push([t, gf]);
            state.chartSeq = id;
            state.sensorTime = t;
            state.chartReceivedAt = performance.now();
          }

          if (chartData.length) {
            const cutoff = chartData[chartData.length - 1][0] - 32000;
            while (chartData.length && chartData[0][0] < cutoff) {
              chartData.shift();
            }
          }
        }
      }
    } catch (_) {
      // Chart failure must never stop live numeric telemetry.
    } finally {
      setTimeout(chartLoop, 150);
    }
  }

  function resizeChart() {
    const rect = canvas.getBoundingClientRect();
    const dpr = Math.min(window.devicePixelRatio || 1, 2);

    canvas.width = Math.round(rect.width * dpr);
    canvas.height = Math.round(rect.height * dpr);
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  }

  addEventListener("resize", resizeChart);
  resizeChart();

  function drawChart(now, dt) {
    const w = canvas.clientWidth;
    const h = canvas.clientHeight;

    if (w < 50 || h < 50) return;

    ctx.clearRect(0, 0, w, h);

    const left = 54, right = 12, top = 16, bottom = 24;
    const pw = w - left - right;
    const ph = h - top - bottom;

    const end =
      state.sensorTime
        ? state.sensorTime + Math.min(250, now - state.chartReceivedAt)
        : 30000;

    const begin = end - 30000;

    const visible = chartData.filter(p => p[0] >= begin - 250);

    let low = -20;
    let high = 20;

    for (const p of visible) {
      low = Math.min(low, p[1]);
      high = Math.max(high, p[1]);
    }

    const pad = Math.max(20, high - low) * 0.12;
    low -= pad;
    high += pad;

    const scaleAlpha = 1 - Math.exp(-dt / 260);

    if (state.scaleMin == null) {
      state.scaleMin = low;
      state.scaleMax = high;
    } else {
      state.scaleMin =
        low < state.scaleMin
          ? low
          : state.scaleMin + (low - state.scaleMin) * scaleAlpha;

      state.scaleMax =
        high > state.scaleMax
          ? high
          : state.scaleMax + (high - state.scaleMax) * scaleAlpha;
    }

    const x = t => left + ((t - begin) / 30000) * pw;
    const y = v =>
      top +
      ((state.scaleMax - v) / (state.scaleMax - state.scaleMin)) * ph;

    ctx.font = "10px Consolas,monospace";
    ctx.lineWidth = 1;

    for (let i = 0; i <= 4; i++) {
      const yy = top + (i * ph) / 4;

      ctx.strokeStyle = "#29404b";
      ctx.beginPath();
      ctx.moveTo(left, yy);
      ctx.lineTo(w - right, yy);
      ctx.stroke();

      ctx.fillStyle = "#7895a3";
      ctx.textAlign = "right";

      const v =
        state.scaleMax -
        (i * (state.scaleMax - state.scaleMin)) / 4;

      ctx.fillText(v.toFixed(0), left - 7, yy + 3);
    }

    for (let i = 0; i <= 5; i++) {
      const xx = left + (i * pw) / 5;

      ctx.strokeStyle = "#21343e";
      ctx.beginPath();
      ctx.moveTo(xx, top);
      ctx.lineTo(xx, h - bottom);
      ctx.stroke();

      ctx.fillStyle = "#7895a3";
      ctx.textAlign = "center";
      ctx.fillText(String(-30 + i * 6), xx, h - 5);
    }

    if (visible.length < 2) return;

    ctx.save();
    ctx.beginPath();
    ctx.rect(left, top, pw, ph);
    ctx.clip();

    ctx.strokeStyle = "#43f4c0";
    ctx.lineWidth = 2;
    ctx.lineJoin = "round";
    ctx.lineCap = "round";

    ctx.beginPath();
    ctx.moveTo(x(visible[0][0]), y(visible[0][1]));

    for (let i = 1; i < visible.length; i++) {
      const a = visible[i - 1];
      const b = visible[i];

      ctx.quadraticCurveTo(
        x(a[0]),
        y(a[1]),
        (x(a[0]) + x(b[0])) / 2,
        (y(a[1]) + y(b[1])) / 2
      );
    }

    const last = visible[visible.length - 1];
    ctx.lineTo(x(last[0]), y(last[1]));
    ctx.stroke();

    ctx.fillStyle = "#43f4c0";
    ctx.beginPath();
    ctx.arc(x(last[0]), y(last[1]), 3, 0, Math.PI * 2);
    ctx.fill();

    ctx.restore();
  }

  function frame(now) {
    const dt = Math.min(100, now - state.lastFrame);
    state.lastFrame = now;

    if (
      state.connected &&
      state.ready &&
      Number.isFinite(state.targetGF)
    ) {
      if (state.shownGF == null) {
        state.shownGF = state.targetGF;
      }

      // Display smoothing only. Recorded CSV values stay unfiltered.
      const alpha = 1 - Math.exp(-dt / 170);
      state.shownGF += (state.targetGF - state.shownGF) * alpha;

      setText("thrust", fmt(state.shownGF, 1));
      setText("kgf", fmt(state.shownGF / 1000, 4));
      setText("lbf", fmt(state.shownGF / 453.59237, 4));
      setText("newtons", fmt(state.shownGF * 0.00980665, 3));
    }

    if (
      state.connected &&
      now - state.lastStatusAt > 2300
    ) {
      setOffline("No fresh telemetry received from ESP32.");
    }

    if (state.connected) {
      setText(
        "updatedLabel",
        "LAST UPDATE " +
        Math.max(0, (now - state.lastStatusAt) / 1000).toFixed(1) +
        " S AGO"
      );
    }

    drawChart(now, dt);
    requestAnimationFrame(frame);
  }

  window.ThrustLabDebug = () => ({
    connected:state.connected,
    ready:state.ready,
    statusResponses:state.statusCount,
    chartSequence:state.chartSeq,
    lastStatusAgeMs:Math.round(performance.now() - state.lastStatusAt),
    error:state.error
  });

  renderControls();
  statusLoop();
  chartLoop();
  requestAnimationFrame(frame);
})();
</script>
</body>
</html>
)HTML";
