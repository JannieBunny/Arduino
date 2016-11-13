/*
  BME280.cpp - BME280 Sensor
  Created by C Rossouw, November 11, 2016.
*/

#include "BME280.h"
#include "Arduino.h"
#include "ArduinoJson.h"
#include "Wire.h"
#include "SparkFunBME280.h"

int BME280DeviceId;
BME280 bme280_Interface;
bool BME280Detected;
String BME280Topic;
String BME280RequestTopic;

void BME280Sensor::Begin(String topic, String requestTopic){
	bme280_Interface.settings.I2CAddress = 0x77;
	bme280_Interface.settings.runMode = 3;
	bme280_Interface.settings.tStandby = 2;
	bme280_Interface.settings.filter = 0;
	bme280_Interface.settings.tempOverSample = 1;
	bme280_Interface.settings.pressOverSample = 1;
	bme280_Interface.settings.humidOverSample = 1;
	if(bme280_Interface.begin() == 0x60){
	  BME280Detected = true;
	  BME280Topic = topic;
	  BME280RequestTopic = requestTopic;
	}
}

String BME280Sensor::Get(){
	DynamicJsonBuffer jsonBuffer;
	JsonObject& response = jsonBuffer.createObject();
	response["ID"] = BME280DeviceId;
	response["Celcius"] = String(bme280_Interface.readTempC());
	response["Pressure"] = String(bme280_Interface.readFloatPressure());
	response["Altiture"] = String(bme280_Interface.readFloatAltitudeMeters());
	response["Humididty"] = String(bme280_Interface.readFloatHumidity());
	char responseString[response.measureLength()+1];
	response.printTo(responseString, response.measureLength()+1);
	return String(responseString);
 }
 
bool BME280Sensor::ValidateTopic(String baseTopic, String receivedTopic){
	if(receivedTopic == (baseTopic + BME280Topic + BME280RequestTopic) && BME280Detected){
		return true;
	}
	return false;
}
 
 