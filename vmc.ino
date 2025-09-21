#include <Adafruit_SHT4x.h>
#include <Wire.h>
#include <Preferences.h>  // For NVS

#include "HumidityStore.h"

// General constants
#define LOOP_DURATION_MS 60e3                                   // 1 minute
#define HUMIDITY_REMEDIATION_DURATION_MS 10 * LOOP_DURATION_MS  // 10 mminutes
#define RH_STORE_BLOCK_SIZE 1440                                // A full day

// Speed at which logs will be sent to the serial console
#define SERIAL_BAUDS 115200

// I2C bus for temp & humidity sensor
#define SHT4_WIRE Wire1

// Fan control
#define FAN_PWM_PIN A2
#define FAN_PWM_FREQ_HZ 25000
#define FAN_PWM_RESOLUTION_BITS 8
#define FAN_STOPPED 0
#define FAN_MID_SPEED 127
#define FAN_HIGH_SPEED 178
#define FAN_MAX_SPEED 255

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
HumidityStore rh_store;
uint8_t rh_buffer[RH_STORE_BLOCK_SIZE];  // Relative humidity measures to be persisted
uint8_t loop_exec_count = 0;
uint32_t humidity_remediation_remain_ms = 0;

void loop() {
  sensors_event_t humidity, temp;

  sht4.getEvent(&humidity, &temp);
  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println("% rH");
  Serial.flush();
  rh_buffer[loop_exec_count] = rh_store.rh_encode(humidity.relative_humidity);
  loop_exec_count++;
  if (loop_exec_count >= RH_STORE_BLOCK_SIZE) {
    // The rh_buffer is full, flush it to storage
    rh_store.write(rh_buffer);
    loop_exec_count = 0;
  }

  // Fan control
  if (humidity_remediation_remain_ms > 0 || humidity.relative_humidity > 55.0) {
    if (humidity.relative_humidity > 60.0) {
      ledcWrite(FAN_PWM_PIN, FAN_HIGH_SPEED);
    } else {
      ledcWrite(FAN_PWM_PIN, FAN_MID_SPEED);
    }
    // We keep the fan running for HUMIDITY_REMEDIATION_DURATION_MS
    if (humidity_remediation_remain_ms <= 0) {
      humidity_remediation_remain_ms = HUMIDITY_REMEDIATION_DURATION_MS;
    }
    humidity_remediation_remain_ms -= LOOP_DURATION_MS;
    delay(LOOP_DURATION_MS);  // Wait LOOP_DURATION_MS
  } else {
    // Acceptable humidity levels
    ledcWrite(FAN_PWM_PIN, FAN_STOPPED);
    esp_light_sleep_start();  // Sleep LOOP_DURATION_US
  }
}


void setup() {
  Serial.begin(SERIAL_BAUDS);
  delay(1000);                                             // Wait for serial monitor
  esp_sleep_enable_timer_wakeup(LOOP_DURATION_MS * 1000);  // Set light sleep duration
  sht4x_setup();
  storage_setup();
  ledcAttach(FAN_PWM_PIN, FAN_PWM_FREQ_HZ, FAN_PWM_RESOLUTION_BITS);
}


// --- Utility Functions ---

void sht4x_setup() {
  if (!sht4.begin(&SHT4_WIRE)) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(10);
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  // Heater not needed as our environment should never reach 90% rH
  sht4.setHeater(SHT4X_NO_HEATER);
}

void storage_setup() {
  if (!rh_store.begin()) {
    Serial.println("Halting due to NVS init failure for HumidityStore.");
    while (1) delay(1000);
  }
}
