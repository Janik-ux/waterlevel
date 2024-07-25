#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <SoftwareSerial.h>
#include <string>
#include "config.h"
#include "time.h"

RTC_DATA_ATTR unsigned int TimeStamps[MAX_DATA];
RTC_DATA_ATTR unsigned int BattVolts[MAX_DATA];
RTC_DATA_ATTR          int WaterLevels[MAX_DATA];
RTC_DATA_ATTR unsigned int Intervals[MAX_DATA];
RTC_DATA_ATTR unsigned int MetricsCount = 0;
 
SoftwareSerial jsnSerial(rxPin, txPin);

struct tm timeinfo;

HTTPClient httpGraphite;

int getDistance() {

  jsnSerial.begin(9600);
  jsnSerial.write(0x01);
  Serial.println("Sent trigger signal to uss.");

  delay(50);

  int iter = 0;

  while (!jsnSerial.available()) {
    if (iter > SENSOR_SERIAL_TIMEOUT/10) {
      Serial.println("Unable to reach Sensor [Serial Timeout occured]");
      return -1;
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
      Serial.println("Invalid result");
    } else {
      Serial.print("Distance [mm]: "); 
      Serial.println(distance);
    } 
  } else {
    Serial.println("Did not get valid Answer from Sensor!");
    return -1;
  }

  jsnSerial.end();
  return distance;
}

unsigned int getBattVolt() {
  unsigned int battVolt = analogRead(batVpin);
  float battPerc = map(battVolt, 3375.27f, 4095.0f, 0, 100);
  // 3375.27 := 2.72V at G33 and := 3.4V at Battery, which equals 0%
  Serial.print("Battery Voltage/Percent: ");
  Serial.print(battVolt);
  Serial.print("/");
  Serial.println(battPerc);
  return battVolt;
}
 
void setup() {
  Serial.begin(115200);
  Serial.println("Beginning test of waterlevel unit sending data via mqtt.");

  initWifi();

  // update time via NTP
  configTime(0, 0, NTP_SERVER);
  if (!getLocalTime(&timeinfo, 10000)) {
    Serial.println("Failed getting time, restarting!");
    ESP.restart();
  }

  Serial.print("Current Time (UTC): ");
  Serial.println(&timeinfo, "%H:%M:%S");

  // if there is space for data get it
  if (MetricsCount < MAX_DATA) {
    TimeStamps[MetricsCount] = mktime(&timeinfo); // mkgtime() for UTC, if I will change TZ sometimes
    BattVolts[MetricsCount] = getBattVolt();
    WaterLevels[MetricsCount] = getDistance();
    Intervals[MetricsCount] = SLEEP_INTERVAL;
    MetricsCount++;
  }
  // try to submit all present data to grafana graphite server
  submitToGraphite();

  Serial.print("going to sleep for seconds: ");
  Serial.println(SLEEP_INTERVAL);
  // 1s = 1e6 us
  ESP.deepSleep(SLEEP_INTERVAL*1e6);
}
 
void loop() {
}

bool initWifi() {
  
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    if (i > WIFI_CONNECT_TIMEOUT) {
      return false;
    }
    delay(1000);
    i++;
  }
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
  Serial.print("local IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void submitToGraphite() {
  // build hosted metrics json payload
  String body = String("[");
  for (int i = 0; i < MetricsCount; i++) { 
    body = body +
    "{\"name\":\"waterlevel\",\"interval\":" + Intervals[i] + ",\"value\":" + WaterLevels[i] + ",\"time\":" + TimeStamps[i] + "}," +
    "{\"name\":\"battvolt\",\"interval\":" + Intervals[i] + ",\"value\":" + BattVolts[i] + ",\"time\":" + TimeStamps[i] + "},";
  }
  body.remove(body.length()-1); // remove last comma...
  body = body + "]";

  // submit POST request via HTTPS
  // httpGraphite.setTimeout(10000);
  httpGraphite.begin(GRAPHITE_METRICS_URL);
  httpGraphite.setAuthorization(GRAPHITE_USER, GRAPHITE_API_KEY);
  httpGraphite.addHeader("Content-Type", "application/json");
  Serial.println(body);

  int httpCode = httpGraphite.POST(body);
  httpGraphite.setTimeout(10000);
  Serial.println(httpCode);
  if (httpCode>0) {
    Serial.print("HTTPS Response code: ");
    Serial.println(httpCode);
    String payload = httpGraphite.getString();
    Serial.println(payload);
    // All got sent to server, we can now delete local data
    if (httpCode == 200) {
      MetricsCount = 0;
    }
  } else {
    Serial.print("Error: ");
    Serial.println(httpGraphite.errorToString(httpCode));
  }
  httpGraphite.end();
}
