# LoadCell_EA60

ESP32 + HX711 static thrust test bench for the EA60 motor.

## Hardware wiring

| HX711 | ESP32 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| DOUT / DT | GPIO 19 |
| SCK / CLK | GPIO 18 |

The load cell connects to the HX711 using its E+/E-/A+/A- connections. Wire colors vary by manufacturer, so verify your load cell datasheet.

## PlatformIO

Open the repository folder in VS Code with PlatformIO and upload the `esp32dev` environment.

The ESP32 creates its own Wi-Fi network:

- SSID: `ThrustBench-ESP32`
- Password: `ThrustTest123`
- Dashboard: `http://192.168.4.1`

No internet connection is required.

## Dashboard

The dashboard shows:

- grams-force (gf)
- kilograms-force (kgf)
- pounds-force (lbf)
- newtons (N)
- peak force
- raw HX711 counts
- HX711 sample rate
- live 30-second force graph

The numeric display and chart are visually interpolated for smoother motion. Recorded data remains unsmoothed.

The dashboard polls `/api/status` independently of `/api/samples`. This separation prevents graph or recording traffic from freezing the live force values.

## Calibration

1. Power the ESP32 and HX711 with the motor off.
2. Open the dashboard.
3. With no force applied, press **TARE**.
4. Apply a known reference load.
5. Enter that load in grams-force and press **CALIBRATE**.
6. Remove the reference load.
7. Press **TARE** again.
8. Verify the displayed force using another known load before running the motor.

Example: a 500 g mass under standard gravity corresponds to approximately 500 gf, 0.500 kgf, 1.102 lbf, and 4.903 N.

## PC-side CSV recording

The ESP32 does not save test CSV files to flash.

Connect the computer to `ThrustBench-ESP32`, then from the repository folder run:

```powershell
py -3 pc_logger.py
```

The dashboard should change to **PC LOGGER CONNECTED**.

Press **START RECORDING** on the dashboard. CSV files are written directly to:

```
recordings/
```

Each row contains:

- computer timestamp
- elapsed time
- ESP32 uptime
- raw ADC counts
- gf
- kgf
- lbf
- N
- sample quality / gap information

## API endpoints

- `GET /api/status` — lightweight live dashboard state
- `GET /api/samples?after=<sequence>&limit=96` — buffered sample stream
- `GET /api/logger/ping` — PC logger heartbeat
- `POST /api/tare`
- `POST /api/calibrate?grams=500`
- `POST /api/resetpeak`
- `POST /api/start`
- `POST /api/stop`

## Troubleshooting live telemetry

Open:

```
http://192.168.4.1/api/status
```

Refresh it several times. Both `uptimeMs` and `latestReadMs` should change.

If `uptimeMs` changes but `latestReadMs` does not, inspect HX711 power, DOUT, SCK, and load-cell wiring.

If the API changes but the dashboard appears stale, force-refresh the browser with **Ctrl+Shift+R**. The dashboard itself uses cache-busting requests and automatically retries polling after request failures.

In the browser console, run:

```javascript
ThrustLabDebug()
```

to inspect telemetry polling state.

## Safety

Secure the motor, propeller, load cell, and test stand before operation. Use a protective barrier and keep people out of the propeller plane. Perform load-cell calibration with the motor/ESC disabled.
