/*
  GPIOResponses.cpp - Library for responding to web requests
  Created by C Rossouw, November 11, 2016.
*/

#include "GPIOResponses.h"
#include "Arduino.h"
#include "ArduinoJson.h"

int GPIODeviceId;
int GPIOCount;
 
 String GPIOResponses::GetGPIOResponse(byte expanderPort, String requestType){
	DynamicJsonBuffer jsonBuffer;
	JsonObject& response = jsonBuffer.createObject();
	response["ID"] = GPIODeviceId;
	response["Type"] = requestType;
	JsonArray& nestedArray = response.createNestedArray("GPIO");
	for(int a=0;a<GPIOCount;a++){
		JsonObject& nestedObject = nestedArray.createNestedObject();
		nestedObject["Pin"] = a + 1;
		nestedObject["Value"] = (int)bitRead(expanderPort, a);
	}
	char responseString[response.measureLength()+1];
	response.printTo(responseString, response.measureLength()+1);
	return String(responseString);
 }
 
 String GPIOResponses::CreateUpdateResponse(byte changedPorts, byte lastReading, String requestType){
	DynamicJsonBuffer jsonBuffer;
    JsonObject& response = jsonBuffer.createObject();
    response["ID"] = GPIODeviceId;
	response["Type"] = requestType;
    JsonArray& gpioArray = response.createNestedArray("GPIO");
    for(int a=0;a<GPIOCount;a++){
      if(bitRead(changedPorts, a)){
        JsonObject& gpioObject = jsonBuffer.createObject();
        gpioObject["Pin"] = 1 + a;
        gpioObject["Value"] = bitRead(lastReading, a);
        gpioArray.add(gpioObject);
      }
    }
    int responseBufferSize = response.measureLength()+1;
    char responseBuffer[responseBufferSize];
    response.printTo(responseBuffer, responseBufferSize);
    return String(responseBuffer);
 }