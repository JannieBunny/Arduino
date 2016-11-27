/*
  BME280.h - BME280 Sensor
  Created by C Rossouw, November 11, 2016.
*/
#ifndef BME280_h
#define BME280_h

#include "Arduino.h"
#include "ArduinoJson.h"
#include "Wire.h"
#include "SparkFunBME280.h"

class BME280Sensor
{
  public:
	int BME280DeviceId;
	bool BME280Detected;
	String BME280Topic;
	String BME280RequestTopic;
	String BME280FriendlyName;
	void Begin(String topic, String requestTopic);
	String Get();
	bool ValidateTopic(String baseTopic, String receivedTopic);
  private:
	BME280 bme280_Interface;
};

#endif