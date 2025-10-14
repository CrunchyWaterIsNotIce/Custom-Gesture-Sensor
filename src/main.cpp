#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

// This is your custom library from Edge Impulse!
#include <Custom_Gesture_Sensor_inferencing.h>

// --- Hardware Setup ---
SparkFun_APDS9960 apds = SparkFun_APDS9960();
#define SDA_PIN 21
#define SCL_PIN 22
#define LED_BUILTIN 2 // The blue LED on most ESP32 boards

// --- Edge Impulse Constants ---
// This buffer will hold the sensor data for one 2-second gesture window
// It's crucial that its size matches what the model was trained on.
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
// This index keeps track of our position in the features buffer
int feature_ix = 0;

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Start with LED off

  Wire.begin(SDA_PIN, SCL_PIN, 100000); // 100kHz I2C frequency
  Wire.setTimeout(2500);

  Serial.println("Initializing APDS-9960 sensor...");
  if (apds.init()) {
    Serial.println("Sensor Initialized!");
  } else {
    Serial.println("Failed to initialize APDS-9960. Check wiring.");
    while (1);
  }
  apds.enableProximitySensor(true);
  apds.enableLightSensor(true);

  Serial.println("\n--- Gesture Recognition Active ---");
  Serial.println("Perform a gesture over the sensor.");
}

void loop() {
  uint8_t proximity_value = 0;
  apds.readProximity(proximity_value);

  // TRIGGER: Start recording when a hand gets close (proximity > 10)
  // and we are not already in the middle of a recording.
  if (proximity_value > 10 && feature_ix == 0) {
    Serial.println("Recording gesture...");
    digitalWrite(LED_BUILTIN, HIGH); // Turn on LED to indicate recording
  }

  // If a recording has been triggered, start filling the buffer
  if (feature_ix > 0 || (proximity_value > 10 && feature_ix == 0)) {
    uint16_t r = 0, g = 0, b = 0;
    apds.readRedLight(r);
    apds.readGreenLight(g);
    apds.readBlueLight(b);

    // Add the current sensor readings to our features buffer
    features[feature_ix++] = (float)proximity_value;
    features[feature_ix++] = (float)r;
    features[feature_ix++] = (float)g;
    features[feature_ix++] = (float)b;
  }

  // Have we collected a full window of data?
  if (feature_ix >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
    digitalWrite(LED_BUILTIN, LOW); // Turn off LED, recording is done

    // --- Run the Classifier ---

    // Wrap the raw features buffer in a signal object
    signal_t signal;
    int err = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
      ei_printf("Failed to create signal from buffer (%d)\n", err);
      feature_ix = 0; // Reset for next recording
      return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };
    err = run_classifier(&signal, &result, false);
    if (err != EIDSP_OK) {
      ei_printf("ERR: Failed to run classifier (%d)\n", err);
      feature_ix = 0; // Reset for next recording
      return;
    }

    // Print the prediction results
    ei_printf("Predictions (DSP: %d ms., Classification: %d ms.): \n",
        result.timing.dsp, result.timing.classification);

    // Find the prediction with the highest score
    float max_confidence = 0.0;
    String predicted_gesture = "";
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
      ei_printf("  %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
      if (result.classification[ix].value > max_confidence) {
        max_confidence = result.classification[ix].value;
        predicted_gesture = result.classification[ix].label;
      }
    }

    // --- Act on the Result ---
    // A confidence threshold of 0.8 (80%) is a good starting point
    if (max_confidence > 0.80) {
      Serial.print("WINNING GESTURE: ");
      Serial.println(predicted_gesture);
    } else {
      Serial.println("Uncertain prediction.");
    }

    // Reset for the next gesture
    feature_ix = 0;
    Serial.println("\nReady for next gesture.");
  }

  delay(100);
}