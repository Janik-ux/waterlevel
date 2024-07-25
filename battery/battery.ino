#include <SoftwareSerial.h>
#define rxPin 17
#define txPin 16
#define batVpin 33
 
SoftwareSerial jsnSerial(rxPin, txPin);
int sensor_serial_timeout = 1000; // ms
 
void setup() {
  Serial.begin(115200);
  Serial.println("Beginning Test of waterlevel unit.");

  getBattVolt();

  // get waterlevel x times
  int i;
  for (i = 0; i < 2; i++) {
    Serial.print(i);
    Serial.print(": ");
    if (!getDistance()) {
      break;
    }
  }

  // 1us = 1e-6s
  ESP.deepSleep(30e6);
}
 
void loop() {
}

bool getDistance(){

  jsnSerial.begin(9600);
  jsnSerial.write(0x01);
  // Serial.println("Sent trigger signal to uss.");

  delay(50);

  int iter = 0;

  while (!jsnSerial.available()) {
    if (iter > sensor_serial_timeout/10) {
      Serial.println("Unable to reach Sensor [Serial Timeout occured]");
      return false;
    }
    Serial.print(iter);
    Serial.println("waiting for Sensor Serial coms...");
    delay(10);
    iter += 1;
  }
  
  unsigned int distance;
  byte startByte, h_data, l_data, sum = 0;
  byte buf[3];
  
  startByte = (byte)jsnSerial.read();
  if(startByte == 255){
    jsnSerial.readBytes(buf, 3);
    h_data = buf[0];
    l_data = buf[1];
    sum = buf[2];
    distance = (h_data<<8) + l_data;
    if(((0xFF + h_data + l_data)&0xFF) != sum){
      // Serial.println("Invalid result");
    } else {
      // Serial.print("Distance [mm]: "); 
      Serial.println(distance);
    } 
  } else {
    Serial.println("Did not get valid Answer from Sensor!");
    return false;
  }

  jsnSerial.end();
  return true;
}

void getBattVolt() {
  int battVolt = map(analogRead(batVpin), 0.0f, 4095.0f, 0, 3300);
  float battPerc = map(analogRead(batVpin), 3375.27f, 4095.0f, 0, 100);
  // 3375.27 := 2.72V at G33 and := 3.4V at Battery, which equals 0%
  Serial.print("Battery Voltage/Percent: ");
  Serial.print(battVolt);
  Serial.print("/");
  Serial.println(battPerc);
}
