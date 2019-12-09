/*
 * My Sensors Node for temperature, humidity and light level
 * 
 * Hardware: MSN R00
 * BME280 (temp, hum) at I2C
 * BH1750 (light level) at I2C
 * 
 * Runs at 2 x AA batterys
 * 
 */

// RFM69CW at 433 MHz / Raspebrry Pi as ethernet gateway
#define MY_RADIO_RFM69
#define MY_RFM69_NEW_DRIVER
#define MY_RFM69_FREQUENCY RFM69_433MHZ 
#include <MySensors.h>

// BME280 library from Adafruit, need the Adafruit "Unified Sensor library", too
#include <Adafruit_BME280.h>
// BH1750 library from https://github.com/claws/BH1750
#include "BH1750.h"

// Define child IDs and messages
#define CHILD_ID_TEMP  0
#define CHILD_ID_HUM   1
#define CHILD_ID_LUX   2
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgLux(CHILD_ID_LUX, V_LEVEL);

// Define strings, which are send to the controller
#define SKETCHSTR "Temp Humidity LUX Sensor"
#define SKETCHVER "0.2.0"

// Sleep time for sensor 
uint32_t SLEEP_TIME = 300000;    // sleep time between reads (seconds * 1000 milliseconds)

// Constructor for sensors
Adafruit_BME280 bme;
#define BME280_TEMP_OFFSET -1.0    // calibration ofsset for the temperature
BH1750 bh1750;

// Battery stuff
#define EMPTY 536         // Brown Out ATMega   //536 equal to 1.8 V
#define SCALE 0.2415      // (100/950-EMPTY)    //950 equal to 3.2 V
int BATTERY_SENSE_PIN = A0;   // select the input pin for the battery sense point
int batteryPcnt = 0, oldBatteryPcnt = 0;

// Global variables for sensor values
float temp = 25.0, hum = 50.0;
float temp_last = 0.0, hum_last = 0.0;
float lux = 100, lux_last = 0;

// Helper for calculate battery percentage
int calc_pcnt(int value){
  int batteryPcnt = ((value - EMPTY) * SCALE);
  batteryPcnt = (batteryPcnt > 100) ? 100 : batteryPcnt;
  batteryPcnt = (batteryPcnt < 0) ? 0 : batteryPcnt;
  return batteryPcnt;
}

void setup() {
  // use the 1.1 V internal reference
  analogReference(INTERNAL);

  bh1750.begin(BH1750::ONE_TIME_HIGH_RES_MODE);

  //BME280 stuff
  bme.begin(0x77, &Wire);    //0x76 for break out board, 0x77 for MSN_R00
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1,   // temperature
                  Adafruit_BME280::SAMPLING_NONE, // pressure
                  Adafruit_BME280::SAMPLING_X1,   // humidity
                  Adafruit_BME280::FILTER_OFF );

  oldBatteryPcnt = calc_pcnt(analogRead(BATTERY_SENSE_PIN));
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCHSTR, SKETCHVER);
 
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  present(CHILD_ID_HUM, S_HUM, "Humidity");
  present(CHILD_ID_LUX, S_LIGHT_LEVEL, "Lux");
}

void loop() {
  //read temp and hum and lux
  bme.takeForcedMeasurement();
  temp = bme.readTemperature();
  temp = temp + BME280_TEMP_OFFSET;
  hum = bme.readHumidity();

  lux = bh1750.readLightLevel();

  if(temp != temp_last){
    send(msgTemp.set(temp, 2));
    temp_last = temp;
  }
  if(hum != hum_last){
    send(msgHum.set(hum, 2));
    hum_last = hum;
  }
  if(lux != lux_last){
    send(msgLux.set(lux, 2));
    lux_last = lux;
  }

  // send the battery level
  batteryPcnt = calc_pcnt(analogRead(BATTERY_SENSE_PIN));
  if (batteryPcnt < oldBatteryPcnt) {
      sendBatteryLevel(batteryPcnt);
      oldBatteryPcnt = batteryPcnt;
  }
 
  sleep(SLEEP_TIME);
}
