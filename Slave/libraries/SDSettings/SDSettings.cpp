/*
  SDSettings.cpp - Library for reading custom settings of a SD Card
  Created by C Rossouw, November 10, 2016.
*/

#include "Arduino.h"
#include "SD.h"
#include "SDSettings.h"

String SDSettings::ReadIntoString(char filename[])
{
	File file = SD.open(filename, FILE_READ);
	if(file){
		//Increase timeout due to file size
		file.setTimeout(4000);
		String content = file.readString();
		file.close();
		return content;
	}
	return String();
}