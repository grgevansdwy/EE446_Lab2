#include <PDM.h>
#include <Arduino_APDS9960.h>
#include <Arduino_BMI270_BMM150.h>

// --- Labels ---
bool labelAudio = false;
bool labelDark = false;
bool labelMoving = false;
bool labelNear = false;

// --- Audio Helper ---
short sampleBuffer[256];
volatile int samplesRead = 0;

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  // Audio
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM microphone.");
    while (1);
  }

  // Ambient light AND Proximity
  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }

  // IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  Serial.println("All sensors started.");
}

void loop() {
  // ------ Audio ------
  int level = 0;
  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    level = sum / samplesRead;
    samplesRead = 0;
  }

  // ------ Ambient Light ------
  int r = 0, g = 0, b = 0, c = 0;
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
  }

  // ------ IMU / Motion detection ------
  float x = 0, y = 0, z = 0;
  static float prevX = 0, prevY = 0, prevZ = 0;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
  }
  float motion = abs(x - prevX) + abs(y - prevY) + abs(z - prevZ);
  prevX = x; prevY = y; prevZ = z;

  // ------ Proximity ------
  int proximity = 255;
  if (APDS.proximityAvailable()) {
    proximity = APDS.readProximity();
  }

  // ------ Logic ------
  labelAudio   = level > 50;
  labelDark    = c < 30;
  labelMoving  = motion > 0.1;
  labelNear    = proximity < 50;

  // ------ Determine Exactly 4 Final Label ------
  String finalLabel = "";
  if (!labelAudio && !labelDark && !labelMoving && !labelNear) {
    finalLabel = "QUIET_BRIGHT_STEADY_FAR";
  } else if (labelAudio && !labelDark && !labelMoving && !labelNear) {
    finalLabel = "NOISY_BRIGHT_STEADY_FAR";
  } else if (!labelAudio && labelDark && !labelMoving && labelNear) {
    finalLabel = "QUIET_DARK_STEADY_NEAR";
  } else if (labelAudio && !labelDark && labelMoving && labelNear) {
    finalLabel = "NOISY_BRIGHT_MOVING_NEAR";
  } else {
    finalLabel = "UNKNOWN";
  }

  // ------ Print ------
  Serial.print("raw,mic="); Serial.print(level);
  Serial.print(",clear="); Serial.print(c);
  Serial.print(",motion="); Serial.print(motion, 3);
  Serial.print(",prox="); Serial.println(proximity);

  Serial.print("flags,sound="); Serial.print(labelAudio);
  Serial.print(",dark="); Serial.print(labelDark);
  Serial.print(",moving="); Serial.print(labelMoving);
  Serial.print(",near="); Serial.println(labelNear);

  Serial.print("state,"); Serial.println(finalLabel);

  Serial.println();

  delay(1000);
}