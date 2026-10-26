#include <Arduino.h>
#include <ESP32Servo.h>

// FloodGuard offline demonstration firmware.
// Select an ESP32 board such as "ESP32 Dev Module" in the Arduino IDE.

// Moisture sensor analog-output pins. GPIO 34 is the third sensor; GPIO 35 is
// intentionally not used.
constexpr uint8_t MOISTURE_SENSOR_1_PIN = 32;
constexpr uint8_t MOISTURE_SENSOR_2_PIN = 33;
constexpr uint8_t MOISTURE_SENSOR_3_PIN = 34;

constexpr uint8_t LEFT_SERVO_PIN = 13;
constexpr uint8_t RIGHT_SERVO_PIN = 14;

constexpr uint8_t GREEN_LED_PIN = 25;
constexpr uint8_t YELLOW_LED_PIN = 26;
constexpr uint8_t RED_LED_PIN = 27;

// Power the servos from a suitable separate 5 V supply, not from the ESP32's
// 3.3 V pin. Connect the external supply ground to ESP32 GND so servo signals
// have a common reference. Keep every moisture sensor output within the
// ESP32 ADC input safe voltage range (no more than 3.3 V).

// These are starting demonstration thresholds, not flood-prediction values.
// Adjust them after observing the sensors in your own dry and wet soil.
constexpr float WARNING_THRESHOLD_PERCENT = 40.0f;
constexpr float DANGER_THRESHOLD_PERCENT = 70.0f;

// Set each sensor's endpoints after calibration. The formula works whether
// wet soil produces a higher or lower ADC value than dry soil.
struct SensorCalibration {
  uint8_t pin;
  int dryAdc;
  int wetAdc;
};

SensorCalibration sensorCalibration[3] = {
  {MOISTURE_SENSOR_1_PIN, 4095, 1500},
  {MOISTURE_SENSOR_2_PIN, 4095, 1500},
  {MOISTURE_SENSOR_3_PIN, 4095, 1500}
};

constexpr int OPEN_BARRIER_ANGLE = 90;
constexpr int CLOSED_BARRIER_ANGLE = 0;
constexpr unsigned long REPORT_INTERVAL_MS = 1000;
constexpr uint8_t REQUIRED_CONFIRMATIONS = 3;
constexpr float FILTER_ALPHA = 0.35f;

enum class RiskState {
  NORMAL,
  WARNING,
  HIGH_RISK
};

struct SensorReading {
  int rawAdc;
  float moisturePercent;
  bool valid;
};

Servo leftBarrier;
Servo rightBarrier;
RiskState currentRiskState = RiskState::NORMAL;
RiskState candidateRiskState = RiskState::NORMAL;
uint8_t candidateCount = 0;
unsigned long lastReportMs = 0;
bool filterInitialized = false;
float filteredMoisture[3] = {0.0f, 0.0f, 0.0f};
SensorReading readings[3];

const char* riskStateName(RiskState state) {
  switch (state) {
    case RiskState::WARNING:
      return "WARNING";
    case RiskState::HIGH_RISK:
      return "HIGH RISK";
    case RiskState::NORMAL:
    default:
      return "NORMAL";
  }
}

bool moisturePercentFromAdc(int rawAdc, const SensorCalibration& calibration,
                            float& moisturePercent) {
  if (calibration.dryAdc < 0 || calibration.dryAdc > 4095 ||
      calibration.wetAdc < 0 || calibration.wetAdc > 4095 ||
      calibration.dryAdc == calibration.wetAdc) {
    return false;
  }

  // This interpolation automatically handles either sensor direction.
  const float percentage =
      (static_cast<float>(rawAdc - calibration.dryAdc) * 100.0f) /
      static_cast<float>(calibration.wetAdc - calibration.dryAdc);
  moisturePercent = constrain(percentage, 0.0f, 100.0f);
  return true;
}

const char* sensorRangeName(float moisturePercent) {
  if (moisturePercent >= DANGER_THRESHOLD_PERCENT) {
    return "DANGER";
  }
  if (moisturePercent >= WARNING_THRESHOLD_PERCENT) {
    return "WARNING";
  }
  return "NORMAL";
}

RiskState calculateRisk(float averageMoisture, uint8_t validSensorCount,
                        uint8_t warningOrDangerCount, uint8_t dangerCount) {
  if (validSensorCount == 0) {
    // No usable measurement is not considered a safe normal condition.
    return RiskState::HIGH_RISK;
  }

  // High risk needs two independent sensors at the danger threshold.
  if (dangerCount >= 2) {
    return RiskState::HIGH_RISK;
  }

  // A single elevated sensor, or a wet average, is enough for warning.
  if (warningOrDangerCount > 0 ||
      averageMoisture >= WARNING_THRESHOLD_PERCENT) {
    return RiskState::WARNING;
  }
  return RiskState::NORMAL;
}

void setRiskOutputs(RiskState state) {
  const bool normal = state == RiskState::NORMAL;
  const bool warning = state == RiskState::WARNING;
  const bool highRisk = state == RiskState::HIGH_RISK;

  digitalWrite(GREEN_LED_PIN, normal ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, warning ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, highRisk ? HIGH : LOW);

  const int targetAngle = normal ? OPEN_BARRIER_ANGLE : CLOSED_BARRIER_ANGLE;
  static int lastTargetAngle = -1;
  if (targetAngle != lastTargetAngle) {
    leftBarrier.write(targetAngle);
    rightBarrier.write(targetAngle);
    lastTargetAngle = targetAngle;
  }
}

void updateRiskState(RiskState measuredState) {
  if (measuredState != candidateRiskState) {
    candidateRiskState = measuredState;
    candidateCount = 1;
  } else if (candidateCount < REQUIRED_CONFIRMATIONS) {
    ++candidateCount;
  }

  if (candidateCount >= REQUIRED_CONFIRMATIONS &&
      currentRiskState != candidateRiskState) {
    currentRiskState = candidateRiskState;
    setRiskOutputs(currentRiskState);
  }
}

void printOnOff(bool on) {
  Serial.print(on ? "ON" : "OFF");
}

void printStatus(float averageMoisture, float highestMoisture,
                 uint8_t validSensorCount, uint8_t warningOrDangerCount,
                 uint8_t dangerCount) {
  Serial.println();
  Serial.println("========== FLOODGUARD STATUS ==========");
  Serial.println();

  for (uint8_t i = 0; i < 3; ++i) {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": Raw ADC = ");
    Serial.print(readings[i].rawAdc);
    Serial.print(" | Moisture = ");
    if (readings[i].valid) {
      Serial.print(readings[i].moisturePercent, 1);
      Serial.print("% | Range = ");
      Serial.println(sensorRangeName(readings[i].moisturePercent));
    } else {
      Serial.println("UNAVAILABLE | Range = INVALID CALIBRATION");
    }
  }

  Serial.println();
  if (validSensorCount > 0) {
    Serial.print("Average moisture: ");
    Serial.print(averageMoisture, 1);
    Serial.println("%");
    Serial.print("Highest moisture: ");
    Serial.print(highestMoisture, 1);
    Serial.println("%");
  } else {
    Serial.println("Average moisture: UNAVAILABLE");
    Serial.println("Highest moisture: UNAVAILABLE");
  }
  Serial.print("Sensors in warning/danger range: ");
  Serial.print(warningOrDangerCount);
  Serial.print("/");
  Serial.println(validSensorCount);
  Serial.print("Sensors in danger range: ");
  Serial.print(dangerCount);
  Serial.print("/");
  Serial.println(validSensorCount);

  Serial.println();
  Serial.print("Risk state: ");
  Serial.println(riskStateName(currentRiskState));
  Serial.print("Green LED: ");
  printOnOff(currentRiskState == RiskState::NORMAL);
  Serial.println();
  Serial.print("Yellow LED: ");
  printOnOff(currentRiskState == RiskState::WARNING);
  Serial.println();
  Serial.print("Red LED: ");
  printOnOff(currentRiskState == RiskState::HIGH_RISK);
  Serial.println();

  const int barrierAngle =
      currentRiskState == RiskState::NORMAL ? OPEN_BARRIER_ANGLE
                                             : CLOSED_BARRIER_ANGLE;
  Serial.print("Left barrier: ");
  Serial.print(barrierAngle == OPEN_BARRIER_ANGLE ? "OPEN" : "CLOSED");
  Serial.print(" (");
  Serial.print(barrierAngle);
  Serial.println(" degrees)");
  Serial.print("Right barrier: ");
  Serial.print(barrierAngle == OPEN_BARRIER_ANGLE ? "OPEN" : "CLOSED");
  Serial.print(" (");
  Serial.print(barrierAngle);
  Serial.println(" degrees)");
  Serial.println();
  Serial.println("Wi-Fi: Not required");
  Serial.println("=======================================");
}

void readSensorsAndUpdateRisk() {
  float totalMoisture = 0.0f;
  float highestMoisture = 0.0f;
  uint8_t validSensorCount = 0;
  uint8_t warningOrDangerCount = 0;
  uint8_t dangerCount = 0;

  for (uint8_t i = 0; i < 3; ++i) {
    readings[i].rawAdc = analogRead(sensorCalibration[i].pin);
    float measuredPercent = 0.0f;
    readings[i].valid = moisturePercentFromAdc(
        readings[i].rawAdc, sensorCalibration[i], measuredPercent);

    if (!readings[i].valid) {
      continue;
    }

    if (!filterInitialized) {
      filteredMoisture[i] = measuredPercent;
    } else {
      filteredMoisture[i] =
          FILTER_ALPHA * measuredPercent +
          (1.0f - FILTER_ALPHA) * filteredMoisture[i];
    }
    readings[i].moisturePercent = filteredMoisture[i];
    totalMoisture += readings[i].moisturePercent;
    highestMoisture = max(highestMoisture, readings[i].moisturePercent);
    ++validSensorCount;

    if (readings[i].moisturePercent >= WARNING_THRESHOLD_PERCENT) {
      ++warningOrDangerCount;
    }
    if (readings[i].moisturePercent >= DANGER_THRESHOLD_PERCENT) {
      ++dangerCount;
    }
  }
  filterInitialized = true;

  const float averageMoisture =
      validSensorCount > 0 ? totalMoisture / validSensorCount : 0.0f;
  const RiskState measuredState =
      calculateRisk(averageMoisture, validSensorCount, warningOrDangerCount,
                    dangerCount);
  updateRiskState(measuredState);
  printStatus(averageMoisture, highestMoisture, validSensorCount,
              warningOrDangerCount, dangerCount);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("FloodGuard starting in offline mode.");

  analogReadResolution(12);
  for (uint8_t i = 0; i < 3; ++i) {
    pinMode(sensorCalibration[i].pin, INPUT);
    analogSetPinAttenuation(sensorCalibration[i].pin, ADC_11db);
  }

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  leftBarrier.setPeriodHertz(50);
  rightBarrier.setPeriodHertz(50);
  leftBarrier.attach(LEFT_SERVO_PIN);
  rightBarrier.attach(RIGHT_SERVO_PIN);
  setRiskOutputs(currentRiskState);
}

void loop() {
  const unsigned long now = millis();
  if (now - lastReportMs >= REPORT_INTERVAL_MS) {
    lastReportMs = now;
    readSensorsAndUpdateRisk();
  }
}
