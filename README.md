# TinyLab

Repository for my projects for the TinyLab Arduino protoyping board.
See http://www.tinylab.cc.

### WiFi
Note: any projects which use wifi connections use a #include to include a
file "wifi-settings.h" from the sketchbook directory.
This file specifies the SSID and password for the wifi and is not
included in the Git repository for obvious reasons.

Either create the include file or comment out the #include and
specify your SSID and password in the source file itself.

## ClockRtcNtp
This project is not really useful but was to help me learn more about the TinyLab,
the Arduino IDE, various libraries and creating a real project.

It is a clock application.
It displays the current date/time on the LCD and the time on the 7-segment LED.
It uses the battery-backed real-time clock and it can set the RTC using NTP
accessed over WiFi using the ESP8266.

To update the RTC using NTP press switch S1.

### Libraries

* WiFiEsp - I found this library useful for accessing the WiFi using the ESP8266.
It is generally compatible with the standard WiFi library.

* LiquidTWI2 - to control the LCD.

* Time

* DS1307RTC - to control and use the RTC.

* LedControl - to control the 7-segment LED.