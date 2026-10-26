# FloodGuard offline exhibition setup

FloodGuard is a self-contained ESP32 demonstration. The ESP32 creates its own
Wi-Fi access point, serves the dashboard from its internal flash, and sends
telemetry directly to the browser over a native WebSocket. It does not need a
router, internet access, cloud service, Mosquitto, MQTT, or an external
JavaScript library.

## Network and dashboard

The firmware starts:

| Service | Address |
| --- | --- |
| Wi-Fi network | `FloodGuard` |
| Wi-Fi password | `Flood123` |
| Expected ESP32 address | `192.168.4.1` |
| Dashboard HTTP page | `http://192.168.4.1/` |
| WebSocket telemetry | `ws://192.168.4.1/ws` |

The actual access-point address is printed with `WiFi.softAPIP()` at startup.
The browser dashboard derives its WebSocket URL from the page host, so it
automatically connects to the ESP32 when opened from its HTTP address. No
laptop IP address is hardcoded.

The complete dashboard is embedded in
[`local_servor.ino`](./local_servor/local_servor.ino) as `index_html`. Its CSS
and JavaScript are inline, and there are no CDN, MQTT.js, cloud, or other
internet dependencies.

## Arduino libraries

Install these libraries through the Arduino IDE Library Manager:

- `AsyncTCP`
- `ESPAsyncWebServer`
- `ArduinoJson`
- `ESP32Servo`

Select the appropriate ESP32 board, normally **ESP32 Dev Module**. The sketch
uses `115200` baud for the Serial Monitor.

## Hardware configuration

The existing hardware pins and processing logic are unchanged:

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

The current demonstration thresholds are:

- `CAUTION_THRESHOLD_PERCENT = 40`
- `DANGER_THRESHOLD_PERCENT = 80`

The current calibration endpoints are:

- Dry reading: `4095`
- Wet reading: `1500`

Verify calibration with real dry and wet sensor readings before relying on the
displayed percentages. Soil moisture alone is not a reliable prediction of a
real flood or landslide. Keep every sensor output within the ESP32 input
voltage limit.

Power both servos from a suitable external 5 V supply, not the ESP32 3.3 V
output, and connect the external supply ground to ESP32 ground. Keep barriers
clear during testing.

## Exhibition operator procedure

1. Power on the ESP32.
2. Open Serial Monitor at **115200 baud**.
3. Confirm messages similar to:

   ```text
   FloodGuard access point started
   Network name: FloodGuard
   Access point IP address: 192.168.4.1
   HTTP dashboard server started
   Dashboard URL: http://192.168.4.1/
   WebSocket endpoint: ws://192.168.4.1/ws
   ```

   The IP is printed from the actual `WiFi.softAPIP()` result and may be
   different on unusual board configurations.

4. Connect the laptop to the Wi-Fi network **FloodGuard** using the password
   **Flood123**.
5. If Windows warns that the network has no internet, remain connected. This
   is expected because FloodGuard is an offline local network.
6. Open `http://192.168.4.1` in the browser.
7. Confirm the dashboard connection indicator changes to **Connected**.
8. Confirm the Serial Monitor reports a WebSocket client connection.
9. Test each of the three moisture sensors and verify that raw readings,
   percentages, chart points, and risk indicators update.
10. Confirm the matching LED changes for `NORMAL`, `CAUTION`, and `RED ZONE`.
    Test servo movement with linkages safely clear.
11. Switch to demo mode to verify the existing demo controls, then return to
    live mode and confirm live WebSocket telemetry resumes.

## Short fault checklist

- **No Wi-Fi network:** confirm the ESP32 is powered and look for
  `FloodGuard access point started` in Serial Monitor.
- **Page does not open:** reconnect the laptop to `FloodGuard`, keep the
  no-internet connection, and use `http://192.168.4.1/`.
- **Connected page but no live data:** check for a WebSocket client connection
  message and refresh the page.
- **Stale values:** verify the ESP32 remains powered and the browser remains
  connected to the FloodGuard network.
- **Unexpected percentages:** verify each sensor's dry/wet calibration and
  confirm sensor voltage safety.

This project has not been physically tested in this environment. The checklist
above is the procedure for validating the hardware at the exhibition.
