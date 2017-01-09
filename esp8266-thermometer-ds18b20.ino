
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>

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

int ledPin = 2; // GPIO2

WiFiServer server(80);
int requests = 0;
int value = LOW;
char hostString[16] = {0};

void setup() {
  Serial.begin(115200);
  delay(10);

  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  Serial.print("Hostname: ");
  Serial.println(hostString);
  WiFi.hostname(hostString);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SECRET_WIFI_SSID);

  WiFi.mode(WIFI_STA); // STA = STand-Alone? Doesn't start an AP on the side, which is nice.
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");


  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  MDNS.addService("prometheus-http", "tcp", 80); // Announce esp tcp service on port 8080

  // Start thermometers
  sensors.begin();
  Serial.print("Found ");
  deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Match the request

  if (request.indexOf("/LED=ON") != -1) {
    value = LOW;
  } else if (request.indexOf("/LED=OFF") != -1) {
    value = HIGH;
  } else {
    if (value == LOW) {
      value = HIGH;
    } else {
      value = LOW;
    }
  }
  digitalWrite(ledPin, value);

  // TODO: Match if the client wants metrics and reply with a bunch of those...
  if (request.indexOf("/metrics") != -1) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/prometheus; version=0.4");
    client.println(""); //  do not forget this one

    sensors.requestTemperatures(); // Send the command to get temperatures
    delay(100);

    client.print("# HELP temperature_c Calculated temperature in centigrade\n");
    client.print("# TYPE temperature_c gauge\n");

    for (int i = 0; i < deviceCount; i += 1) {
      client.printf("temperature_c{sensor=\"%d\"} ", i);
      client.print(sensors.getTempCByIndex(i));
      client.print("\n");
    }

    client.print("# HELP wifi_rssi_dbm Number of requests processed\n");
    client.print("# TYPE wifi_rssi_dbm counter\n");
    client.printf("wifi_rssi_dbm{} %d\n\n", WiFi.RSSI());

    client.print("# HELP http_requests_total Number of requests processed\n");
    client.print("# TYPE http_requests_total counter\n");
    client.print("http_requests_total{} ");
    client.print(requests++);
    client.print("\n\n");

    client.print("# HELP uptime_ms Uptime in milliseconds\n");
    client.print("# TYPE uptime_ms gauge\n");
    client.print("uptime_ms{} ");
    client.print(millis());
    client.print("\n\n");
    return;
  }

  // Set ledPin according to the request
  //digitalWrite(ledPin, value);


  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.print("Led pin is now: ");

  if (value == LOW) {
    client.print("On");
  } else {
    client.print("Off");
  }
  client.println("<br><br>");
  client.println("Click <a href=\"/LED=ON\">here</a> turn the LED on pin 2 ON<br>");
  client.println("Click <a href=\"/LED=OFF\">here</a> turn the LED on pin 2 OFF<br>");

  client.println("<br>Requests ");
  client.print("DATA: ");
  client.print(requests++);
  client.println(" requests");


  client.println("<br>Uptime ");
  client.print("DATA: ");
  client.print(millis());
  client.println(" ms");


  client.println("</html>");

  Serial.println("Client disonnected");
  Serial.println("");

}

