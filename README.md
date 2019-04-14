# MH-Z19B infrared co2 sensor

This repository contains the source code for the firmware used to read the measurements taken by a MH-Z19B infrared co2 sensor and publish them to our MQTT server.

# Hardware
The row of 5 pins has the following signals on it: Vout, RX, TX, SR, HD
The row of 4 pins has the following signals on it: Vin, GND, AOT, PWM

The device can be used in two modes:
 - UART: Connect Vin, GND, RX and TX to your Arduino
 - PWM: Connect Vin, GND and PWM to your Arduino
 
# Software

The firmware provided here uses the UART mode to communicate with the sensor.
The firmware runs on an Arduino Ethernet with Wiznet 5100 chipset (using the ethernet2 library).

Once uploaded the firmware will ask you to set the parameters using the serial console. After entering the parameters the parameters will be stored to the EEPROM of the Arduino.

# Links
https://revspace.nl/MHZ19

# Where to buy the sensor?
You can get the MH-Z19B sensor cheaply on Aliexpress.
