/*
  SDSettings.h - Library for reading custom settings of a SD Card
  Created by C Rossouw, November 10, 2016.
*/
#ifndef SDSettings_h
#define SDSettings_h

#include "Arduino.h"
#include "SD.h"
#include "SPI.h"

class SDSettings
{
  public:
	void Begin(int selectPin, void (*SDstatusFunction)(int));
	String ReadIntoString(char filename[], int timeout);
};

#endif