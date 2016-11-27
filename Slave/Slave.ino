#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <MQTTClient.h>

//Custom Libraries
#include <SDSettings.h>
#include <GPIOResponses.h>
#include <ExpanderPort.h>
#include <MQTTActions.h>
#include <BME280.h>

#define MAX_GPIO 8
#define MAX_SENSORS 4

#define STATUS_PORT 10
#define EXPANDER_ADRRESS 0x20

SDSettings SDconfig;
GPIOResponses gpioResponses;
ExpanderPort expanderPort;
MQTTActions mqttBrokerClient;
BME280Sensor bme280Sensor;
  
String json;

int id;
byte count;

//MQTT Config
bool publishToMQTT;
String GPIOUpdateTopic;
String GPIOGetTopic;
String GPIORequestTopic;
String BaseTopic;
String MQTTUsername;
String MQTTPassword;

//MQTT Base Config
const char* MQTTREQUEST = "REQUEST";
const char* MQTTPOLL = "INTERRUPT";

void setup()
{ 
  //Debug
  Serial.begin(9600);
  delay(10);

  pinMode(STATUS_PORT, OUTPUT);
  digitalWrite(STATUS_PORT, HIGH);

  //Clear the Expander of any latched ports
  expanderPort.Address = EXPANDER_ADRRESS;
  expanderPort.GPIOCount_Expander = MAX_GPIO;
  expanderPort.Clear();

  //Init SD Card
  SDconfig.Begin(16, flashStatus);
  json = SDconfig.ReadIntoString("config.slv", 1000);
  
  //Read SDconfig from File
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json); 
  
  //Identity
  const char* identity = root["Device"]["Identity"];
  id = root["Device"]["ID"];

  //Connect to WiFi
  const char* ssid = root["WiFi"]["Ssid"];
  const char* password = root["WiFi"]["Password"];

  //https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF); 
  WiFi.mode(WIFI_STA);
  WiFi.hostname(identity);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    flashStatus(50);
  }
  
  delay(10);

  gpioResponses.GPIODeviceId = id;
  gpioResponses.GPIODeviceFriendlyName = String(identity);
  gpioResponses.GPIOCount = MAX_GPIO;
  publishToMQTT = root["MQTT"]["Enabled"];
  if(publishToMQTT){
    delay(10);
    JsonObject& mqttSettings = root["MQTT"];
    MQTTUsername = String((const char*)mqttSettings["Username"]);
    MQTTPassword = String((const char*)mqttSettings["Password"]);
    
    BaseTopic = String((const char*)mqttSettings["BaseTopic"]);
    GPIOUpdateTopic = String((const char*)mqttSettings["GPIOUpdate"]);
    GPIOGetTopic = String((const char*)mqttSettings["GPIOStatus"]);
    GPIORequestTopic = String((const char*)mqttSettings["GPIORequest"]);
    
    mqttBrokerClient.ClientName = String(id);
    mqttBrokerClient.Broker = String((const char*)mqttSettings["Broker"]);
    mqttBrokerClient.BrokerPort = mqttSettings["Port"];
    mqttBrokerClient.Begin();
    connectMQTT(MQTTUsername, MQTTPassword);

    //Subscribe
    mqttBrokerClient.Subscribe(BaseTopic + GPIOUpdateTopic);
    mqttBrokerClient.Subscribe(BaseTopic + GPIORequestTopic);
  }

  //Detect any sensors
  bme280Sensor.BME280DeviceId = id;
  bme280Sensor.BME280FriendlyName = String(identity);
  bme280Sensor.Begin("/bme280", "/request");
  if(bme280Sensor.BME280Detected){
      mqttBrokerClient.Subscribe(BaseTopic + bme280Sensor.BME280Topic + bme280Sensor.BME280RequestTopic);
  }
}

void loop()
{
  //Reconnect WiFI if it drops
  while (WiFi.status() != WL_CONNECTED) {
    flashStatus(50);
  }

  delay(10);
  if(!mqttBrokerClient.Connected()){
    connectMQTT(MQTTUsername, MQTTPassword);
    mqttBrokerClient.Subscribe(BaseTopic + GPIOUpdateTopic);
    mqttBrokerClient.Subscribe(BaseTopic + GPIORequestTopic);
    if(bme280Sensor.BME280Detected){
        mqttBrokerClient.Subscribe(BaseTopic + bme280Sensor.BME280Topic + bme280Sensor.BME280RequestTopic);
    }
  }
  mqttBrokerClient.Loop();
  
  count++;
  bool changed = false;

  //Trigger every one second
  if(count == 10){
    expanderPort.GetStatus();
    count = 0;
  }

  //Update host
  if(expanderPort.Changed){ 
    String response =
      gpioResponses.CreateUpdateResponse(expanderPort.ChangedPorts, expanderPort.LastReading, MQTTPOLL);
    mqttBrokerClient.Publish(BaseTopic + GPIOGetTopic, response);
    flagGPIOUpdated();
  }
  else{
    delay(100); 
  }
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  int requestId = root["ID"];
  //Was the message intended for me?
  if(id == requestId){
    if(topic == (BaseTopic + GPIOUpdateTopic)){
       updateGPIOPins(root);
    }
    if(topic == (BaseTopic + GPIORequestTopic)){
       mqttBrokerClient.Publish(BaseTopic + GPIOGetTopic, gpioResponses.GetGPIOResponse(expanderPort.LastReading, MQTTREQUEST));
    }
  
    //BME280 Sensor
    if(bme280Sensor.BME280Detected && bme280Sensor.ValidateTopic(BaseTopic, topic)){
      String response = bme280Sensor.Get();
      mqttBrokerClient.Publish(BaseTopic + bme280Sensor.BME280Topic, response);
    }
  }
}

void flagGPIOUpdated(){
    expanderPort.ResetChangeFlag();
}

void updateGPIOPins(JsonObject& root){
    byte newStatus = expanderPort.LastReading;
    for(int a=0;a<MAX_GPIO;a++){
      int pin = root["GPIO"][a]["Pin"];
      int pinValue = root["GPIO"][a]["Value"];
      if(pin != 0){
          bitWrite(newStatus, pin-1, pinValue);
      }
    }
    expanderPort.SetStatus(newStatus);
}

void flashStatus(int ms){
  digitalWrite(STATUS_PORT, HIGH);
  delay(ms);
  digitalWrite(STATUS_PORT, LOW);
  delay(ms);
}

void connectMQTT(String username, String password){
  if(username != "" && password != ""){
    mqttBrokerClient.Connect(flashStatus, username, password);
  }
  else{
    mqttBrokerClient.Connect(flashStatus);
  }
}

