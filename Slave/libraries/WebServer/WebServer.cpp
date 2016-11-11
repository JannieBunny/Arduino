/*
  WebServer.cpp - Library for responding to web requests
  Created by C Rossouw, November 11, 2016.
*/

#include "WebServer.h"
#include "Arduino.h"
#include "ArduinoJson.h"

String Ip;
String DeviceIdentity;

int DeviceId;
int GPIOCount;

String WebServer::GetHomePageResponse(String page, byte expanderPort, 
										float celcius, float pressure, 
										float altitude, float humidity){ 
	String temp = page.substring(0); 
	temp.replace("{{friendlyName}}", DeviceIdentity);
	temp.replace("{{apiLink}}", Ip);
	temp.replace("{{celcius}}", String(celcius));
	temp.replace("{{pressure}}", String(pressure));
	temp.replace("{{altitude}}", String(altitude));
	temp.replace("{{humidity}}", String(humidity));
	for(int a=0;a<GPIOCount;a++){
		temp.replace("{{gpio" + String(a + 1) + "}}", String(bitRead(expanderPort, a)));
	}
	return temp; 
}

String WebServer::GetAPIPageResponse(String page){
	page.replace("{{host}}", Ip);
	page.replace("{{friendlyName}}", DeviceIdentity);
	return page;
}

String WebServer::GetBME280Response(float celcius, float pressure, 
									float altitude, float humidity){
	DynamicJsonBuffer jsonBuffer;
	JsonObject& response = jsonBuffer.createObject();
	response["ID"] = DeviceId;
	response["CELCIUS"] = String(celcius);
	response["PRESSURE"] = String(pressure);
	response["ALTITUDE"] = String(altitude);
	response["HUMIDITY"] = String(humidity);
	char responseString[response.measureLength()+1];
	response.printTo(responseString, response.measureLength()+1);
	return String(responseString);
 }
 
 String WebServer::GetGPIOResponse(byte expanderPort){
	DynamicJsonBuffer jsonBuffer;
	JsonObject& response = jsonBuffer.createObject();
	response["ID"] = DeviceId;
	JsonArray& nestedArray = response.createNestedArray("GPIO");
	for(int a=0;a<GPIOCount;a++){
		JsonObject& nestedObject = nestedArray.createNestedObject();
		nestedObject["PIN"] = a + 1;
		nestedObject["VALUE"] = (int)bitRead(expanderPort, a);
	}
	char responseString[response.measureLength()+1];
	response.printTo(responseString, response.measureLength()+1);
	return String(responseString);
 }