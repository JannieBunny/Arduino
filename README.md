# ESP8266 - MQTT, REST and all the Sensors
This project is based on the [NodeMCU](https://github.com/nodemcu/nodemcu-firmware) breakout board. The project is created on the Arduino IDE using the [ESP8266 Core](https://github.com/esp8266/Arduino)
![Alt](https://raw.githubusercontent.com/JannieBunny/Arduino/master/docs/nodemcu.jpg "NodeMCU")

## What does it do?

This is v1.0 of my small Home Automation Project. ESP8266 Device, coupled with a SD card, BME280 sensor and a PCF8574P expander.
The purpose is simple, learn, play and enjoy.

### JSON SD Card Sample Contents

```json
{
  "DEVICE": {
    "ID": 1,
    "IDENTITY": "Test Device"
  },
  "REST": {
    "ENABLED": true,
    "HOST": "172.24.1.1",
    "PORT": 80,
    "URL": "/api/slave"
  },
  "MQTT": {
    "ENABLED": true,
    "BROKER": "172.24.1.1",
    "PORT": 1883,
    "USERNAME": "",
    "PASSWORD": "",
    "TOPICOUT": "SLAVE/1/OUT",
    "TOPICIN": "SLAVE/1/IN"
  },
  "WIFI": {
    "SSID": "SSID",
    "PASSWORD": "PASSWORD"
  }
}
```