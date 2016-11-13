#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <MQTTClient.h>
#include <SparkFunBME280.h>
#include <ESP8266WebServer.h>

//Custom Libraries
#include <SDSettings.h>
#include <WebServer.h>
#include <ExpanderPort.h>
#include <MQTTActions.h>

#define MAX_GPIO 8
#define MAX_SENSORS 4

#define STATUS_PORT 10
#define EXPANDER_ADRRESS 0x20

SDSettings SDconfig;
BME280 bme280;
ESP8266WebServer server(80);
WebServer webserver;
ExpanderPort expanderPort;
MQTTActions mqttBrokerClient;
  
String json;
String homePage;
String apiDocs;

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

//Post Config
bool PostUpdate;
String POSTUsername;
String POSTPassword;

//API Config
bool APIEnabled;
String APIBaseURL;

//BME280 Sensor COnfig
bool bme280Enabled = false;
const char* BME280Sensor = "BME280";
//Endpoints
const char* Celcius;
const char* Altitude;
const char* Pressure;
const char* Humididty;
String BME280BaseTopic;
String BME280Request;

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
  
  //Check if we should post an update
  JsonObject& rest = root["Rest"];
  PostUpdate = rest["Enabled"];
  if(PostUpdate){
    POSTUsername = String((const char*)rest["Username"]);
    POSTPassword = String((const char*)rest["Password"]);
    //Config to build POST requests
    webserver.Host = String((const char*)rest["Host"]);
    webserver.Url = String((const char*)rest["Url"]);
    webserver.Port = rest["Port"];
  }

  JsonObject& api = root["Api"];
  APIEnabled = api["Enabled"];
  if(APIEnabled){
    APIBaseURL = (const char*)api["BaseUrl"];
    String gpioGet = (const char*)api["GPIOGet"];
    String gpioUpdate = (const char*)api["GPIOUpdate"];
    server.on((APIBaseURL + gpioUpdate).c_str(), HTTP_PUT, updateGPIO);
    server.on((APIBaseURL + gpioGet).c_str(), HTTP_GET, getGPIO);
    server.begin();
  }
  
  webserver.GPIOCount = MAX_GPIO;
  webserver.DeviceId = id;
  
  delay(10);

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

  //Loop through any sensors found in config
  JsonArray& sensors = root["Sensors"];
  for(int a=0;a<MAX_SENSORS;a++){
    JsonObject& sensor = sensors[a];
    String sensorType = String((const char*)sensor["Type"]);
    if(sensorType == String(BME280Sensor)){
      bme280.settings.I2CAddress = 0x77;
      bme280.settings.runMode = 3;
      bme280.settings.tStandby = 2;
      bme280.settings.filter = 0;
      bme280.settings.tempOverSample = 1;
      bme280.settings.pressOverSample = 1;
      bme280.settings.humidOverSample = 1;
      bme280.begin();
      JsonObject& configUrls = sensor["ConfigUrls"];
      BME280BaseTopic = String((const char*)configUrls["BaseTopic"]);
      BME280Request = String((const char*)configUrls["Request"]);
      if(APIEnabled){
         server.on((APIBaseURL + BME280BaseTopic).c_str() , HTTP_GET, getBME280);
      }
      mqttBrokerClient.Subscribe(BaseTopic + BME280BaseTopic + BME280Request);
      bme280Enabled = true;
    }
  }
}

void loop()
{
  //Reconnect WiFI if it drops
  while (WiFi.status() != WL_CONNECTED) {
    flashStatus(50);
  }

  if(publishToMQTT){
    delay(10);
    if(!mqttBrokerClient.Connected()){
      connectMQTT(MQTTUsername, MQTTPassword);
      mqttBrokerClient.Subscribe(BaseTopic + GPIOUpdateTopic);
      mqttBrokerClient.Subscribe(BaseTopic + GPIORequestTopic);
      if(bme280Enabled){
        mqttBrokerClient.Subscribe(BaseTopic + BME280BaseTopic + BME280Request);
      }
    }
    mqttBrokerClient.Loop();
  }
  
  //Handle a web client
  server.handleClient();
  
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
      webserver.CreateUpdateResponse(expanderPort.ChangedPorts, expanderPort.LastReading, MQTTPOLL);
    if(publishToMQTT){
      mqttBrokerClient.Publish(BaseTopic + GPIOGetTopic, response);
    }
    if(PostUpdate){
      webserver.SendGPIOUpdate(response, POSTUsername, POSTPassword);
    }
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
  if(topic == (BaseTopic + GPIOUpdateTopic) && id == requestId){
     updateGPIOPins(root);
  }
  if(topic == (BaseTopic + GPIORequestTopic) && id == requestId){
     mqttBrokerClient.Publish(BaseTopic + GPIOGetTopic, webserver.GetGPIOResponse(expanderPort.LastReading, MQTTREQUEST));
  }
  if(topic == (BaseTopic + BME280BaseTopic + BME280Request) && id == requestId){
    String response = webserver.GetBME280Response(bme280.readTempC(), bme280.readFloatPressure(),
                            bme280.readFloatAltitudeMeters(), bme280.readFloatHumidity());
    mqttBrokerClient.Publish(BaseTopic + BME280BaseTopic, response);
  }
}

void flagGPIOUpdated(){
    expanderPort.ResetChangeFlag();
}

void updateGPIO(){
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    updateGPIOPins(root);
    getGPIO();
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

//GET calls
void getGPIO(){
  String response = webserver.GetGPIOResponse(expanderPort.LastReading, MQTTREQUEST);
  server.send(200, "application/json", response);
}

//BME280
void getBME280(){
  String response = webserver.GetBME280Response(bme280.readTempC(), bme280.readFloatPressure(),
                              bme280.readFloatAltitudeMeters(), bme280.readFloatHumidity());
  server.send(200, "application/json", response);
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

