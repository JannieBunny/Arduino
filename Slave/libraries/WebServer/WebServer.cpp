/*
  WebServer.cpp - Library for responding to web requests
  Created by C Rossouw, November 11, 2016.
*/

#include "WebServer.h"
#include "Arduino.h"
#include "ArduinoJson.h"
#include "ESP8266WebServer.h"
#include "base64.h"

int DeviceId;
int GPIOCount;

String Host;
String Url;
int Port;

base64 encoder;


String WebServer::GetBME280Response(float celcius, float pressure, 
									float altitude, float humidity){
	DynamicJsonBuffer jsonBuffer;
	JsonObject& response = jsonBuffer.createObject();
	response["ID"] = DeviceId;
	response["Celcius"] = String(celcius);
	response["Pressure"] = String(pressure);
	response["Altiture"] = String(altitude);
	response["Humididty"] = String(humidity);
	char responseString[response.measureLength()+1];
	response.printTo(responseString, response.measureLength()+1);
	return String(responseString);
 }
 
 String WebServer::GetGPIOResponse(byte expanderPort, String requestType){
	DynamicJsonBuffer jsonBuffer;
	JsonObject& response = jsonBuffer.createObject();
	response["ID"] = DeviceId;
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
 
 void WebServer::SendGPIOUpdate(String response, String username, String password){
	WiFiClient client;
	Serial.println("Connecting to POST server");
	if (client.connect(Host.c_str(), Port)) {
		//POST Headers
		String hostParam = "Host: ";
		String contentType = "POST ";
		String httpType = " HTTP/1.1";
		client.println(contentType + Url + httpType);
		client.println(hostParam + Host);
		client.println("User-Agent: ESP8266/1.0");
		if(username != "" && password != ""){
			String auth = encoder.encode(username + String(":") + password);
			client.println("Authorization: Basic " + auth);	
		}
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
 
 String WebServer::CreateUpdateResponse(byte changedPorts, byte lastReading, String requestType){
	DynamicJsonBuffer jsonBuffer;
    JsonObject& response = jsonBuffer.createObject();
    response["ID"] = DeviceId;
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