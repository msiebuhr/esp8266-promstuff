
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Import board-specifics
#include "LoLin-NodeMCU-board.h"

/* Ideas
    - https://github.com/esp8266/Arduino/blob/master/doc/libraries.md gives some things
    - Tag thermometers by their address, so we don't rely on implicit orderings...
    - Write (quick) Prometheus client
*/

#include "secrets.h"


// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
// Output lead:red (VCC), yellow(DATA) , black(GND) -55'C~125'C
OneWire oneWire(D5);
DallasTemperature sensors(&oneWire);
int deviceCount;

WiFiServer server(80);
int requests = 0;
char hostString[16] = {0};

DeviceAddress address;
char buf[16];
DeviceAddress* deviceAddressList;

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

  // Start the server
  server.begin();

  // Print the IP address
  Serial.print("Server started on: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  // Start mDNS
  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  }
  MDNS.addService("prometheus-http", "tcp", 80); // Announce esp tcp service on port 80
  Serial.println("mDNS responder started");

  // Start thermometers
  sensors.begin();
  deviceCount = sensors.getDeviceCount();
  Serial.printf("DS18B20's initialized, %d found\n", deviceCount);

  deviceAddressList = (DeviceAddress*) malloc(sizeof(DeviceAddress) * deviceCount);
  for (int i = 0; i < deviceCount; i += 1) {
    sensors.getAddress(deviceAddressList[i], i);
  }
}

// function to print a device address
void sprintAddress(char buf[], DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) {
      sprintf(buf + sizeof(char) * i * 2, "0%x", deviceAddress[i]);
    } else {
      sprintf(buf + sizeof(char) * i * 2, "%x", deviceAddress[i]);
    }
  }
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  while (!client.available()) {
    delay(1);
  }

  Serial.println("Client request");
  digitalWrite(BUILTIN_LED, LOW);
  requests = requests + 1;

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // TODO: Match if the client wants metrics and reply with a bunch of those...
  if (request.indexOf("/metrics") != -1) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/prometheus; version=0.4");
    client.println(""); //  do not forget this one

    sensors.requestTemperatures(); // Send the command to get temperatures
    delay(1);

    client.print("# HELP temperature_c Calculated temperature in centigrade\n");
    client.print("# TYPE temperature_c gauge\n");

    for (int i = 0; i < deviceCount; i += 1) {
      sprintAddress(buf, deviceAddressList[i]);
      client.printf("temperature_c{sensor=\"%s\"} ", buf);
      client.print(sensors.getTempCByIndex(i));
      client.print("\n");
    }

    client.print("# HELP wifi_rssi_dbm Number of requests processed\n");
    client.print("# TYPE wifi_rssi_dbm counter\n");
    client.printf("wifi_rssi_dbm{} %d\n\n", WiFi.RSSI());

    client.print("# HELP http_requests_total Number of requests processed\n");
    client.print("# TYPE http_requests_total counter\n");
    client.printf("http_requests_total{} %d\n\n", requests);

    client.print("# HELP heap_free_b Uptime in milliseconds\n");
    client.print("# TYPE heap_free_b gauge\n");
    client.printf("heap_free_b{} %d\n\n", ESP.getFreeHeap());

    client.print("# HELP uptime_ms Uptime in milliseconds\n");
    client.print("# TYPE uptime_ms gauge\n");
    client.printf("uptime_ms{} %d\n\n", millis());
  } else {
    // Return the response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); //  do not forget this one
    client.println("<!DOCTYPE HTML>");
    client.println("<html><a href=\"/metrics\">/metrics</a></html>");
  }

  digitalWrite(BUILTIN_LED, HIGH);
  Serial.println("Client disonnected");
  Serial.println("");

}

