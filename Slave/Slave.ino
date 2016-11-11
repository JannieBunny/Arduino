//ESP8266 - Webserver/client and basic sensors with SD card config
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <MQTTClient.h>
#include <SparkFunBME280.h>
#include <ESP8266WebServer.h>
#include <SDSettings.h>
#include <WebServer.h>

#define MAX_GPIO 8
#define STATUS_PORT 10
#define EXPANDER_PORT 0x20

SDSettings settings;
BME280 bme280;
ESP8266WebServer server(80);
WiFiClient mqttpipe;
MQTTClient mqtt;
WebServer webserver;
  
String json;
String homePage;
String apiDocs;

byte oldValue = 255;
byte expanderValue = 255;
byte addressUpdated = 0;
int id;
int count;
bool postUpdate;
bool publishToMQTT;

void setup()
{ 
  //Debug
  Serial.begin(9600);
  delay(10);

  pinMode(STATUS_PORT, OUTPUT);
  digitalWrite(STATUS_PORT, HIGH);
  
  //Setup expansion port
  //GPIO 5 - SDA
  //GPIO 4 - CLK
  Wire.begin(5, 4);

  //Clear the Expander of any latched ports
  Wire.beginTransmission(EXPANDER_PORT);
  Wire.write(0xFF);
  Wire.endTransmission(); 

  //Setup SD Card with select pin on 16
  if (!SD.begin(16)) {
    while(true){
      debug(200);
    }
  }

  //Get API docs
  apiDocs = settings.ReadIntoString("api.txt");
 
  //Get homepage
  homePage = settings.ReadIntoString("home.txt");
  
  //Find Settings file from SD Card
  json = settings.ReadIntoString("settings.txt");
  
  //Read Settings from File
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
    debug(50);
  }

  //Device ID and identity
  //Update the docs to include our IP address and ID

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

  //Check if we should connect to MQTT broker
  bool connectToMQTT = root["MQTT"]["ENABLED"];
  if(connectToMQTT){
      const char* broker = root["MQTT"]["BROKER"];
      int port = root["MQTT"]["PORT"];
      mqtt.begin(broker, port, mqttpipe);
      connectMQTT();
      publishToMQTT = true;
  }
  //Check if we should post an update
  postUpdate = root["REST"]["ENABLED"];
}

void loop()
{
  //Reconnect WiFI if it drops
  while (WiFi.status() != WL_CONNECTED) {
    debug(50);
  }

  if(publishToMQTT){
    mqtt.loop();
    delay(10);
    if(!mqtt.connected()) {
      connectMQTT();
    }
  }
  
  //Handle a web client
  server.handleClient();
  
  count++;
  bool changed = false;

  if(count == 10){
    Wire.requestFrom(EXPANDER_PORT,1); 
    if(Wire.available())
    {
      int result = Wire.read();
      for(int a=0;a<MAX_GPIO;a++){
        bool preValue = bitRead(oldValue, a);
        bool currentValue = bitRead(result, a);
        if(preValue != currentValue){
          bitWrite(oldValue, a, currentValue);
          bitWrite(addressUpdated, a, 1);
          changed = true;
        }
      }
    }
    count = 0;
  }

  //Update host
  if(changed){ 
    String response = createUpdateResponse();
    if(publishToMQTT){
      //Get my topic
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(json); 
      const char* outTopic = root["MQTT"]["TOPICOUT"];
      mqtt.publish(outTopic, response);
    }
    if(postUpdate){
      sendUpdate(response);
    }
    flagGPIOUpdated();
  }
  else{
    delay(100); 
  }
}

void connectMQTT() {
  Serial.println("Connecting to MQTT Broker");
  while (!mqtt.connect(String(id).c_str())) {
    Serial.println("MQTT connection failed, retrying...");
    //Retry in 5 seconds
    delay(5000);
  }

  Serial.println("Connected to MQTT Broker");
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json); 
  const char* inTopic = root["MQTT"]["TOPICIN"];
  mqtt.subscribe(inTopic);
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  updateGPIOPins(root);
}

void sendUpdate(String response){
  WiFiClient client;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json); 
  const char* host = root["REST"]["HOST"];
  const char* url = root["REST"]["URL"];
  const int port = root["REST"]["PORT"];
  Serial.println("Connecting to POST server");
  if (client.connect(host, port)) {
    //POST Headers
    String hostParam = "Host: ";
    String contentType = "POST ";
    String httpType = " HTTP/1.1";
    client.println(contentType + String(url) + httpType);
    client.println(hostParam + String(host));
    client.println("User-Agent: Arduino/1.0");
    client.println("Cache-Control: no-cache");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(response.length());
    client.println();
    client.println(response);
    Serial.println("Response sent to POST server");
  }
  else{
    Serial.println("Failed to Connect to POST server");
  }
}

String createUpdateResponse(){
    DynamicJsonBuffer jsonBuffer;
    JsonObject& response = jsonBuffer.createObject();
    response["ID"] = id;
    JsonArray& gpioArray = response.createNestedArray("GPIO");
    for(int a=0;a<MAX_GPIO;a++){
      if(bitRead(addressUpdated, a)){
        JsonObject& gpioObject = jsonBuffer.createObject();
        gpioObject["PIN"] = 1 + a;
        gpioObject["VALUE"] = bitRead(oldValue, a);
        gpioArray.add(gpioObject);
      }
    }
    int responseBufferSize = response.measureLength()+1;
    char responseBuffer[responseBufferSize];
    response.printTo(responseBuffer, responseBufferSize);
    return String(responseBuffer);
}

void flagGPIOUpdated(){
  for(int a=0;a<MAX_GPIO;a++){
      bitWrite(addressUpdated, a, 0);  
  }
}

void updateGPIO(){
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    updateGPIOPins(root);
    getGPIO();
}

void updateGPIOPins(JsonObject& root){
    for(int a=0;a<MAX_GPIO;a++){
      int pin = root["GPIO"][a]["PIN"];
      int pinValue = root["GPIO"][a]["VALUE"];
      if(pin != 0){
         sendExpanderValue(!pinValue, (pin-1));
      }
  }
}

void getGPIO(){
  String response = webserver.GetGPIOResponse(expanderValue);
  server.send(200, "application/json", response);
}

void getBME280(){
  String response = webserver.GetBME280Response(bme280.readTempC(), 
                              bme280.readFloatPressure(),
                              bme280.readFloatAltitudeMeters(), 
                              bme280.readFloatHumidity());
  server.send(200, "application/json", response);
}

void getHome(){
  String page = webserver.GetHomePageResponse(homePage, expanderValue,
                                            bme280.readTempC(), 
                                            bme280.readFloatPressure(),
                                            bme280.readFloatAltitudeMeters(), 
                                            bme280.readFloatHumidity());
  server.send(200, "text/html", page);
}

void getAPIDocs(){
  apiDocs = webserver.GetAPIPageResponse(apiDocs);
  server.send(200, "text/html", apiDocs);
}

String ipToString(IPAddress ip){
  String temp="";
  for (int i=0; i<4; i++)
    temp += i  ? "." + String(ip[i]) : String(ip[i]);
  return temp;
}

void debug(int ms){
  digitalWrite(STATUS_PORT, HIGH);
  delay(ms);
  digitalWrite(STATUS_PORT, LOW);
  delay(ms);
}

void sendExpanderValue(bool value, byte port){
  bitWrite(expanderValue, port, value);
  Wire.beginTransmission(EXPANDER_PORT);
  Wire.write(expanderValue);
  Wire.endTransmission(); 
}

