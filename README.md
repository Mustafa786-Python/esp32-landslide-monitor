# FloodGuard ESP32 Monitor

FloodGuard is an ESP32 moisture-monitoring dashboard for three calibrated moisture
sensors and two servo-controlled safety gates. The firmware broadcasts telemetry over
the `/ws` WebSocket endpoint.

## Run on the ESP32

1. Install the Arduino libraries `AsyncTCP`, `ESPAsyncWebServer`, `ArduinoJson`, and
   `ESP32Servo`.
2. Set `WIFI_SSID` and `WIFI_PASSWORD` in [`Code/Code.ino`](./Code/Code.ino).
3. Flash the sketch to an ESP32 and note the local IP printed by the serial monitor.
4. Open that IP in a browser. The dashboard is embedded in the firmware and connects
   to `ws://<device-ip>/ws` automatically.

The standalone dashboard can be opened directly from [`Dashboard.html`](./Dashboard.html).
Use the settings button to enter the ESP32 IP address or a complete WebSocket URL;
the value is stored in browser local storage. The standalone page does not fabricate
readings: the chart and event history remain empty until valid messages arrive.

## Interactive demo mode

Use the header mode selector to switch to **DEMO MODE** for a hardware-free
presentation. Demo Mode is explicitly marked as a simulation and never reports
simulated values as ESP32 telemetry. Use **START DEMO**, **PAUSE DEMO**, and
**RESET DEMO** to control the 700 ms simulation timer. The scenario buttons show
rising moisture, localized wetness, danger, and recovery; the three sliders allow
manual sensor values from 0% to 100%. Demo state and gate decisions use the same
firmware rules as the live dashboard:

- average `> 70`: `RED ZONE - LOCKDOWN`, gates `CLOSED`
- average `> 40` or any sensor `>= 40`: `CAUTION`, gates `CLOSED`
- otherwise: `NORMAL`, gates `OPEN`

Switching back to **LIVE MODE** stops the simulation, closes its demo data path,
and displays only messages received from the configured ESP32 WebSocket.

## Standalone frontend workflow

1. Open `Dashboard.html` in a browser on the same local network as the ESP32.
2. Select the settings button and enter the ESP32 address, for example `192.168.4.1`.
3. The dashboard normalizes that value to `ws://192.168.4.1/ws`, reconnects after
   connection loss, and marks telemetry stale after 2.5 seconds without a valid
   message.

The firmware currently embeds its own lightweight dashboard in `index_html`. The
standalone page is intentionally separate so the firmware and sensor decision logic
remain unchanged. To serve this polished page from the ESP32, replace only the
contents of the firmware's `index_html` raw string with the browser-ready contents
of `Dashboard.html` (keeping the existing `/ws` handler and JSON broadcast code).
For local development or larger future builds, it can also be served as a static
file from any computer on the same LAN; no backend or cloud service is required.

## Telemetry contract

The firmware sends calibrated moisture percentages, not raw ADC values:

```json
{
  "s1": 45,
  "s2": 52,
  "s3": 60,
  "avg": 52.333,
  "state": "CAUTION",
  "gates": "OPEN"
}
```

`state` and `gates` are the source of truth for the dashboard. The gate visuals
represent commanded servo positions; the current hardware has no position-feedback
sensors.

Hardware mapping:

| Component | GPIO |
| --- | ---: |
| Moisture sensors 1 / 2 / 3 | 32 / 34 / 35 |
| Left / right servos | 12 / 13 |
| Green / yellow / red LEDs | 25 / 26 / 27 |