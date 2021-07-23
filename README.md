# Wio TerminalRoLa_GPS_TEMP

## introduction

 The Wio terminal RoLa GPS_TEMP demo basic on the RoLa tester to developed, it isn't only just sent an uplink to get the RSSI data, but also can bring with other data such temperature and humidity and GPS data to the getaway, and then show the data on the TTN website, just like an IoT device.

 ## Feature

 - LoRa device information such as DevEui, APPEui, Appkey
 - detecting the current temperature and humidity
 - GPS position reporting and the current time and satellites number.
 - Display the device and TTN connection status  

## Hardware 

This demo you will need the device list as below:

- [**WioTerminal**](https://www.seeedstudio.com/Wio-Terminal-p-4509.html)

- [**Wio Terminal Chassis - Battery (650mAh)**](https://www.seeedstudio.com/Wio-Terminal-Chassis-Battery-650mAh-p-4756.html)

- [**Grove - Temperature&Humidity Sensor (DHT11)**](https://www.seeedstudio.com/Grove-Temperature-Humidity-Sensor-DHT1-p-745.html)

- [**LoRa gateway tester**]()


## Usage

This demo is basically sending a frame to the gateway and then transfer to the server(Uplink), after that it will enter waiting for an ACK status. If the RoLa tester does not get the response, it will sent the same frame again until the number of setting. conversely, the ACK obtain the response (Downlink) back to LoRa tester, that mean the message is passed to a backend service, eventually the imformation will display on the Wio terminal screen. 

This project bases using on the Arduino, you need to download the Arduino IDE and some library on your PC, if you are first time use the Wio terminal, here is the [**Wio terminal instruction**](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/).

requite library:
- [**Seeed_Arduino_LCD**](https://github.com/Seeed-Studio/Seeed_Arduino_LCD)
- [**Seeed_Arduino_SFUD**](https://github.com/Seeed-Studio/Seeed_Arduino_SFUD)
- [**TinyGPS**](https://github.com/mikalhart/TinyGPSPlus)
- [**Grove_Temperature_And_Humidity_Sensor**](https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor)

### Note

When you upload the code, please selecte slave mode.
    <div align=center><img width = 400 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/ROLA.png"/></div>

Each LoRa device has a unique serial number, after you connect the LoRa device to the Wio terminal then there will display the deveui, appeui and appkey on the first page, you need to fill the LoRa ID and gateway ID in server.

<div align=center><img width = 600 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/temp_ID.png"/></div>

There are displaying temperature and humidity, also there has the GPS function, but it is not recommended for using in enclosed space in case effect collect satellites.

<div align=center><img width = 600 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TEMP_GPS_DATA.png"/></div>