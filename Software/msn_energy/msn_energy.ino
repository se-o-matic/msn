/*
 * msn_energy.ino
 * MySensors Node for reading actual kwh from an energy meter.
 * 
 * Hardware:
 * msn board by SE
 * IR head for serial communication with the meter.
 *
 */

// Enable and select radio type attached
#define MY_RADIO_RFM69
#define MY_RFM69_NEW_DRIVER
#define MY_RFM69_FREQUENCY RFM69_433MHZ 
#include <MySensors.h>
 
#define CHILD_ID_WATT  0
#define CHILD_ID_KWH   1

#define SKETCHSTR "Energy Meter"
#define SKETCHVER "0.1.1"

uint32_t SLEEP_TIME = 1800000;    // sleep time between reads (seconds * 1000 milliseconds)
#define PART_HOUR 2000            // 1/h * 1000 ==> kWh --> W

// Battery stuff
#define EMPTY 536                 // Brown Out ATMega   //536 equal to 1.8 V
#define SCALE 0.2415              // (100/950-EMPTY)    //950 equal to 3.2 V
int BATTERY_SENSE_PIN = A0;       // input pin for battery sensing
int batteryPcnt = 0;
int oldBatteryPcnt = 0;

char buffer[30];
char debug_buf[10];
int chars = 0;
float energy = 0, energy_last = 0.0;
float power = 0;

int calc_pcnt(int value){
  // 1M, 470K divider across battery and using internal ADC ref of 1.1V
  // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
  // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
  // 3.44/1023 = Volts per bit = 0.003363075
  int batteryPcnt = ((value - EMPTY) * SCALE);
  batteryPcnt = (batteryPcnt > 100) ? 100 : batteryPcnt;
  batteryPcnt = (batteryPcnt < 0) ? 0 : batteryPcnt;
  return batteryPcnt;
}

float get_energy(void){
  float kwh = 0.0;
  int timeout = 0;
  bool done = 0;

  //read old data and purge it
  while(Serial.available())Serial.read();
 
  //start reading with requesting the ident
  Serial.write("/?!\r\n");        //meter sends ident "/PAF5EC3g00006\r\n"
  do{
    if (Serial.available() > 0){
      Serial.readBytesUntil('\n', buffer, sizeof(buffer));
      timeout = 0;
      if(strncmp(buffer, "/PAF", 4) == 0) break;
    } else {
      timeout++;
    }
    delay(1);
  } while(timeout < 5000);
 
  // Daten anfragen
  Serial.write(0x06);       // ACK
  Serial.write("000\r\n");
 
  //Pafal sendet alle Daten
  timeout = 0;
  do{
    if (Serial.available() < 1) timeout++;
    else {
      timeout = 0;
      Serial.readBytesUntil('\n', buffer, sizeof(buffer));
      if(strncmp(buffer, "1.8.0", 5) == 0){
        kwh += (buffer[9]-0x30)  * 100000.00;
        kwh += (buffer[10]-0x30) *  10000.00;
        kwh += (buffer[11]-0x30) *   1000.00;
        kwh += (buffer[12]-0x30) *    100.00;
        kwh += (buffer[13]-0x30) *     10.00;
        kwh += (buffer[14]-0x30);     //1.00
        kwh += (buffer[16]-0x30) *      0.10;
        kwh += (buffer[17]-0x30) *      0.01;
      }
      if(buffer[0] == 0x03) done = 1;   // ETX = Ende der Daten
    }
    delay(1);
  } while (timeout < 5000 && !done);
  return kwh;
}

void setup() {
  // use the 1.1 V internal reference
  analogReference(INTERNAL);

  Serial.begin(300, SERIAL_7E1);        // Meter uses 300 baud, 7 data bits, even parity, 1 stop bit

  //Wert hängt vom Zähler ab
  Serial.setTimeout(1000);    //1000 ms = default

  energy_last = get_energy();
  oldBatteryPcnt = calc_pcnt(analogRead(BATTERY_SENSE_PIN));
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCHSTR, SKETCHVER);
 
  present(CHILD_ID_WATT, S_POWER, "Power");
  present(CHILD_ID_KWH, S_POWER, "Energy");
}

void loop() {
  energy = get_energy();
  //dtostrf(energy, 9, 2, debug_buf);
  //Serial.write(debug_buf);
  //Serial.write(" new\n");
  //dtostrf(energy_last, 9, 2, debug_buf);
  //Serial.write(debug_buf);
  //Serial.write(" old\n");
  power = (energy - energy_last) * PART_HOUR;   //normalize SLEEP_TIME to hours
  //dtostrf(power, 9, 2, debug_buf);
  //Serial.write(debug_buf);
  //Serial.write(" W\n");  
  if (power < 0) power = 0;
  energy_last = energy;
  //energy = 123.45;
  //power = 42.42;

  static MyMessage msgPower(CHILD_ID_WATT, V_WATT);
  static MyMessage msgEnergy(CHILD_ID_KWH, V_KWH);

  if(energy > 8000){
    send(msgEnergy.set(energy, 2));
  }
  if(power > 0){
    send(msgPower.set(power, 2));
  }

  // send the battery level
  batteryPcnt = calc_pcnt(analogRead(BATTERY_SENSE_PIN));
  if (batteryPcnt < oldBatteryPcnt) {
      sendBatteryLevel(batteryPcnt);
      oldBatteryPcnt = batteryPcnt;
  }
 
  sleep(SLEEP_TIME);
}
