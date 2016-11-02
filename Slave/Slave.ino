//ESP8266 - Webserver/client and basic sensors with SD card config
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <stdint.h>
#include "SparkFunBME280.h"
#include "Wire.h"

#define DEBUG_PIN  10
#define MAX_GPIO 4

ESP8266WebServer server(80);
String json;
const char* postHost;
int portHost;
int gpioValues[] = { -1, -1, -1, -1};
int gpioValuesToUpdate[] = { -1, -1, -1, -1};
int gpioMappings[] = {15,3,5,16};
int gpioPortMappings[] = {0,0,0,0};
int id;

//Sensors
BME280 bme280;

void setup()
{
  //Vars
  String apiDocs;
  String homeStatus;
  
  //Debug
  Serial.begin(9600);
  delay(10);
  pinMode(DEBUG_PIN,OUTPUT); 
  digitalWrite(DEBUG_PIN, 1);

  //Setup SD Card
  if (!SD.begin(4)) {
    while(true){
      debug(200);
    }
  }

  //Get API docs
  File docs = SD.open("api.txt", FILE_READ);
  if(docs){
    //Increase timeout due to file size
    docs.setTimeout(4000);
    apiDocs = docs.readString();
    docs.close();
  }

  //Get homepage
  File homePage = SD.open("home.txt", FILE_READ);
  if(homePage){
    //Increase timeout due to file size
    homePage.setTimeout(4000);
    homeStatus = homePage.readString();
    homePage.close();
  }
  
  //Find Settings file from SD Card
  File myIdentity = SD.open("settings.txt");
  if(myIdentity){
    json = myIdentity.readString();
    myIdentity.close();
  }
  
  //Read Settings from File
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json); 

  //Connect to WiFi
  JsonObject& wifi = root["WIFI"];
  const char* ssid = wifi["SSID"];
  const char* password = wifi["PASSWORD"];

  //https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF); 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    debug(50);
  }

  //Device ID and identity
  const char* identity = root["IDENTITY"];
  id = root["ID"];
  //Update the docs to include our IP address and ID
  apiDocs.replace("{{host}}", ipToString(WiFi.localIP()));
  apiDocs.replace("{{friendlyName}}", identity);

  //Update Home Page Controls
  homeStatus.replace("{{friendlyName}}", identity);

  JsonArray& gpio = root["GPIO"];

  for(int a=0;a<MAX_GPIO;a++){
    int pin = gpio[a]["PIN"];
    int value = gpio[a]["VALUE"];
    pinMode(gpioMappings[pin -1], value);
    gpioPortMappings[a] = value;
  }

  //Reset all PINS
  for(int a=0;a<MAX_GPIO;a++){
    digitalWrite(gpioMappings[a], 0);
  }

  //Documentation
  server.on("/", [homeStatus](){
    String temp = homeStatus.substring(0); 
    temp.replace("{{apiLink}}", ipToString(WiFi.localIP()));
    temp.replace("{{celcius}}", String(bme280.readTempC()));
    temp.replace("{{pressure}}", String(bme280.readFloatPressure()));
    temp.replace("{{altitude}}", String(bme280.readFloatAltitudeMeters()));
    temp.replace("{{humidity}}", String(bme280.readFloatHumidity()));
    temp.replace("{{gpio1}}", String(digitalRead(gpioMappings[0])));
    temp.replace("{{gpio2}}", String(digitalRead(gpioMappings[1])));
    temp.replace("{{gpio3}}", String(digitalRead(gpioMappings[2])));
    temp.replace("{{gpio4}}", String(digitalRead(gpioMappings[3])));
    server.send(200, "text/html", temp);
  }); 

  server.on("/api", [apiDocs](){
    server.send(200, "text/html", apiDocs);
  }); 
  
  server.on("/api/GPIO/update", HTTP_PUT, updateGPIO);
  server.on("/api/GPIO/get", HTTP_GET, getGPIO);
  server.on("/api/BME280/get", HTTP_GET, getBME280);
  
  server.begin();

  //Ready
  digitalWrite(DEBUG_PIN, 1);
  delay(3000);
  digitalWrite(DEBUG_PIN, 0);

  bme280.settings.commInterface = I2C_MODE;
  bme280.settings.I2CAddress = 0x77;
  bme280.settings.runMode = 3;
  bme280.settings.tStandby = 2;
  bme280.settings.filter = 0;
  bme280.settings.tempOverSample = 1;
  bme280.settings.pressOverSample = 1;
  bme280.settings.humidOverSample = 1;
  
  delay(10);
  bme280.begin();
}

void loop()
{
  //Reconnect WiFI if it drops
  while (WiFi.status() != WL_CONNECTED) {
    debug(50);
  }
  //Handle a client
  server.handleClient();

  int value = 0;
  bool changed = false;
  for(int a=0;a<MAX_GPIO;a++){
    gpioValuesToUpdate[a] = gpioValues[a];
    if(gpioValues[a] != (value = digitalRead(gpioMappings[a]))){
        gpioValues[a] = value;
        changed = true;
    }
  }
  if(changed){
    sendUpdate();
  }
}

void updateGPIO(){
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    int gpio[] = {-1, -1, -1, -1};
    for(int a=0;a<MAX_GPIO;a++){
        int pin = root["GPIO"][a]["PIN"];
        int pinValue = root["GPIO"][a]["VALUE"];
        if(pin != 0){
          gpio[pin-1] = pinValue;
        }
    }
    for(int a=0;a<2;a++){
       if(gpio[a] != -1){
          digitalWrite(gpioMappings[a], gpio[a]);
       }
    }
    server.send(200, "text/plain");
}

void getGPIO(){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& response = jsonBuffer.createObject();
  response["ID"] = id;
  JsonArray& nestedArray = response.createNestedArray("GPIO");
  for(int a=0;a<MAX_GPIO;a++){
      JsonObject& nestedObject = nestedArray.createNestedObject();
      nestedObject["PIN"] = a + 1;
      nestedObject["VALUE"] = digitalRead(gpioMappings[a]);
  }
  char responseString[response.measureLength()+1];
  response.printTo(responseString, response.measureLength()+1);
  server.send(200, "application/json", responseString);
}

void getBME280(){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& response = jsonBuffer.createObject();
  response["ID"] = id;
  response["CELCIUS"] = bme280.readTempC();
  response["PRESSURE"] = bme280.readFloatPressure();
  response["ALTITUDE"] = bme280.readFloatAltitudeMeters();
  response["HUMIDITY"] = bme280.readFloatHumidity();
  char responseString[response.measureLength()+1];
  response.printTo(responseString, response.measureLength()+1);
  server.send(200, "application/json", responseString);
}

void sendUpdate(){
  WiFiClient client;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json); 
  const char* host = root["HOST"];
  const int port = root["PORT"];
  if (client.connect(host, port)) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& response = jsonBuffer.createObject();
    response["ID"] = id;
    JsonArray& gpioArray = response.createNestedArray("GPIO");
    for(int a=0;a<MAX_GPIO;a++){
      JsonObject& gpioObject = jsonBuffer.createObject();
      if(gpioValuesToUpdate[a] != gpioValues[a]){
        gpioObject["PIN"] = a + 1;
        gpioObject["VALUE"] = gpioValues[a];
        gpioArray.add(gpioObject);
        gpioValuesToUpdate[a] = gpioValues[a];
      }
    }
    
    client.println("POST /api/values/post HTTP/1.1");
    client.println("Host: httpbin.org");
    client.println("Cache-Control: no-cache");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(response.measureLength());
    client.println();
    response.printTo(client);
  }
  else{
    Serial.println("Failed to Connect");
  }
}

String ipToString(IPAddress ip){
  String temp="";
  for (int i=0; i<4; i++)
    temp += i  ? "." + String(ip[i]) : String(ip[i]);
  return temp;
}

void debug(int ms){
    digitalWrite(DEBUG_PIN, 1);
    delay(ms);
    digitalWrite(DEBUG_PIN, 0);
    delay(ms);
}

