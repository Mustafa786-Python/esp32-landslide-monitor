# Landslide Detector

The Landslide Detector reads three soil-moisture sensors, calculates a risk
state, and controls two safety gates and three status LEDs. The ESP32 hosts the
dashboard locally, so the system works offline.

## How it works

The ESP32 starts its own Wi-Fi access point, serves the dashboard from flash,
and sends sensor data to the browser over a WebSocket. The firmware runs the
sensor and risk update once every second.

The dashboard HTML is embedded in the sketch. The old internal raw-string
delimiter `FLOODGUARD_HTML` remains in the code.

## Network and dashboard

| Setting | Value |
| --- | --- |
| Wi-Fi name | `Landsliding_detector` |
| Wi-Fi password | `slide123` |
| Dashboard page | `http://192.168.4.1/` |
| WebSocket | `ws://192.168.4.1/ws` |

The dashboard works without internet access. The page loads the JetBrains Mono
font from Google Fonts; without internet, the browser uses its default
fallback font.

## Hardware

| Component | GPIO |
| --- | ---: |
| Moisture sensor 1 | 32 |
| Moisture sensor 2 | 34 |
| Moisture sensor 3 | 35 |
| Left servo signal | 12 |
| Right servo signal | 13 |
| Green LED | 25 |
| Yellow LED | 26 |
| Red LED | 27 |

## Risk logic

Danger is checked before caution.

| State | Exact rule | LED | Gate action |
| --- | --- | --- | --- |
| `NORMAL` | Average is `<= 60%` and every sensor is `< 60%` | Green | Open |
| `CAUTION` | Average is `> 60%`, or any sensor is `>= 60%`, while no danger rule is true | Yellow | Open |
| `RED ZONE` | Average is `>= 80%`, or any sensor is `>= 80%` | Red | Closed |

## Calibration

The firmware uses these calibration values:

- `DRY_SENSOR_READING = 4095`
- `WET_SENSOR_READING = 1500`

`moisturePercentFromRaw()` maps a raw reading from the dry value to the wet
value as `0` to `100` percent, then limits the result to that range. Recalibrate
these endpoints with your own sensors and their measured dry and wet readings.

## Telemetry format

The firmware broadcasts one JSON message every `1000` milliseconds when a
WebSocket client is connected:

```json
{
  "sensor1_raw": 2512,
  "sensor2_raw": 2382,
  "sensor3_raw": 2201,
  "sensor1_percent": 61,
  "sensor2_percent": 66,
  "sensor3_percent": 73,
  "average_moisture": 66.6667,
  "highest_moisture": 73,
  "risk_state": "CAUTION",
  "green_led": false,
  "yellow_led": true,
  "red_led": false,
  "left_gate": "OPEN",
  "right_gate": "OPEN"
}
```

## Servo gates

| Gate state | Servo angle |
| --- | ---: |
| Open | 90 degrees |
| Closed | 0 degrees |

Use an external 5 V supply for the servos. Share the external supply ground
with the ESP32 ground.

## Dashboard features

- **Live mode** receives ESP32 WebSocket telemetry.
- **Demo mode** uses simulated values and does not use ESP32 data. It includes
  start, pause, reset, scenario buttons, and sliders for all three sensors.
- The live moisture history chart shows Sensor 1, Sensor 2, and Sensor 3. It
  supports 1-minute, 5-minute, and 15-minute ranges and shows the 60% caution
  reference.
- The events list records risk-state and gate changes.
- A stale-data warning appears when recent telemetry is not available. It is
  hidden in Demo mode.

## Arduino libraries

Install these libraries through the Arduino IDE Library Manager:

- `AsyncTCP`
- `ESPAsyncWebServer`
- `ArduinoJson`
- `ESP32Servo`

The sketch also includes the standard `WiFi` and `string` headers.

## Setup and run

1. Open `Main_code/local_servor/local_servor.ino`.
2. Select **ESP32 Dev Module** as the board.
3. Install the libraries listed above.
4. Upload the sketch.
5. Open Serial Monitor at `115200` baud.
6. Connect to `Landsliding_detector` with password `slide123`.
7. Open `http://192.168.4.1/` in a browser.

The normal startup output is:

```text
Landslide Detector access point started
Network name: Landsliding_detector
Access point IP address: 192.168.4.1
HTTP dashboard server started
Dashboard URL: http://192.168.4.1/
WebSocket endpoint: ws://192.168.4.1/ws
```

The access-point IP line is printed from `WiFi.softAPIP()`. If startup fails,
the firmware prints:

```text
ERROR: Landslide Detector access point failed to start
```

## Troubleshooting

- **No Wi-Fi:** Check power and look for `Landslide Detector access point started`.
- **Page does not open:** Reconnect to `Landsliding_detector` and open the exact
  dashboard URL.
- **No live data:** Check the browser connection and WebSocket endpoint.
- **Stale values:** Check ESP32 power, Wi-Fi connection, and the WebSocket
  connection.
- **Wrong percentages:** Check the dry and wet calibration readings.
- **Servos do not move:** Check the external 5 V supply and shared ground.

## Limits

Soil moisture alone does not predict a real landslide. This is a demonstration
model. Keep sensor voltage within the ESP32 limit.

## License and author

MIT License

Copyright (c) 2026 Mustafa Ali Danish

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.