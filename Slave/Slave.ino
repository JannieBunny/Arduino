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
int count;
bool postUpdate;
bool publishToMQTT;
String GPIOUpdateSubscription;
String GPIOGetSubscription;
String PublishTopic;

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
  apiDocs = SDconfig.ReadIntoString("api.con", 3500);
  homePage = SDconfig.ReadIntoString("home.slv", 3500);
  json = SDconfig.ReadIntoString("config.slv", 1000);
  
  //Read SDconfig from File
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json); 
  
  //Identity
  const char* identity = root["DEVICE"]["IDENTITY"];
  id = root["DEVICE"]["ID"];

  //Connect to WiFi
  const char* ssid = root["WIFI"]["SSID"];
  const char* password = root["WIFI"]["PASSWORD"];

  //https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF); 
  WiFi.mode(WIFI_STA);
  WiFi.hostname(identity);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    flashStatus(50);
  }
  
  webserver.Host = String((const char*)root["REST"]["HOST"]);
  webserver.Url = String((const char*)root["REST"]["URL"]);
  webserver.Port = root["REST"]["PORT"];
  
  webserver.DeviceIdentity = String(identity);
  webserver.Ip = ipToString(WiFi.localIP());
  webserver.GPIOCount = MAX_GPIO;
  webserver.DeviceId = id;

  server.on("/", HTTP_GET, getHome);
  server.on("/api", HTTP_GET, getAPIDocs);
  server.on("/api/GPIO/update", HTTP_PUT, updateGPIO);
  server.on("/api/GPIO/get", HTTP_GET, getGPIO);
  server.on("/api/BME280/get", HTTP_GET, getBME280);
  
  server.begin();

  bme280.settings.I2CAddress = 0x77;
  bme280.settings.runMode = 3;
  bme280.settings.tStandby = 2;
  bme280.settings.filter = 0;
  bme280.settings.tempOverSample = 1;
  bme280.settings.pressOverSample = 1;
  bme280.settings.humidOverSample = 1;
  
  delay(10);
  bme280.begin();

  publishToMQTT = root["MQTT"]["ENABLED"];
  if(publishToMQTT){
    delay(10);
    PublishTopic = String((const char*)root["MQTT"]["PUBLISH"]);
    GPIOUpdateSubscription = String((const char*)root["MQTT"]["UPDATE"]);
    GPIOGetSubscription = String((const char*)root["MQTT"]["GET"]);
    mqttBrokerClient.ClientName = String(id);
    mqttBrokerClient.Broker = String((const char*)root["MQTT"]["BROKER"]);
    mqttBrokerClient.BrokerPort = root["MQTT"]["PORT"];
    mqttBrokerClient.Begin();
    mqttBrokerClient.Connect(flashStatus);
    mqttBrokerClient.Subscribe(GPIOUpdateSubscription);
  }
  
  //Check if we should post an update
  postUpdate = root["REST"]["ENABLED"];
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
      mqttBrokerClient.Connect(flashStatus);
      mqttBrokerClient.Subscribe(GPIOUpdateSubscription);
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
      webserver.CreateUpdateResponse(expanderPort.ChangedPorts, expanderPort.LastReading);
    if(publishToMQTT){
      mqttBrokerClient.Publish(PublishTopic, response);
    }
    if(postUpdate){
      webserver.SendGPIOUpdate(response);
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
  updateGPIOPins(root);
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
      int pin = root["GPIO"][a]["PIN"];
      int pinValue = root["GPIO"][a]["VALUE"];
      if(pin != 0){
          bitWrite(newStatus, pin-1, pinValue);
      }
      expanderPort.SetStatus(newStatus);
    }
    
}

//GET calls
void getGPIO(){
  String response = webserver.GetGPIOResponse(expanderPort.LastReading);
  server.send(200, "application/json", response);
}

void getBME280(){
  String response = webserver.GetBME280Response(bme280.readTempC(), bme280.readFloatPressure(),
                              bme280.readFloatAltitudeMeters(), bme280.readFloatHumidity());
  server.send(200, "application/json", response);
}

void getHome(){
  String page = webserver.GetHomePageResponse(homePage, expanderPort.LastReading, bme280.readTempC(), 
                                            bme280.readFloatPressure(), bme280.readFloatAltitudeMeters(), 
                                            bme280.readFloatHumidity());
  server.send(200, "text/html", page);
}

void getAPIDocs(){
  apiDocs = webserver.GetAPIPageResponse(apiDocs);
  server.send(200, "text/html", apiDocs);
}

//Helpers
String ipToString(IPAddress ip){
  String temp="";
  for (int i=0; i<4; i++){
    temp += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return temp;
}

void flashStatus(int ms){
  digitalWrite(STATUS_PORT, HIGH);
  delay(ms);
  digitalWrite(STATUS_PORT, LOW);
  delay(ms);
}
