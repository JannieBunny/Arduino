/*
  WebServer.h - Library for responding to web requests
  Created by C Rossouw, November 11, 2016.
*/
#ifndef WebServer_h
#define WebServer_h

#include "Arduino.h"
#include "ArduinoJson.h"

class WebServer
{
  public:
  	String Ip;
  	String DeviceIdentity;
	
	int DeviceId;
	int GPIOCount;
	
	String GetHomePageResponse(String page, 
								byte expanderPort, 
								float celcius, 
								float pressure, 
								float altitude, 
								float humidity);
	String GetAPIPageResponse(String page);
	String GetBME280Response(float celcius, float pressure, 
							 float altitude, float humidity);
    String GetGPIOResponse(byte expanderPort);
};

#endif