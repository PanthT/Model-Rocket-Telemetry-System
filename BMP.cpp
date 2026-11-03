#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <MPU9250.h>
#include <math.h>

Adafruit_BMP085 bmp;
MPU9250 imu;

constexpr uint8_t IMU_ADDRESS = 0x68; // Use 0x69 if AD0 is high
constexpr uint32_t OUTPUT_INTERVAL_MS = 100; // About 10 rows/second

float groundPressurePa = 0.0f;
uint32_t lastOutputMs = 0;
bool haveImuSample = false;

void halt(const __FlashStringHelper *message) {
  Serial.println(message);
  while (true) {
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  delay(500);

  if (!bmp.begin(BMP085_STANDARD)) {
    halt(F("# ERROR: BMP085 not detected"));
  }

  MPU9250Setting settings;
  settings.accel_fs_sel = ACCEL_FS_SEL::A16G;
  settings.gyro_fs_sel = GYRO_FS_SEL::G2000DPS;

  if (!imu.setup(IMU_ADDRESS, settings)) {
    halt(F("# ERROR: MPU9250 initialization failed"));
  }

  Serial.println(F("# Keep sensors still: measuring ground pressure"));

  // Average pressure at startup to establish zero relative height.
  const uint8_t samples = 30;
  for (uint8_t i = 0; i < samples; ++i) {
    const int32_t pressure = bmp.readPressure();

    if (pressure <= 0) {
      halt(F("# ERROR: Invalid startup pressure"));
    }

    groundPressurePa += pressure;
    delay(20);
  }

  groundPressurePa /= samples;

  Serial.println(F(
    "time_ms,temp_C,pressure_Pa,height_m,"
    "ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps"
  ));

  lastOutputMs = millis();
}

void loop() {
  // Update as often as possible; print less frequently.
  if (imu.update()) {
    haveImuSample = true;
  }

  const uint32_t now = millis();

  if (!haveImuSample ||
      uint32_t(now - lastOutputMs) < OUTPUT_INTERVAL_MS) {
    return;
  }

  lastOutputMs = now;
  haveImuSample = false;

  // Capture the latest IMU readings before the blocking BMP reads.
  const float ax = imu.getAccX();
  const float ay = imu.getAccY();
  const float az = imu.getAccZ();
  const float gx = imu.getGyroX();
  const float gy = imu.getGyroY();
  const float gz = imu.getGyroZ();

  const float temperatureC = bmp.readTemperature();
  const int32_t pressurePa = bmp.readPressure();

  if (pressurePa <= 0) {
    Serial.println(F("# ERROR: Invalid pressure reading"));
    return;
  }

  // Approximate height relative to startup using a standard atmosphere.
  const float heightM =
      44330.0f *
      (1.0f - powf(float(pressurePa) / groundPressurePa, 0.1903f));

  Serial.print(now);
  Serial.print(',');
  Serial.print(temperatureC, 2);
  Serial.print(',');
  Serial.print(pressurePa);
  Serial.print(',');
  Serial.print(heightM, 2);
  Serial.print(',');
  Serial.print(ax, 3);
  Serial.print(',');
  Serial.print(ay, 3);
  Serial.print(',');
  Serial.print(az, 3);
  Serial.print(',');
  Serial.print(gx, 2);
  Serial.print(',');
  Serial.print(gy, 2);
  Serial.print(',');
  Serial.println(gz, 2);
}