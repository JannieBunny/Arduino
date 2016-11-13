# ESP8266 - MQTT, REST and all the Sensors
This project is based on the [NodeMCU](https://github.com/nodemcu/nodemcu-firmware) breakout board. The project is created on the Arduino IDE using the [ESP8266 Core](https://github.com/esp8266/Arduino)
![Alt](https://raw.githubusercontent.com/JannieBunny/Arduino/master/docs/nodemcu.jpg "NodeMCU")

## Why?

I need to store my code somewhere afterall.

### TODO

1. Move MQTT calls from main to libs

Ughhh so much to do...

### JSON SD Card Sample Contents

```json
{
  "Device": {
    "ID": 1,
    "Identity": "Bedroom Light"
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
  }
}
```