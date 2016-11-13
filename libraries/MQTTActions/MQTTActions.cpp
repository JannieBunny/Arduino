/*
  MQTTActions.h - Library for handling MQTT Actions
  Created by C Rossouw, November 12, 2016.
*/

#include "MQTTActions.h"
#include "MQTTClient.h"
#include "ESP8266WiFi.h"

//Public
String ClientName;
String Broker;
int BrokerPort;

//Private
WiFiClient mqttPipe;
MQTTClient mqttClient;
	
void MQTTActions::Begin(){ 
	mqttClient.begin(Broker.c_str(), BrokerPort, mqttPipe);
}

void MQTTActions::Connect(void (*statusFunction)(int)){
	while(!mqttClient.connected()){
		mqttClient.connect(ClientName.c_str());
		(*statusFunction)(500);
	}
}

void MQTTActions::Connect(void (*statusFunction)(int), String username, String password){
	while(!mqttClient.connected()){
		mqttClient.connect(ClientName.c_str(), username.c_str(), password.c_str());
		(*statusFunction)(500);
	}
}

void MQTTActions::Loop(){
	mqttClient.loop();
}

void MQTTActions::Subscribe(String topic){
	mqttClient.subscribe(topic.c_str());
}

void MQTTActions::Unsubscribe(String topic){
	mqttClient.unsubscribe(topic.c_str());
}

bool MQTTActions::Connected(){
	return mqttClient.connected();
}

void MQTTActions::Publish(String topic, String content){
	mqttClient.publish(topic, content);
}