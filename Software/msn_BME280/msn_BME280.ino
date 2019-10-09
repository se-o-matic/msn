/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2018 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 *
 * This is an template for RFM69CW 433 MHz sensors, which run on two AA or AAA batterys
 *
 */
 
// 1M, 470K divider across battery and using internal ADC ref of 1.1V
// Sense point is bypassed with 0.1 uF cap to reduce noise at that point
// ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
// 3.44/1023 = Volts per bit = 0.003363075
// Enable and select radio type attached
#define MY_RADIO_RFM69
#define MY_RFM69_NEW_DRIVER
#define MY_RFM69_FREQUENCY RFM69_433MHZ 
#include <MySensors.h>
#include "cactus_io_BME280_I2C.h"
 
#define CHILD_ID_TEMP  0
#define CHILD_ID_HUM   1

#define SKETCHSTR "Temp Humidity Sensor"
#define SKETCHVER "0.1"

BME280_I2C bme(0x76);
uint32_t SLEEP_TIME = 300000;    // sleep time between reads (seconds * 1000 milliseconds)

// Battery stuff
#define EMPTY 536         // Brown Out ATMega   //536 entspricht 1.8 V
#define SCALE 0.2415      // (100/950-EMPTY)    //950 entspricht 3.2 V
int BATTERY_SENSE_PIN = A0;   // select the input pin for the battery sense point
int batteryPcnt = 0, oldBatteryPcnt = 0;

float temp = 25.0, hum = 50.0;
float temp_last = 0.0, hum_last = 0.0;

int calc_pcnt(int value){
  int batteryPcnt = ((value - EMPTY) * SCALE);
  batteryPcnt = (batteryPcnt > 100) ? 100 : batteryPcnt;
  batteryPcnt = (batteryPcnt < 0) ? 0 : batteryPcnt;
  return batteryPcnt;
}

void setup() {
  // use the 1.1 V internal reference
  analogReference(INTERNAL);

  //BME280 stuff
  bme.begin();

  oldBatteryPcnt = calc_pcnt(analogRead(BATTERY_SENSE_PIN));
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCHSTR, SKETCHVER);
 
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  present(CHILD_ID_HUM, S_HUM, "Humidity");
}

void loop() {
  //read temp and hum

  static MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
  static MyMessage msgHum(CHILD_ID_HUM, V_HUM);

  bme.readSensor();
  temp = bme.getTemperature_C();
  hum = bme.getHumidity();

  if(temp != temp_last){
    send(msgTemp.set(temp, 2));
    temp_last = temp;
  }
  if(hum != hum_last){
    send(msgHum.set(hum, 2));
    hum_last = hum;
  }

  // send the battery level
  batteryPcnt = calc_pcnt(analogRead(BATTERY_SENSE_PIN));
  if (batteryPcnt < oldBatteryPcnt) {
      sendBatteryLevel(batteryPcnt);
      oldBatteryPcnt = batteryPcnt;
  }
 
  sleep(SLEEP_TIME);
}
