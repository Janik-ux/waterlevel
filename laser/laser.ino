#include "Adafruit_VL53L0X.h"
#include "Wire.h"
#include "credentials.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5
#define ONBOARD_LED  2

RTC_DATA_ATTR int bootCount = 0;

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// NTP Client
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);

void send_data(int data, unsigned long time) {

  HTTPClient https;

  Serial.print("[HTTPS] begin...\n");
  if (https.begin("https://graphite-prod-01-eu-west-0.grafana.net/graphite/metrics")) {  // HTTPS
    String body = String("[") +
                  "{\"name\":\"waterlevel-laser\",\"interval\":5,\"value\":"+data+",\"time\":" + time + "}]";

    https.setAuthorization(GRAPHITE_USER, GRAPHITE_API_KEY);
    https.addHeader("Content-Type", "application/json");

    Serial.print("[HTTPS] POST...\n");
    // start connection and send HTTP header
    int httpCode = https.POST(body);

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

      String payload = https.getString();
      Serial.println(payload);
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
}

void setup() {
  Serial.begin(115200);
  int dist;
  // blink led to indicate new boot
  pinMode(ONBOARD_LED,OUTPUT);
  digitalWrite(ONBOARD_LED,HIGH);
  delay(100);
  digitalWrite(ONBOARD_LED,LOW);

  // wait until serial port opens for native USB devices
  while (! Serial) {
    delay(1);
  }

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  Serial.println("Waterlevel test with VL53L0X");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
 
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

  // measuring the data
  VL53L0X_RangingMeasurementData_t measure;
  Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  // write data to var if its good
  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    int dist = measure.RangeMilliMeter;
    Serial.print("Distance (mm): "); Serial.println(dist);
  } else {
    int dist = 0;
    Serial.println(" out of range ");
  }

  // update time
  while (!ntpClient.update()) {
    yield();
    ntpClient.forceUpdate();
  }
  // Get current timestamp
  unsigned long ts = ntpClient.getEpochTime();

  send_data(dist, ts);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
}


void loop() {

}
