#include <TinyWireM.h>
#include <USI_TWI_Master.h>

#include <Tiny4kOLED.h>

#define ROTOR_FACES 3

const int SENSOR_PIN = A3;
const int HYSTERESIS = 27;
const float SENSOR_PSI_MAX = 200.0;
const float SENSOR_V[2] = {0.5, 4.5};
const int ADC_MAX = 1023; // 2^10 - 1
const float ADC_V_MAX = 5.0;
const int CALIBRATION_COUNT = 10;
const int CALIBRATION_DELAY = 100; // ms
int CALIBRATION_VALUE = 0;

float convertToPSI(int value)
{
  const int delta = value - CALIBRATION_VALUE;
  const float a = ADC_V_MAX / ADC_MAX; 
  const float b = (SENSOR_V[1] - SENSOR_V[0]) / SENSOR_PSI_MAX;
  return delta * a / b;
}

unsigned long calculateRotorRotationDuration(unsigned long peakTimes[ROTOR_FACES])
{
  unsigned long avgPeakToPeakDuration = 0.0;
  for (int i = 0; i < ROTOR_FACES - 1; i++)
  {
    avgPeakToPeakDuration += (peakTimes[i + 1] - peakTimes[i]);
  }
  avgPeakToPeakDuration /= ROTOR_FACES - 1;
  return avgPeakToPeakDuration * ROTOR_FACES;
}

float calculateRPM(unsigned long duration)
{
  return 3.0 * 1000.0 * 60.0 / duration;
}

void setup() {
  oled.begin();
  oled.setRotation(0);
  oled.clear();
  oled.on();
  oled.setFont(FONT6X8P);
  setupInitalAndCallCalibrate();
}

void setupInitalAndCallCalibrate() {
  oled.clear();
  oled.setCursor(24, 1);
  oled.print(F("Rotary Engine"));
  oled.setCursor(16, 2);
  oled.print(F("Compression Tool"));
  delay(5000);
  oled.clear();
  beginCalibrate();
}

void beginCalibrate() {
  
  for (int i = 0; i < CALIBRATION_COUNT; i++)
  {
    oled.clear();
    oled.setCursor(30, 1);
    oled.print(F("Calibrating..."));
    delay(CALIBRATION_DELAY);
    const int v = analogRead(SENSOR_PIN);
    oled.setCursor(55, 2);
    oled.print(v);
    CALIBRATION_VALUE += v;
  }
  oled.clear();
  oled.setCursor(43, 1);
  CALIBRATION_VALUE /= CALIBRATION_COUNT;
  oled.print(F("Avg: "));
  oled.print(CALIBRATION_VALUE);
  oled.setCursor(25, 2);
  oled.print(F("Begin Cranking"));

}

void loop() {
  int facePeaks[ROTOR_FACES] = {0};
  unsigned long peakTimes[ROTOR_FACES] = {0};
  int v = 0;
  for (int i = 0; i < ROTOR_FACES; i++)
  {
    v = analogRead(SENSOR_PIN);
    while (v >= facePeaks[i] - HYSTERESIS)
    {
      if (v > facePeaks[i])
      {
        facePeaks[i] = v;
        peakTimes[i] = millis();
      }
      v = analogRead(SENSOR_PIN);
    }
    int minimum = facePeaks[i];
    while (v <= minimum + HYSTERESIS)
    {
      if (v < minimum)
      {
        minimum = v;
      }
      v = analogRead(SENSOR_PIN);
    }
  }
  const float rpm = calculateRPM(calculateRotorRotationDuration(peakTimes));
  float psi[ROTOR_FACES] = {0.0};
  for (int i = 0; i < ROTOR_FACES; i++)
  {
    psi[i] = convertToPSI(facePeaks[i]);
  }
  {
    oled.clear();
    oled.setCursor(25, 0);
    oled.print(F("Test Complete!"));
    oled.setCursor(5, 1);
    oled.print(F("PSI: "));
    for (int i = 0; i < ROTOR_FACES; i++)
    {
      oled.print(psi[i]);
      if (i < ROTOR_FACES - 1)
      {
        oled.print(F(" "));
      }
    }
    oled.setCursor(30, 2);
    oled.print(F("RPM: "));
    oled.print(rpm);
  }
}
