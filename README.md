# ESP32 interface to connect Calypso BLE anemometer (Tested with Solar Portable) to Signalk

This project connects the (Ultrasonic Portable Solar wind meter)[https://calypsoinstruments.com/shop/product/ultrasonic-portable-solar-wind-meter-2?category=1#attr=62] of Calypso Intruments to a SignalK server.

It runs on a ESP32 device because it has WiFi and BLE needed for the connections. Runs on 12v and need no other connection.

At this moment ssid and password of the network are hardwired. Looking for a simple good way to configure them. probably will be from a BLE characteristic.

Configuration of SignalK server is done through MDNS so it is important that in the Server/Settings optiom mdns is on!!!.

The device asks for an acces request token so first time (or after resetting with the button down) it will send a request to the 
server that must be accepted. Other times it will not be necessary.

It writes to the signalK server in paths :

    - environment.wind.angleApparent
    - environment.wind.speedApparent
    - electrical.batteries.99.capacity.stateOfCharge 

For configuring sensor speed use the server sharacteristic sensors.wind.speed with a value of 1, 4 or 8 Hz
For using or not the other sensors use sensors.wind.sensors with a value of 0 or 1

If you don't preten d to modify them best i set them fixed in base values.

The device uses :

    - GPIO 17 as WiFi LED
    - GPIO 16 as BLE LED
    - GPIO 26 as Force MDNS

GPIO 26 is pulled up. Put the switch between GPIO 26 and GND

## Hardware

This project has been tested in an ESP32 MINI D1 just because there was one on the shelf. If using another device check if pins are OK and modify them as necessary at the beginning og main.cpp. Also configure your ssi and password. If you change the name of the wind meter change ULTRASONIC in main.cpp to the name you put. It is used to get it from the advertised data.

You may power the device as you want. I used an LD33V linear regulator and 2 10uF capacitors just because I had them available.

LED's start on when not connected and are off when connections are full (WIfi Connected, Server found and connected and identified correctly, Wind Meter BLE connected and services and characteristics found)

Green LED (GPIO 17) is used for WiFi
Blue LED (GPIO 16) is used for BLE. It blinks once for every data received.
