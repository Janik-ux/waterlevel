#include <SoftwareSerial.h>
#define rxPin 17
#define txPin 16
 
SoftwareSerial jsnSerial(rxPin, txPin);
int sensor_serial_timeout = 10000; // ms
 
void setup() {
  Serial.begin(115200);
  Serial.println("Beginning Test of aj-sr04m in low power serial mode.");

  int i;

  for (i = 0; i < 200; i++) {
    Serial.print(i);
    Serial.print(": ");
    getDistance();
  }


  ESP.deepSleep(8e6);
}
 
void loop() {
}

void getDistance(){

  jsnSerial.begin(9600);
  jsnSerial.write(0x01);
  // Serial.println("Sent trigger signal to uss.");

  delay(50);

  int iter = 0;

  while (!jsnSerial.available()) {
    if (iter > sensor_serial_timeout/10) {
      Serial.println("Unable to reach Sensor [Serial Timeout occured]");
      return;
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
    return;
  }

  jsnSerial.end();
}