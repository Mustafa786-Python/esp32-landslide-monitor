# FloodGuard dashboard

`copilor.html` is a standalone responsive dashboard for the ESP32 FloodGuard prototype.

## Run

Open `copilor.html` directly in a browser for the presentation-ready demo mode. No build step or package installation is required.

- **Demo mode** uses clearly labeled simulated readings. Use **Simulate rising moisture** to walk through NORMAL → WARNING → HIGH RISK; the demo commands both barriers closed at the danger threshold and resets after the demonstration.
- **Live mode** is opt-in. Select **Settings** in the sidebar, enter the ESP32 WebSocket URL, save, and then select **Live mode**. The interface displays real connection errors/disconnects and never labels an unconnected device as live.

## ESP32 integration

The frontend expects either of these WebSocket JSON shapes:

```json
{"sensors":[{"raw":1780,"pct":22},{"raw":1920,"pct":27},{"raw":1680,"pct":25}]}
```

or the compact prototype shape:

```json
{"s1":22,"s2":27,"s3":25,"avg":24.7,"state":"NORMAL","gates":"OPEN"}
```

For the full sensor shape, `raw` is the ADC reading and `pct` is the calibrated moisture percentage. The dashboard maps the prototype inputs as follows:

- Moisture ADC: GPIO 32, GPIO 33, GPIO 34
- LEDs: green GPIO 25, yellow GPIO 26, red GPIO 27
- Barriers: Servo 1 GPIO 13, Servo 2 GPIO 14

Barrier state received from the device should be included in a production payload and should distinguish a commanded position from a confirmed position. Manual barrier controls are intentionally not exposed by this frontend because a display-only override must never masquerade as a real ESP32 command.

## Calibration and thresholds

Calibrate each probe in the ESP32 firmware using measured dry and wet ADC endpoints, then map the ADC value to 0–100% before sending `pct`. Do not send an ADC value as a percentage. The dashboard defaults to warning at **45%** and danger at **70%**; both thresholds can be changed in Settings and are applied to the risk explanation and chart guides.
