/*
  ExpanderPort.h - Library for PCF8574
  Created by C Rossouw, November 11, 2016.
*/
#ifndef ExpanderPort_h
#define ExpanderPort_h

#include "Arduino.h"
#include "Wire.h"

class ExpanderPort
{
  public:
	int GPIOCount_Expander;
	bool Changed;
	byte ChangedPorts;
	byte LastReading;
	byte Address;
	byte GetStatus();
	void SetStatus(byte value);
	void Clear();
	void ResetChangeFlag();
};

#endif