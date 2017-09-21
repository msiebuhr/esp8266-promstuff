
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <ArduinoOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Hash.h>
#include <SPIFFSEditor.h>

char buf[16];

// Import board-specifics
#include "LoLin-NodeMCU-board.h"

// Things we don't want the outside world to see...
#include "devicemeta.h"
#include "secrets.h"

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
// Output lead:red (VCC), yellow(DATA) , black(GND) -55'C~125'C
OneWire oneWire(D5);
DallasTemperature sensors(&oneWire);
int deviceCount;

// SKETCH BEGIN
AsyncWebServer server(80);

char hostString[16] = {0};

DeviceAddress address;
DeviceAddress *deviceAddressList;

void setup() {
  Serial.begin(115200);
  delay(10);

  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  Serial.print("Hostname: ");
  Serial.println(hostString);
  WiFi.hostname(hostString);

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  // Connect to WiFi network
  Serial.println();
  Serial.printf("Connecting to %s ", SECRET_WIFI_SSID);

  WiFi.mode(WIFI_STA); // STA = STand-Alone? Doesn't start an AP on the side, which is nice.
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  // Start mDNS
  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  }
  MDNS.addService("prometheus-http", "tcp", 80); // Announce esp tcp service on port 80
  MDNS.addService("http", "tcp", 80);

  // Start thermometers
  sensors.begin();
  deviceCount = sensors.getDeviceCount();
  Serial.printf("DS18B20's initialized, %d found\n", deviceCount);

  deviceAddressList = (DeviceAddress *)malloc(sizeof(DeviceAddress) * deviceCount);
  //char* metadata = (char*) malloc(1024);
  for (int i = 0; i < deviceCount; i += 1) {
    sensors.getAddress(deviceAddressList[i], i);
    sensors.setResolution(deviceAddressList[i], 12);

    // Get meta-data for sensors
    //get_device_meta(metadata, 1024, deviceAddressList[i]);

    //Serial.println(metadata);
  }
  //free(metadata);

  SPIFFS.begin();

  // Reachable on /edit
  server.addHandler(new SPIFFSEditor("t", "t"));

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "SERVER<hr><a href='/metrics'>/metrics</a>, <a href='/edit'>/edit</a>");
  });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(BUILTIN_LED, LOW);
    AsyncResponseStream *response = request->beginResponseStream("text/prometheus; version=0.4");
    sensors.requestTemperatures(); // Send the command to get temperatures
    delay(1);

    response->print("# HELP temperature_c Calculated temperature in centigrade\n");
    response->print("# TYPE temperature_c gauge\n");

    char *metadata = (char *)malloc(1024);
    for (int i = 0; i < deviceCount; i += 1) {
      sprintAddress(buf, deviceAddressList[i]);

      // Get meta-data for sensors
      if (get_device_meta(metadata + 1, 1023, deviceAddressList[i])) {
        metadata[0] = 44; // ","
      } else {
        metadata[0] = 0;
      }

      // Read temperature
      float temperature = sensors.getTempCByIndex(i);

      if (temperature != 85) { // 85 C is some kind of NO-OP data for DS18B20's
        response->printf("temperature_c{address=\"%s\"%s} ", buf, metadata);
        response->print(sensors.getTempCByIndex(i));
        response->print("\n");
      }
    }
    free(metadata);
    response->print("\n");

    response->print("# HELP wifi_rssi_dbm Received Signal Strength Indication, dBm\n");
    response->print("# TYPE wifi_rssi_dbm counter\n");
    response->printf("wifi_rssi_dbm{} %d\n\n", WiFi.RSSI());

    response->print("# HELP heap_free_b Uptime in milliseconds\n");
    response->print("# TYPE heap_free_b gauge\n");
    response->printf("heap_free_b{} %d\n\n", ESP.getFreeHeap());

    response->print("# HELP uptime_ms Uptime in milliseconds\n");
    response->print("# TYPE uptime_ms gauge\n");
    response->printf("uptime_ms{} %lu\n\n", millis());

    request->send(response);

    digitalWrite(BUILTIN_LED, HIGH);
  });

  // Print the IP address
  Serial.print("Server started on: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  // Start the server
  server.begin();
}

void loop() {}
