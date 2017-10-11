#include <ESP8266mDNS.h>
//#include <ArduinoOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Hash.h>
#include <IOTAppStory.h>
#include <SPIFFSEditor.h>

#define APPNAME "prom_ds18b20_sensor"
#define VERSION "V1.1.0"
#define COMPDATE __DATE__ __TIME__
#define MODEBUTTON D3
IOTAppStory IAS(APPNAME, VERSION, COMPDATE, MODEBUTTON);

char buf[16];

// Import board-specifics
//#include "LoLin-NodeMCU-board.h"

#include "devicemeta.h"

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
// Output lead:red (VCC), yellow(DATA) , black(GND) -55'C~125'C
OneWire oneWire(D5);
DallasTemperature sensors(&oneWire);
int deviceCount;

// SKETCH BEGIN
AsyncWebServer server(80);


DeviceAddress address;
DeviceAddress *deviceAddressList;

uint32_t lastCallHome = 1;

void setup() {
  IAS.serialdebug(true, 115200);

  char hostString[16] = {0};
  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  IAS.preSetConfig(hostString, false);
  IAS.begin(true);

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  // Add a few MDNS names (IAS starts MDNS for us!)
  MDNS.addService("prometheus-http", "tcp", 80);
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

  server.onNotFound([](AsyncWebServerRequest * request) {
    request->send(404);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", "SERVER<hr><a href='/metrics'>/metrics</a>, <a href='/edit'>/edit</a>, <a href='/call-home'>/call-home</a>");
  });

  server.on("/call-home", HTTP_GET, [](AsyncWebServerRequest * request) {
    lastCallHome = 0; // Set flag, as IAS aparently doesn't work from async requests
    request->send(200, "text/plain", "OK");
  });

  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest * request) {
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

    response->print("# HELP heap_free_bytes Free heap in bytes\n");
    response->print("# TYPE heap_free_bytes gauge\n");
    response->printf("heap_free_bytes{} %d\n\n", ESP.getFreeHeap());

    response->print("# HELP uptime_seconds Uptime in seconds\n");
    response->print("# TYPE uptime_seconds gauge\n");
    response->printf("uptime_seconds{} %f\n\n", ((double)millis())/1000.0);

    response->print("# HELP ias_build_info A metric with a constant '1' value labeled by sketch_md5 and version.\n");
    response->print("# TYPE ias_build_info gauge\n");
    response->printf("ias_build_info{appname=\"%s\",sketch_md5=\"%s\",version=\"%s\"} 1\n", APPNAME, ESP.getSketchMD5().c_str(), VERSION);

    request->send(response);

    digitalWrite(BUILTIN_LED, HIGH);
  });

  // Start the server
  server.begin();
}

void loop() {
  IAS.buttonLoop();
  if ((millis() - lastCallHome) > (2 * 60 * 60 * 1000) || (lastCallHome == 0)) {
    IAS.callHome(false); // true = request SPIFFS
    lastCallHome = millis();
  }
}

