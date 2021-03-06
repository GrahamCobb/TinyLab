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

## HumaxPower
This project is the prototype to turn on my Humax PVR under command from another system.
When it receives the command (a web access to http://<arduino-IP>/poweron?humax=<humax-IP>) it
transmits the Humax remote control POWER button infrared code.
I plan to use this so that my Humax backups always work (not just when I am watching it so it is on).

At the moment this is being prototyped on my TinyLab.
But I hope to eventually move it to running directly on a standalone ESP8266-based board.

###Libraries

* WiFiEsp - Note this requires a newer version of the WiFiEsp library in order to allow
simultaneous inbound and outbound connections.
See my issue #60 (and pull request #61) on the github page.

* IRremote

## HumaxPowerOnThing
This project is the completed version of HumaxPower.
It turns on my Humax PVR under command from another system.
When it receives the command (a web access to http://<arduino-IP>/poweron?humax=<humax-IP>) it
transmits the Humax remote control POWER button infrared code.
I use this so that my Humax backup scripts can turn on the Humax even if I am not watching it.

This runs on the Sparkfun ESP8266 Thing (https://www.sparkfun.com/products/13231).
Unlike the prototype (above) this uses the ESP8266WebServer library and so the code had
to be refactored, but the code is now a bit simpler.

###Libraries

* ESP8266WebServer - and associated libraries.

* IRremoteESP8266 - not part of the standard ESP8266 Arduino support package but available from
https://github.com/markszabo/IRremoteESP8266
