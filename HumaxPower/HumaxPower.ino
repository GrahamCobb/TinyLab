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

 NOTE: The serial buffer size must be larger than 36 + packet size
 In this example we use an UDP packet of 48 bytes so the buffer must be
 at least 36+48=84 bytes that exceeds the default buffer size (64).
 You must modify the serial buffer size to 128
 For HardwareSerial modify _SS_MAX_RX_BUFF in
   Arduino\hardware\arduino\avr\cores\arduino\SoftwareSerial.h
 For SoftwareSerial modify _SS_MAX_RX_BUFF in
   Arduino\hardware\arduino\avr\libraries\SoftwareSerial\SoftwareSerial.h
*/

#include <string.h>
#include "WiFiEsp.h"
#include "WiFiEspUdp.h"
#include <IRremote.h>

const int led_pin =  12;      // the number of the LED pin
#define HUMAX_POWER 0x800FF   // IR code for power button
IRsend irsend;
// Note: pin 13 is IR transmitter pin on tinylab
const unsigned long max_wait = 30 * 1000; // Maximum time for Humax to boot, in milliseconds

// To specify your wifi settings here, comment out the #include below
// and modify and uncomment the two lines below that
#include "/pc-share/Programming/Arduino/sketchbook/wifi-settings.h"
//char ssid[] = "mywifi";            // your network SSID (name)
//char pass[] = "12345678";        // your network password

// Web server on port 80
WiFiEspServer server(80);
#define BUFFERSIZE 100
struct {
  WiFiEspClient client;
  char in_buf[BUFFERSIZE];
  char *hostname;
  } command;

bool setupWiFi(char *ssid, char *pass) {
  int status;     // the Wifi radio's status
  int retries = 0;
  
  // initialize serial for ESP module
  Serial1.begin(115200);
  
  // initialize ESP module
  WiFi.init(&Serial1);
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
}

void sendError(WiFiEspClient *p_client, int code, char *p_error) {
  Serial.println(p_error);
  
  p_client->print("HTTP/1.1 ");
  p_client->print(code);
  p_client->print(" ");
  p_client->println(p_error);
  p_client->println("Content-type:text/html");
  p_client->println();
  p_client->println(p_error);
  p_client->println();

  p_client->stop();
}

/* 
 *  Modern browsers can send very large requests, for which we don't have enough memory.
 *  So we ignore the requirements of the HTTP protocol and just look at the first line
 *  (up to the first CR).
 */
bool getCommand() {
  char *p_buf = command.in_buf;
  char *name_end;
  
  command.client = server.available(); // Listen for client;
  if (!command.client) return false;

  //Serial.println("New client");
  while (command.client.connected()) {
    if (command.client.available()) {
      if (p_buf - command.in_buf >= BUFFERSIZE) {
        sendError(&command.client, 413, "HTTP request exceeds buffer size");
        return false;
      }
      *p_buf = command.client.read();
    }

    // End of request line?
    if (*p_buf == '\r') {
      *p_buf = '\0'; // Terminate the buffer

      //Serial.println(command.in_buf);
      
      // Parse the request
      p_buf = command.in_buf;
      // Must be GET
      if (strncmp(p_buf, "GET ", 4) != 0) {
        sendError(&command.client, 501, "Not a GET method");
        return false;        
      }
      p_buf += 4;
      // ?AbsoluteURI - skip http:/
      if (strncmp(p_buf, "http://", 7) == 0) p_buf += 6;
      // URI should be /poweron?humax=... where ... must be hostname or IP address
      if (strncmp(p_buf, "/poweron?humax=", 15) != 0) {
        sendError(&command.client, 404, "Incorrect URI");
        return false;
      }
      p_buf += 15;
      command.hostname = p_buf;
      name_end = strchr(p_buf, ' ');
      if (name_end) *name_end = '\0'; // Terminate hostname

      return true;
    }
    
    p_buf++;
  }
  return false;
}

/*
 * Check whether Hunax PVR is running.
 * 
 * Currently this checks by trying to connect to FTP.
 * 
 */
bool humax_is_on(char *humax) {
#if 1
  // Try connecting to FTP server
  WiFiEspClient client;
  int ret;
  ret = client.connect(humax, 21);
  if (ret) client.stop();
  return ret;
#else
  // Try getting a DLNA SSDP response (not currently working)
  WiFiEspUDP Udp;
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

void startHttpResponse(WiFiEspClient *client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client->println("HTTP/1.1 200 OK");
  client->println("Content-type:text/html");
  client->println();
  
  // the content of the HTTP response follows the header:
  client->println("<html><title>Humax action response</title><body>");
}

void logToHttpResponse(WiFiEspClient *client, const char *message)
{
  client->println("<p>");
  client->println(message);
  Serial.println(message);
  client->println("</p>");
}

void finishHttpResponse(WiFiEspClient *client, const char *response)
{
  client->println("<p>");
  client->println(response);
  client->println("</p></body></html>");
  
  // The HTTP response ends with another blank line:
  client->println();
  
  // close the connection
  client->stop();
  //Serial.println("Client disconnected");
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
        server.begin(); // Note: this does not return a status
        printWifiStatus();
        digitalWrite(led_pin, LOW);
        state = idle;
      } else {
        Serial.println("Could not connect to WiFi. Waiting for retry");
        digitalWrite(led_pin, !digitalRead(led_pin)); // Toggle LED
        delay(1000);
      }
      break;
    
    case idle: //Serial.println("State: idle");
      if (getCommand()) state = check; // Change state
      break;
      
    case check: //Serial.println("State: check");
      digitalWrite(led_pin, HIGH);
      startHttpResponse(&command.client);

      Serial.print("Humax hostname = ");
      logToHttpResponse(&command.client, command.hostname);
      if (humax_is_on(command.hostname)) {
        success = true;
        state = result;
      } else {
        success = false;
        state = turn_on;
      }
      break;
      
    case turn_on: logToHttpResponse(&command.client, "State: turn_on");
      irsend.sendNEC(HUMAX_POWER, 32);
      start_wait = millis();
      state = wait;
      break;
      
    case wait: logToHttpResponse(&command.client, "State: wait");
      // Is it on?
      if (humax_is_on(command.hostname)) {
        success = true;
        state = result;
      } else {
        // Timeout?
        if (millis() - start_wait > max_wait) {
          logToHttpResponse(&command.client, "Timeout");
          success = false;
          state = result;
        } else {
          delay(1000);
        }
      }
      break;
      
    case result: //Serial.println("State: result");
      if(success) {
        finishHttpResponse(&command.client, "<b>Success:</b> Humax powered up.");
      } else {
        finishHttpResponse(&command.client, "<b>Failed:</b> Humax not responding.");
      }
      digitalWrite(led_pin, LOW);
      state = idle;
      break;
      
    default: Serial.print("BUG: invalid state ");
      Serial.println(state);
      state = init;
  }
  
  delay(1000);
}

