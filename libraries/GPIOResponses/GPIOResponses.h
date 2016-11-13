/*
  GPIOResponses.h - Library for responding to web requests
  Created by C Rossouw, November 11, 2016.
*/
#ifndef GPIOResponses_h
#define GPIOResponses_h

#include "Arduino.h"
#include "ArduinoJson.h"

class GPIOResponses
{
  public:	
	int GPIODeviceId;
	int GPIOCount;
	
    String GetGPIOResponse(byte expanderPort, String requestType);
	String CreateUpdateResponse(byte changedPorts, byte lastReading, String requestType);
};

#endif