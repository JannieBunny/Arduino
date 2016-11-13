/*
  WebServer.h - Library for responding to web requests
  Created by C Rossouw, November 11, 2016.
*/
#ifndef WebServer_h
#define WebServer_h

#include "Arduino.h"
#include "ArduinoJson.h"
#include "ESP8266WebServer.h"
#include "base64.h"

class WebServer
{
  public:
	String Host;
	String Url;
	int Port;
	
	int DeviceId;
	int GPIOCount;
	
	String GetBME280Response(float celcius, float pressure, 
							 float altitude, float humidity);
    String GetGPIOResponse(byte expanderPort, String requestType);
	String CreateUpdateResponse(byte changedPorts, byte lastReading, String requestType);
	void SendGPIOUpdate(String response, String username, String password);
  private:
	base64 encoder;
};

#endif