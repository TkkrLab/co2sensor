# MH-Z19B infrared co2 sensor

We've connected a MH-Z19B co2 sensor to our MQTT network. This allows us to monitor the co2 concentration in our space.

The project itself is fairly simple: we just connected the serial port of the co2 sensor to an Arduino Ethernet and used the software-serial library to communicate with the sensor. The Arduino ethernet then uses the Adafruit MQTT library to send the values read from the sensor to the MQTT network.

# More information


# Links
https://revspace.nl/MHZ19

# Where to buy the sensor?
You can get the MH-Z19B sensor cheaply on Aliexpress.
