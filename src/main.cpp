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

  Serial.println("initializing apds9960");

  if (apds.init()) {
    Serial.println("it works");
  } else {
    Serial.println("nope");
    while (1);
  }

  

  // gesture recognition
  if (apds.enableGestureSensor(true)) {
    Serial.println("running");
  } else {
    Serial.println("failed");
  }
}

void loop() {
  if (apds.isGestureAvailable()) {
    int gesture = apds.readGesture();

    switch (gesture) {
      case DIR_UP:    Serial.println("UP"); break;
      case DIR_DOWN:  Serial.println("DOWN"); break;
      case DIR_LEFT:  Serial.println("LEFT"); break;
      case DIR_RIGHT: Serial.println("RIGHT"); break;
      case DIR_NEAR:  Serial.println("NEAR"); break;
      case DIR_FAR:   Serial.println("FAR"); break;
      default:        Serial.println("NONE");
    }
  }

  delay(150); // slow loop avoids I2C timeouts
}
