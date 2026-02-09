#include <Wire.h>
#include <Adafruit_SHT4x.h>

Adafruit_SHT4x sht4;

void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialize I2C on ESP32-C3 with SDA=D4, SCL=D5.
  Wire.begin();

  if (!sht4.begin()) {
    Serial.println("Failed to find SHT4x sensor");
    while (true) {
      delay(1000);
    }
  }

  // Use highest precision, no heater for continuous readings.
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
}

void loop() {
  sensors_event_t humidity, temp;
  if (sht4.getEvent(&humidity, &temp)) {
    Serial.print("Temp: ");
    Serial.print(temp.temperature);
    Serial.print(" C, Humidity: ");
    Serial.print(humidity.relative_humidity);
    Serial.println(" %");
  } else {
    Serial.println("Failed to read from SHT4x");
  }

  delay(2000);
}
