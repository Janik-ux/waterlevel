#include <Wire.h>
#include <VL53L0X.h>
#include "credentials.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60
#define ONBOARD_LED  2

typedef struct {
  time_t time;
  uint16_t distance;
} measurement;

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR measurement sensor_data[10];

VL53L0X dist_sensor;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

bool measure_dist() {
  // assumes, that esp32 has already updated its time at least one time over ntp

  uint16_t dist; // Distance maybe set direct to array
  time_t now; // Store time here, maybe direct into array too

  Wire.begin();
  dist_sensor.setTimeout(500);
  if (!dist_sensor.init()) {
    Serial.println("Failed to detect and initialize sensor!");
    return false;
  }

  dist_sensor.setMeasurementTimingBudget(200000);

  // get time
  time(&now);
  Serial.println(now);

  // try five times, to get good value
  for (int i=0; i<5; i++) {
    dist = dist_sensor.readRangeSingleMillimeters();
    Serial.println(dist);
    if (!dist_sensor.timeoutOccurred()) {
      break;
    }
  }

  // write dist to sleep memory
  sensor_data[0].time = now;
  sensor_data[0].distance = dist;
  Serial.println(sensor_data[0].time);
  
  return true;
}

bool send_data() {

  HTTPClient https;
  bool sent_successful;

  // try to connect to wifi
  if (!connect_wifi()) {
    return false;
  }

  Serial.print("[HTTPS] begin...\n");
  if (https.begin("https://graphite-prod-01-eu-west-0.grafana.net/graphite/metrics")) {  // HTTPS
    String body = String("[") +
                  "{\"name\":\"waterlevel-laser\",\"interval\":" + TIME_TO_SLEEP+ ",\"value\":" + sensor_data[0].distance + ",\"time\":" + sensor_data[0].time + "}]";
    Serial.println(body);

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
      sent_successful = true;
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      sent_successful = false;
    }

    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
    sent_successful = false;
  }

  if (sent_successful) {
    // empty data array
    return true;
  } else {
    return false;
  }
}

void ntp_sync_time() {
  struct tm timeinfo;
  if (!connect_wifi()) {
    return;
  }
  configTzTime(time_zone, ntpServer);
  getLocalTime(&timeinfo, 5000); // Try 5 seconds to get time
  Serial.println(&timeinfo, "Zeit synced! Datum: %d.%m.%y  Zeit: %H:%M:%S");
}

bool connect_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected to WiFi!");
    return true;
  }

  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    return false;
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void setup() {
  Serial.begin(115200);

  // wait until serial port opens for native USB devices
  while (! Serial) {
    delay(1);
  }

  Serial.println("Boot number: " + String(bootCount));
  Serial.println("Waterlevel test with VL53L0X");

  setenv("TZ", time_zone, 1); // Zeitzone  muss nach dem reset neu eingestellt werden
  tzset();

  // if time was not synced for to long or not at all, sync it 
  if (bootCount % 10 == 0) {
    ntp_sync_time();
  }

  measure_dist();

  // if some criteria is met, send the data
  if (true) {
    send_data();
  }

  bootCount++;

  // Put esp32 into deep sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop() {} 