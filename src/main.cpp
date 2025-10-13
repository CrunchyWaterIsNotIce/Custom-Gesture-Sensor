#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

SparkFun_APDS9960 apds = SparkFun_APDS9960();

#define SDA_PIN 21
#define SCL_PIN 22

void setup() {
  Serial.begin(115200);
  delay(100);

  // at 100 kHz
  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.setTimeout(2500); // increase timeout to avoid error 263

  Serial.println("Initializing apds9960");

  if (apds.init()) {
    Serial.println("Initialized!");
  } else {
    Serial.println("Fail to initialize apds9960");
    while (1);
  }
  apds.enableProximitySensor(true);
  apds.enableLightSensor(true);
}

void loop() {
  uint8_t proximity = 0;
  apds.readProximity(proximity);
  
  uint16_t r = 0, g = 0, b = 0;
  apds.readRedLight(r);
  apds.readGreenLight(g);
  apds.readBlueLight(b);
  Serial.printf("Proximity: %d | RGB: %d, %d, %d\n", proximity, r, g, b);

  delay(150);
}
