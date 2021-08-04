# LoRa Node with WioTerminal-IoT Smart Garden Monitor For PlatformIO

## Introduction

The IoT Smart Garden Monitor is used the Wio Terminal Chassis-LoRa-E5 and GNSS as an IoT device, it is basically sending a frame to the gateway and then transfer to the server(Uplink), in this case, I can bunch other data with the frame to upload, such as GPS, temperature and humidity, you also can bring other sensor data as you want. After the ACK obtain the response(Downlink) back to the LoRa device, meanwhile, the connection status will display on the Wio terminal, which means the message is passed to the backend service and then you can view the data on TheThingsNetwork platform, you also can use other platforms, but the premise is that platform can support the Wio Terminal Chassis-LoRa-E5 and GNSS. 

  <div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/%E5%B8%A6%E4%BC%A0%E6%84%9F%E5%99%A8%E5%9C%BA%E6%99%AF%E5%9B%BE.jpg"/></div>

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

- [**Wio Terminal Chassis - LoRa-E5 and GNSS**](https://www.seeedstudio.com/Wio-Terminal-p-4509.html)


## Usage


This project bases using on the Arduino, you need to download the Arduino IDE and some library on your PC, if you are first time use the Wio terminal, here is the [**Wio terminal instruction**](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/).

requite library:
- [**Seeed_Arduino_LCD**](https://github.com/Seeed-Studio/Seeed_Arduino_LCD)
- [**Seeed_Arduino_SFUD**](https://github.com/Seeed-Studio/Seeed_Arduino_SFUD)
- [**TinyGPS**](https://github.com/mikalhart/TinyGPSPlus)
- [**Grove_Temperature_And_Humidity_Sensor**](https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor)

### TheThingsNetwork Console Configuration Setup

In this project, I test the LoRa tester on [**TheThingsNetwork**](https://www.thethingsnetwork.org) platform, the instuction as below:

Step 1: Load into [**TTN website**](https://www.thethingsnetwork.org) and create your account, then go to gateways start to set up your device.

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_gataway1.png"/></div>

Step 2: Add the gateway device:
- Owner
- Gteway ID
- Gateway EUI
- Gateway name

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_gateway2.png"/></div>

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_gateway3.png"/></div>

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_gateway4.png"/></div>

Step 3: Add Application:
- Owner
- Application ID
- Application name

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_applications.png"/></div>

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/application2.png"/></div>

Step 4：Add the LoRa node:
- Brand (Select Sense CAP)
- Model (Select LoRa-E5)
- Hardware Ver (Defult)
- Firmware Ver (Defult)
- Profile (The Region is according to your location)
- Frequency plan
- AppEUI
- DEVEUI
- AppKey
- End Device ID

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_device1.png"/></div>

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_device4.png"/></div>

Step 5: Add the code for decode the data:

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_decode1.png"/></div>

```CPP
function Decoder(bytes, port) {
 
  var decoded = {};
  if (port === 8) {
    decoded.temp = bytes[0] <<8 | bytes[1];
    decoded.humi = bytes[2] <<8 | bytes[3];
    decoded.latitude   = (bytes[7] | (bytes[6]<<8) | (bytes[5]<<16)  | (bytes[4]<<24)) /1000000;
    decoded.longitude  = (bytes[11] | (bytes[10]<<8) | (bytes[9]<<16)  | (bytes[8]<<24)) /1000000;
    decoded.altitude   = (bytes[15] | (bytes[14]<<8) | (bytes[13]<<16) | (bytes[12]<<24))/100;
    decoded.satellites = bytes[16];
  }
 
  return decoded;
}
 
```

Step 5: Cheack the result on TheThingsNetwork

Go to the geteway, then click "Live data".

<div align=center><img width = 500 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TTN_dataTEMP1.png"/></div>


### Note

When you upload the code, please selecte slave mode.
    <div align=center><img width = 400 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/ROLA.png"/></div>

Each LoRa device has a unique serial number, after you connect the LoRa device to the Wio terminal then there will display the deveui, appeui and appkey on the first page, you need to fill the LoRa ID and gateway ID in server.

<div align=center><img width = 600 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/temp_ID.png"/></div>

There are displaying temperature and humidity, also there has the GPS function, but it is not recommended for using in enclosed space in case effect collect satellites.

<div align=center><img width = 600 src="https://files.seeedstudio.com/wiki/LoRa_WioTerminal/TEMP_GPS_DATA.png"/></div>