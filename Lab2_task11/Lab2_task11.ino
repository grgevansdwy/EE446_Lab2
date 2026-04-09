#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

// --- Baseline references ---
float baseHumidity   = -1;
float baseTemp       = -1;
float baseMag        = -1;
int   baseClear      = -1;

// --- Cooldown ---
unsigned long lastEventTime = 0;
const unsigned long COOLDOWN_MS = 3000;

void setup() {
  Serial.begin(115200);
  delay(3000);

  // Humidity & Temperature
  if (!HS300x.begin()) {
    Serial.println("Failed to initialize HS300x.");
    while (1);
  }

  // IMU 
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  // APDS
  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960.");
    while (1);
  }

  Serial.println("All sensors started. Calibrating baseline...");

  // Take baseline readings
  delay(2000);
  baseHumidity = HS300x.readHumidity();
  baseTemp     = HS300x.readTemperature();

  float mx = 0, my = 0, mz = 0;
  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(mx, my, mz);
  }
  baseMag = sqrt(mx*mx + my*my + mz*mz);

  int r = 0, g = 0, b = 0, c = 0;
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
  }
  baseClear = c;

  Serial.println("Baseline captured. Running...");
}

void loop() {
  // ------ Humidity & Temperature ------
  float humidity    = HS300x.readHumidity();
  float temperature = HS300x.readTemperature();

  // ------ Magnetometer ------
  float mx = 0, my = 0, mz = 0;
  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(mx, my, mz);
  }
  float mag = sqrt(mx*mx + my*my + mz*mz);

  // ------ Ambient Light ------
  int r = 0, g = 0, b = 0, c = 0;
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
  }

  // ------ Flags ------
  bool humid_jump          = (humidity - baseHumidity) > 5.0;
  bool temp_rise           = (temperature - baseTemp) > 1.0;
  bool mag_shift           = abs(mag - baseMag) > 30.0;
  bool light_or_color_change = abs(c - baseClear) > 100;

  // ------ Event Label (priority order) ------
  String finalLabel = "BASELINE_NORMAL";

  unsigned long now = millis();
  bool cooldownOver = (now - lastEventTime) > COOLDOWN_MS;

  if (cooldownOver) {
    if (humid_jump || temp_rise) {
      finalLabel = "BREATH_OR_WARM_AIR_EVENT";
      lastEventTime = now;
    } else if (mag_shift) {
      finalLabel = "MAGNETIC_DISTURBANCE_EVENT";
      lastEventTime = now;
    } else if (light_or_color_change) {
      finalLabel = "LIGHT_OR_COLOR_CHANGE_EVENT";
      lastEventTime = now;
    }
  }

  // ------ Print ------
  Serial.print("raw,rh=");   Serial.print(humidity, 2);
  Serial.print(",temp=");    Serial.print(temperature, 2);
  Serial.print(",mag=");     Serial.print(mag, 2);
  Serial.print(",r=");       Serial.print(r);
  Serial.print(",g=");       Serial.print(g);
  Serial.print(",b=");       Serial.print(b);
  Serial.print(",clear=");   Serial.println(c);

  Serial.print("flags,humid_jump=");         Serial.print(humid_jump);
  Serial.print(",temp_rise=");               Serial.print(temp_rise);
  Serial.print(",mag_shift=");               Serial.print(mag_shift);
  Serial.print(",light_or_color_change=");   Serial.println(light_or_color_change);

  Serial.print("event,"); Serial.println(finalLabel);
  Serial.println();

  delay(1000);
}