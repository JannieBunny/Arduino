/*
  SDSettings.cpp - Library for reading custom settings of a SD Card
  Created by C Rossouw, November 10, 2016.
*/

#include "SDSettings.h"
#include "Arduino.h"
#include "SD.h"
#include "SPI.h"

void SDSettings::Begin(int selectPin, void (*SDstatusFunction)(int)){
	if(!SD.begin(selectPin)){
		while(true){
			(*SDstatusFunction)(250);
		}
	}
}

String SDSettings::ReadIntoString(char filename[], int timeout)
{
	File file = SD.open(filename, FILE_READ);
	if(file){
		file.setTimeout(timeout);
		String content = file.readString();
		file.close();
		return content;
	}
	return String();
}