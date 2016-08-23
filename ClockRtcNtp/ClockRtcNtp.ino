/*
 WiFiEsp example: UdpNTPClient

 Get the time from a Network Time Protocol (NTP) time server.
 Demonstrates use of UDP to send and receive data packets
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 NOTE: The serial buffer size must be larger than 36 + packet size
 In this example we use an UDP packet of 48 bytes so the buffer must be
 at least 36+48=84 bytes that exceeds the default buffer size (64).
 You must modify the serial buffer size to 128
 For HardwareSerial modify _SS_MAX_RX_BUFF in
   Arduino\hardware\arduino\avr\cores\arduino\SoftwareSerial.h
 For SoftwareSerial modify _SS_MAX_RX_BUFF in
   Arduino\hardware\arduino\avr\libraries\SoftwareSerial\SoftwareSerial.h
*/

#include "WiFiEsp.h"
#include "WiFiEspUdp.h"
#include <Wire.h>
#include <LiquidTWI2.h>
#include <Time.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <LedControl.h>

const int button_pin = 9;     // the number of the pushbutton pin
const int led_pin =  13;      // the number of the LED pin

// To specify your wifi settings here, comment out the #include below
// and modify and uncomment the two lines below that
#include "../../wifi-settings.h"
//char ssid[] = "mywifi";            // your network SSID (name)
//char pass[] = "12345678";        // your network password

//char timeServer[] = "time.nist.gov";  // NTP server
char timeServer[] = "192.168.1.1";  // NTP server

unsigned int localPort = 2390;        // local port to listen for UDP packets
#define NTP_PACKET_SIZE 48
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiEspUDP Udp;

// Connect to LCD via i2c, address 0x20 
LiquidTWI2 lcd(0x20);

// 7-segment
LedControl lc = LedControl(10, 12, 11, 1);

void setup()
{
  delay(5000);
  // initialize serial for debugging
  Serial.begin(115200);
  
  // initialize the LED pin as an output:
  pinMode(led_pin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(button_pin, INPUT);

  // set the LCD type
  lcd.setMCPType(LTI_TYPE_MCP23008); 
    // set up the LCD's number of rows and columns:
  lcd.begin(16, 2);
  // turn on LCD backlight
  lcd.setBacklight(HIGH);

  // Set up RTC as time provider
  setSyncProvider(RTC.get);
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else {
     Serial.print("RTC has set the system time: ");
     Serial.print(year(), DEC);
     Serial.print('-');
     Serial.print(month(), DEC);
     Serial.print('-');
     Serial.print(day(), DEC);
     Serial.print(' ');
     print2Digits(hour());
     Serial.print(':');
     print2Digits(minute());
     Serial.print(':');
     print2Digits(second());
     Serial.println();
  }

  //7-segment
  lc.shutdown(0,false); // Turn it on
  lc.setIntensity(0,8); //Medium brightness
  lc.clearDisplay(0);
      
}

bool setupWiFi(char *ssid, char *pass) {
  int status;     // the Wifi radio's status
  
  // initialize serial for ESP module
  Serial1.begin(115200);
  
  // initialize ESP module
  WiFi.init(&Serial1);
  status = WiFi.status();

  // check for the presence of the shield
  if (status == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // you're connected now, so print out the data
  Serial.println("You're connected to the network");

  return true;
}

void getNTPtime(char *ntpServer) {
  setupWiFi(ssid, pass);

  Udp.begin(localPort);
  
  sendNTPpacket(ntpServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  long startMillis = millis();
  do {if (Udp.parsePacket()) break;} while(millis() - startMillis < 5000);

  if (Udp.parsePacket() > 0) {
    Serial.print("packet received: ");
    // We've received a packet, read the data from it into the buffer
    int len = Udp.read(packetBuffer, NTP_PACKET_SIZE);
    if (len >= 44) {
      Serial.println(len);
    
      // the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:

      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      //  this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      Serial.print("Seconds since Jan 1 1900 = ");
      Serial.println(secsSince1900);

      // now convert NTP time into everyday time:
      Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      time_t epoch = secsSince1900 - seventyYears;
      // print Unix time:
      Serial.println(epoch);

      Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
      Serial.print(year(epoch), DEC);
      Serial.print('-');
      Serial.print(month(epoch), DEC);
      Serial.print('-');
      Serial.print(day(epoch), DEC);
      Serial.print(' ');
      print2Digits(hour(epoch));
      Serial.print(':');
      print2Digits(minute(epoch));
      Serial.print(':');
      print2Digits(second(epoch));
      Serial.print("\r\n");

      // Set the RTC and system time
      RTC.set(epoch);
      setTime(epoch);
    } else {
      Serial.print("NTP response too short: ");
      Serial.print(len);
    }
  }
  WiFi.reset();
}

void print2Digits(int n) {
  if (n < 10) Serial.print('0');
  Serial.print(n, DEC);
}
  
void loop()
{
  int button_state = 0;

  // LCD
  lcd.setCursor(0,0); lcd.print("S1->NTP");
  lcd.setCursor(0,1);
  lcd.print(year(), DEC);
  lcd.print('-');
  lcd.print(month(), DEC);
  lcd.print('-');
  lcd.print(day(), DEC);
  lcd.print(' ');
  lcd.print(hour(), DEC);
  lcd.print(':');
  lcd.print(minute(), DEC);
  lcd.print(':');
  lcd.print(second(), DEC);

  //7-segment
  lc.setDigit(0, 0, hour() / 10, (second() >> 3) & 1);
  lc.setDigit(0, 1, hour() % 10, (second() >> 2) & 1);
  lc.setDigit(0, 2, minute() / 10, (second() >> 1) & 1);
  lc.setDigit(0, 3, minute() % 10, second() & 1);

  // read the state of the pushbutton value:
  button_state = digitalRead(button_pin);
  if (button_state == LOW) {
    // turn LED on:
    digitalWrite(led_pin, HIGH);
    lcd.setCursor(0,0); lcd.print("Fetching time");
    // get NTP time
    getNTPtime(timeServer);
    // wait for button to be released
    do button_state = digitalRead(button_pin); while (button_state == LOW);
    // turn LED off:
    digitalWrite(led_pin, LOW);
    lcd.clear();
  }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(char *ntpSrv)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)

  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(ntpSrv, 123); //NTP requests are to port 123

  Udp.write(packetBuffer, NTP_PACKET_SIZE);

  Udp.endPacket();
}

