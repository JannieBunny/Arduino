# ESP8266 - MQTT, REST and all the Sensors
This project is based on the [NodeMCU](https://github.com/nodemcu/nodemcu-firmware) breakout board. The project is created on the Arduino IDE using the [ESP8266 Core](https://github.com/esp8266/Arduino)
![Alt](https://raw.githubusercontent.com/JannieBunny/Arduino/master/docs/nodemcu.jpg "NodeMCU")

## Why?

I need to store my code somewhere afterall.

### TODO

1. Clean up custom libs
2. Move MQTT calls from main to libs
3. Split webserver lib into API and actual Webserver parts
4. Create BME280 lib and merge it with sparkfun lib

Ughhh so much to do...

### JSON SD Card Sample Contents

```json
{
  "Device": {
    "ID": 1,
    "Identity": "Bedroom Light"
  },
  "Rest": {
    "Enabled": true,
    "Host": "172.24.1.1",
    "Port": 80,
    "Url": "/api/device",
    "Username": "",
    "Password": ""
  },
  "Api": {
    "Enabled": true,
    "BaseUrl": "/api",
    "GPIOGet": "/get",
    "GPIOUpdate": "/update"
  },
  "MQTT": {
    "Enabled": true,
    "Broker": "172.24.1.1",
    "Port": 1883,
    "Username": "username",
    "Password": "password",
    "BaseTopic": "/device/1",
    "GPIOStatus": "/status",
    "GPIORequest": "/request",
    "GPIOUpdate": "/update"
  },
  "WiFi": {
    "Ssid": "ssid",
    "Password": "password"
  },
  "Sensors": [
    {
      "Type": "BME280",
      "ConfigUrls": {
        "BaseTopic": "/bme280",
        "Request": "/request"
      }
    }
  ]
}
```