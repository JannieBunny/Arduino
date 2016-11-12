/*
  MQTTActions.h - Library for handling MQTT Actions
  Created by C Rossouw, November 12, 2016.
*/
#ifndef MQTTAction_h
#define MQTTAction_h

#include "MQTTClient.h"
#include "ESP8266WiFi.h"

class MQTTActions
{
  public:
	String ClientName;
	String Broker;
	int BrokerPort;
	void Begin();
	void Connect(void (*SDstatusFunction)(int));
	void Loop();
	void Subscribe(String topic);
	void Unsubscribe(String topic);
	void Publish(String topic, String content);
	bool Connected();
  private:
	WiFiClient mqttPipe;
	MQTTClient mqttClient;
};

#endif