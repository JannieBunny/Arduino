/*
  SDSettings.h - Library for reading custom settings of a SD Card
  Created by C Rossouw, November 10, 2016.
*/
#ifndef SDSettings_h
#define SDSettings_h

#include "SD.h"
#include "Arduino.h"

class SDSettings
{
  public:
	String ReadIntoString(char filename[]);
};

#endif