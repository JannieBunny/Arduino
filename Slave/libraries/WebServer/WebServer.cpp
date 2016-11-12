/*
  WebServer.cpp - Library for responding to web requests
  Created by C Rossouw, November 11, 2016.
*/

#include "WebServer.h"
#include "Arduino.h"
#include "ArduinoJson.h"
#include "ESP8266WebServer.h"

String Ip;
String DeviceIdentity;

int DeviceId;
int GPIOCount;

String Host;
String Url;
int Port;

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
 
 void WebServer::SendGPIOUpdate(String response){
	WiFiClient client;
	Serial.println("Connecting to POST server");
	if (client.connect(Host.c_str(), Port)) {
		//POST Headers
		String hostParam = "Host: ";
		String contentType = "POST ";
		String httpType = " HTTP/1.1";
		client.println(contentType + Url + httpType);
		client.println(hostParam + Host);
		client.println("User-Agent: Arduino/1.0");
		client.println("Cache-Control: no-cache");
		client.println("Content-Type: application/json");
		client.println("Connection: close");
		client.print("Content-Length: ");
		client.println(response.length());
		client.println();
		client.println(response);
		Serial.println("Response sent to POST server");
	}
	else{
		Serial.println("Failed to Connect to POST server");
	}
 }