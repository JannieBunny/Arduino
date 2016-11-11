/*
  ExpanderPort.h - Library for PCF8574
  Created by C Rossouw, November 11, 2016.
*/

#include "ExpanderPort.h"
#include "Arduino.h"
#include "Wire.h"

//Public
int GPIOCount_Expander;
bool Changed;
byte ChangedPorts;
byte Address;
byte LastReading;

byte ExpanderPort::GetStatus(){ 
	Wire.requestFrom(Address, (byte)1);
	if(Wire.available())
    {
		int result = Wire.read();
		for(int a=0;a<GPIOCount_Expander;a++){
			bool lastBit = bitRead(LastReading, a);
			bool bit = bitRead(result, a);
			if(bit != lastBit){
				bitWrite(LastReading, a, bit);
				bitWrite(ChangedPorts, a, 1);
				Changed = true;
			}
		}
	}
	return LastReading;
}

void ExpanderPort::Clear(){ 
	SetStatus(0);
}

void ExpanderPort::SetStatus(byte value){
	Wire.beginTransmission(Address);
	Wire.write(value);
	Wire.endTransmission(); 
}

void ExpanderPort::ResetChangeFlag(){
	ChangedPorts = 0;
	Changed = false;
}