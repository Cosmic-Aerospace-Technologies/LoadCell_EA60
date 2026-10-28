# LoadCell_EA60

ESP32 + HX711 static thrust and endurance test bench for brushless motors.

## Hardware wiring

### HX711 load cell amplifier

| HX711 | ESP32 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| DOUT / DT | GPIO 19 |
| SCK / CLK | GPIO 18 |

The load cell connects to the HX711 using its E+/E-/A+/A- connections. Wire colors vary by manufacturer, so verify the load cell datasheet.

### DS18B20 temperature probe

This project assumes the common waterproof DS18B20-style probe with red, black, and yellow leads plus a separate pull-up resistor.

| Temperature probe | ESP32 |
| --- | --- |
| Red | 3V3 |
| Black | GND |
| Yellow / data | GPIO 4 |

Install a **4.7 kΩ pull-up resistor between Yellow/Data and 3V3**.

If your sensor is not a DS18B20, do not wire it from this table until its exact part number is confirmed.

Attach the temperature probe firmly to the motor exterior with good thermal contact. Kapton tape, high-temperature tape, or a suitable thermally conductive attachment method is preferable to a loose air-gap contact. Keep the probe and wiring clear of the rotating bell and propeller.

## PlatformIO

Open the repository folder in VS Code with PlatformIO and upload the `esp32dev` environment.

The ESP32 creates its own Wi-Fi network:

- SSID: `ThrustBench-ESP32`
- Password: `ThrustTest123`
- Dashboard: `http://192.168.4.1`

No internet connection is required during testing.

## Dashboard

The dashboard shows:

- grams-force (gf)
- kilograms-force (kgf)
- pounds-force (lbf)
- newtons (N)
- peak force
- raw HX711 counts
- HX711 sample rate
- motor surface temperature in °C and °F
- minimum and maximum temperature for the current run
- 30-second force history
- 30-minute temperature history
- endurance-test elapsed time
- PC logger status
- thermal warning/danger state

The force display and graphs are visually interpolated for smoother motion. CSV values remain unsmoothed.

### Temperature alarm

The default thresholds are:

- Warning: **60 °C**
- Danger: **70 °C**

These are conservative generic surface-temperature defaults for endurance testing, not universal motor limits. Change them in the dashboard if the exact motor manufacturer provides a validated thermal limit.

The dashboard changes temperature color continuously as the motor heats up. At warning temperature it turns amber. At danger temperature the motor temperature panel and top banner pulse red.

Click **ENABLE ALARM SOUND** once after opening the page if you want browser beeps. Browsers require a user interaction before allowing audio.

## Load-cell calibration

1. Install the motor/load-cell assembly in its normal test orientation.
2. With the motor off and no reference force applied, press **TARE**.
3. Apply a known force along the same axis as motor thrust.
4. Enter that force in grams-force and press **CALIBRATE**.
5. Remove the reference force.
6. Press **TARE** again.
7. Reapply the known force and verify the displayed result before running the motor.

Example: a 207 g hanging mass produces approximately 207 gf, 0.207 kgf, 0.456 lbf, and 2.03 N when the line tension acts along the calibrated load axis.

## PC-side CSV recording

The ESP32 does not save test CSV files to flash.

Connect the computer to `ThrustBench-ESP32`, then from the repository folder run:

```powershell
py -3 pc_logger.py
```

The dashboard should change to **PC LOGGER CONNECTED**.

Press **START ENDURANCE TEST** on the dashboard. CSV files are written directly to:

```
recordings/
```

Each row contains:

- computer timestamp
- elapsed test time
- ESP32 uptime
- raw ADC counts
- gf
- kgf
- lbf
- N
- motor temperature °C
- motor temperature °F
- sample quality / gap information

## API endpoints

- `GET /api/status` — lightweight live dashboard state
- `GET /api/samples?after=<sequence>&limit=96` — buffered force + temperature samples
- `GET /api/temperature?after=<sequence>&limit=120` — temperature-history stream
- `GET /api/logger/ping` — PC logger heartbeat
- `POST /api/tare`
- `POST /api/calibrate?grams=207`
- `POST /api/resetpeak`
- `POST /api/temperature/thresholds?warning=60&danger=70`
- `POST /api/start`
- `POST /api/stop`

## Troubleshooting

### Load cell

Open:

```
http://192.168.4.1/api/status
```

Refresh it several times. `uptimeMs` and `latestReadMs` should change.

If `uptimeMs` changes but `latestReadMs` does not, inspect HX711 power, DOUT, SCK, and load-cell wiring.

### Temperature probe

In the same status response:

- `tempOnline:true` means the DS18B20 is responding.
- `tempC` is the current temperature.
- `latestTempMs` should update approximately once per second.

If the probe remains offline, verify:

- Red -> 3V3
- Black -> GND
- Yellow -> GPIO 4
- 4.7 kΩ resistor from Yellow/Data -> 3V3

### Browser telemetry

If the API changes but the dashboard appears stale, force-refresh with **Ctrl+Shift+R**.

In the browser console, run:

```javascript
ThrustLabDebug()
```

to inspect telemetry polling state.

## Safety

Secure the motor, propeller, load cell, temperature probe, and test stand before operation. Use a protective barrier and keep people out of the propeller plane.

A surface-mounted temperature probe does **not** directly measure winding temperature. Internal windings and magnets can be hotter than the motor exterior, so use conservative thresholds unless the motor manufacturer provides validated limits.
