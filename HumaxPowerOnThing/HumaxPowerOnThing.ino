/*
 HumaxPower

 Turn on my Humax PVR, if it is not already on.

 idle) Wait for a command to turn on the Humax
 check) When command is received, check if Humax is already on. If on, go to state "result"
 turn_on) If not on, send IR command to turn on.
 wait) Wait for boot time, then go back to check (unless limit reached)
 result)  Send confirmation or error

 Debug info sent to Serial.
 LED will be on while steps 2-6 in progress.

*/

#include <string.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>

// Forward declarations
void handleRoot();
void handlePoweron();
void handleIRtest();

const int led_pin =  5;      // the number of the LED pin on the Sparkfun ESP8266 Thing
const int ir_led_pin = 4;   // GPIO pin number for IR led
#define HUMAX_POWER 0x800FF   // IR code for power button
IRsend irsend(ir_led_pin);

// To specify your wifi settings here, comment out the #include below
// and modify and uncomment the two lines below that
#include "/pc-share/Programming/Arduino/sketchbook/wifi-settings.h"
//char ssid[] = "mywifi";            // your network SSID (name)
//char pass[] = "12345678";        // your network password

// Web server on port 80
ESP8266WebServer server(80);
String humax;

bool setupWiFi(char *ssid, char *pass) {
  int status;     // the Wifi radio's status
  int retries = 0;

  status = WiFi.status();
  // check for the presence of the shield
  if (status == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    return false;
  } 
  
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    if (retries++ > 3) return false;
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // you're connected now, so print out the data
  Serial.println("You're connected to the network");

  return true;
}

void printWifiStatus() {
  WiFi.printDiag(Serial);

  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in the browser
  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}

void setup()
{
  delay(5000);
  // initialize serial for debugging
  Serial.begin(115200);
  
  // initialize the LED pin as an output:
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  // IR LED is output as well
  pinMode(ir_led_pin, OUTPUT);
}

/*
 * Check whether Hunax PVR is running.
 * 
 * Currently this checks by trying to connect to FTP.
 * 
 */
bool humax_is_on(const char *humax) {
#if 1
  // Try connecting to FTP server
  WiFiClient client;
  int ret;
  ret = client.connect(humax, 21);
  if (ret) client.stop();
  return ret;
#else
  // Try getting a DLNA SSDP response (not currently working)
  WiFiUDP Udp;
  char request[] = "M-SEARCH * HTTP/1.1\r\n"; // Probably need more of the request
  char packetBuffer[100];
  Udp.begin(1900);

  Udp.beginPacket(humax, 1900);
  Udp.write(request, sizeof(request));
  Udp.endPacket();
  Serial.println("Packet sent");

  // wait to see if a reply is available
  unsigned long startMillis = millis();
  do {if (Udp.parsePacket()) break;} while(millis() - startMillis < 5000);
  if (Udp.parsePacket() > 0) {
    Serial.print("packet received: ");
    int len = Udp.read(packetBuffer, sizeof(packetBuffer));
    Serial.println(len);;
    return true;
  }
  return false;
#endif
}

void loop()
{
  static enum {init, idle, check, turn_on, wait, result} state = init;
  static int success = false;
  static int start_time;
  static unsigned long start_wait;

  switch (state) {
    case init: Serial.println("State: init");
      // Set up the WiFi
      if (setupWiFi(ssid, pass)) {
        printWifiStatus();
        if ( MDNS.begin ( "humax-power" ) ) {
          Serial.println ( "MDNS responder started" );
        }
        server.on("/", handleRoot);
        server.on("/poweron", handlePoweron);
        server.on("/irtest", handleIRtest);
        server.onNotFound([]() {
          server.send ( 404, "text/plain", "File not found" );
        } );
        server.begin(); // Note: this does not return a status
        digitalWrite(led_pin, LOW);
        Serial.println ( "HTTP server started" );
        state = idle;
      } else {
        Serial.println("Could not connect to WiFi. Waiting for retry");
        digitalWrite(led_pin, !digitalRead(led_pin)); // Toggle LED
        delay(1000);
      }
      break;
    
    case idle: //Serial.println("State: idle");
      server.handleClient(); 
      break;
 
     default: Serial.print("BUG: invalid state ");
      Serial.println(state);
      state = init;
  }
  
  digitalWrite(led_pin, !digitalRead(led_pin)); // Toggle LED
  delay(1000);
  yield(); // Not necessary with delay above, but just in case we remove the delay...
}

void handleRoot() {
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf ( temp, 400,

"<html>\
  <head>\
    <title>HumaxPower on Thing</title>\
  </head>\
  <body>\
    <h1>HumaxPower on Thing</h1>\
    <p>To turn on the humax power access\
    <pre>http://%s/poweron?humax=<name-or-ip></pre></p>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>Last reset reason: %s</p>\
  </body>\
</html>",

    WiFi.localIP().toString().c_str(), hr, min % 60, sec % 60, ESP.getResetReason().c_str()
  );
  server.send ( 200, "text/html", temp );
}

void handlePoweron() {
  unsigned long max_wait = 120 * 1000; // Maximum time for Humax to boot, in milliseconds
  char temp[400];
  bool nocheck = server.hasArg("nocheck"); // For testing
  bool nowait = server.hasArg("nowait"); // For testing
  digitalWrite(led_pin, HIGH);
  
  if (server.hasArg("timeout") && server.arg("timeout").length() > 0) max_wait = server.arg("timeout").toInt() * 1000;
  
  humax = server.arg("humax");
  if (humax && humax.length() > 0) {
    Serial.print("Humax hostname = ");
    Serial.println(humax);
    
    if ((!nocheck) && humax_is_on(humax.c_str())) {
      Serial.println("Already on");
      server.send(200, "text/html",
"<html>\
  <head>\
    <title>HumaxPower on Thing</title>\
  </head>\
  <body>\
    <p><b>Success:</b> Humax already on.</p>\
  </body>\
</html>"
      );
      
    } else {
      unsigned long start_wait = millis();
      irsend.sendNEC(HUMAX_POWER, 32);
      delay(5000);
      while (! (nowait || humax_is_on(humax.c_str()))) {
        if (millis() - start_wait > max_wait) {
          Serial.print("Timeout ");
          Serial.println((millis() - start_wait)/1000);
          server.send(408, "text/html",
"<html>\
  <head>\
    <title>HumaxPower on Thing</title>\
  </head>\
  <body>\
    <p><b>Failed:</b> Humax not responding.</p>\
  </body>\
</html>"
          );
          digitalWrite(led_pin, LOW);
          return;
        } else {
          delay(1000);
        }        
      }

      Serial.println("Success");
      snprintf ( temp, sizeof(temp),
"<html>\
  <head>\
    <title>HumaxPower on Thing</title>\
  </head>\
  <body>\
    <p><b>Success:</b> Humax powered up after %d seconds.</p>\
  </body>\
</html>",
      (millis() - start_wait)/1000
      );
      server.send(200, "text/html", temp);
    }
  }else {
    server.send ( 400, "text/plain", "Must specify humax hostname" );
  }
  
  digitalWrite(led_pin, LOW);
}

void handleIRtest() {
  int interval=1000, duration=10000;
  unsigned long start_wait = millis();
  
  digitalWrite(led_pin, HIGH);
  digitalWrite(ir_led_pin, LOW);

  if (server.hasArg("interval") && server.arg("interval").length() > 0) interval = server.arg("interval").toInt();
  if (server.hasArg("duration") && server.arg("duration").length() > 0) duration = server.arg("duration").toInt();
  Serial.print("IRtest: interval ");
  Serial.print(interval);
  Serial.print(", duration ");
  Serial.println(duration);
  
  while ((millis() - start_wait) < duration) {
    delay(interval);
    digitalWrite(ir_led_pin, !digitalRead(ir_led_pin)); // Toggle IR LED
  }

  server.send(200, "text/html",
"<html>\
  <head>\
    <title>HumaxPower on Thing</title>\
  </head>\
  <body>\
    <p>IR test completed.</p>\
  </body>\
</html>"
  );

  digitalWrite(ir_led_pin, LOW);
  digitalWrite(led_pin, LOW);
}

void handleNotFound() {
  server.send ( 404, "text/plain", "File not found" );
}

