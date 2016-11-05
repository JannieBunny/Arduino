//ESP8266 - Webserver/client and basic sensors with SD card config
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <SparkFunBME280.h>
#include <ESP8266WebServer.h>

#define MAX_GPIO 8
#define STATUS_PORT 10

BME280 bme280;
ESP8266WebServer server(80);
String json;
const char* postHost;
int portHost;
byte expanderValue = 255;
byte addressUpdated = 0;
int id;
int count;

void setup()
{
  //Vars
  String apiDocs;
  String homeStatus;
  
  //Debug
  Serial.begin(9600);
  delay(10);

  pinMode(STATUS_PORT, OUTPUT);
  digitalWrite(STATUS_PORT, HIGH);

  //Analog Input - NodeMCU 0-3.3v
  pinMode(A0, INPUT);
  
  //Setup expansion port
  //GPIO 5 - SDA
  //GPIO 4 - CLK
  Wire.begin(5, 4);

  //Clear the Expander of any latched ports
  Wire.beginTransmission(0x20);
  Wire.write(0xFF);
  Wire.endTransmission(); 

  //Setup SD Card with select pin on 16
  if (!SD.begin(16)) {
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

  //Documentation
  server.on("/", [homeStatus](){
    String temp = homeStatus.substring(0); 
    temp.replace("{{apiLink}}", ipToString(WiFi.localIP()));
    temp.replace("{{celcius}}", String(bme280.readTempC()));
    temp.replace("{{pressure}}", String(bme280.readFloatPressure()));
    temp.replace("{{altitude}}", String(bme280.readFloatAltitudeMeters()));
    temp.replace("{{humidity}}", String(bme280.readFloatHumidity()));
    temp.replace("{{gpio1}}", String(!bitRead(expanderValue, 0)));
    temp.replace("{{gpio2}}", String(!bitRead(expanderValue, 1)));
    temp.replace("{{gpio3}}", String(!bitRead(expanderValue, 2)));
    temp.replace("{{gpio4}}", String(!bitRead(expanderValue, 3)));
    temp.replace("{{gpio5}}", String(!bitRead(expanderValue, 4)));
    temp.replace("{{gpio6}}", String(!bitRead(expanderValue, 5)));
    temp.replace("{{gpio7}}", String(!bitRead(expanderValue, 6)));
    temp.replace("{{gpio8}}", String(!bitRead(expanderValue, 7)));
    server.send(200, "text/html", temp);
  }); 

  server.on("/api", [apiDocs](){
    server.send(200, "text/html", apiDocs);
  }); 
  
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
}

void loop()
{
  //Reconnect WiFI if it drops
  while (WiFi.status() != WL_CONNECTED) {
    debug(50);
  }
  //Handle a client
  server.handleClient();

  count++;
  bool changed = false;

  if(count == 10){
    int value = analogRead(A0);
    if(value < 320){
      sendExpanderValue(0, 0);
    }
    else{
      sendExpanderValue(1, 0);
    }
    Wire.requestFrom(0x20,1); 
    if(Wire.available())
    {
      int result = Wire.read();
      for(int a=0;a<MAX_GPIO;a++){
        bool preValue = bitRead(expanderValue, a);
        bool currentValue = bitRead(result, a);
        if(preValue != currentValue){
          bitWrite(expanderValue, a, currentValue);
          bitWrite(addressUpdated, a, 1);
          changed = true;
        }
      }
    }
    count = 0;
  }

  //Update host
  if(changed){
    Serial.println("Updating");
    sendUpdate();
  }
  else{
    delay(100); 
  }
}

void updateGPIO(){
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    for(int a=0;a<MAX_GPIO;a++){
        int pin = root["GPIO"][a]["PIN"];
        int pinValue = root["GPIO"][a]["VALUE"];
        if(pin != 0){
           sendExpanderValue(!pinValue, (pin-1));
        }
    }
    getGPIO();
}

void getGPIO(){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& response = jsonBuffer.createObject();
  response["ID"] = id;
  JsonArray& nestedArray = response.createNestedArray("GPIO");
  for(int a=0;a<MAX_GPIO;a++){
      JsonObject& nestedObject = nestedArray.createNestedObject();
      nestedObject["PIN"] = a + 1;
      nestedObject["VALUE"] = (int)!bitRead(expanderValue, a);
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
  const char* url = root["URL"];
  const int port = root["PORT"];
  if (client.connect(host, port)) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& response = jsonBuffer.createObject();
    response["ID"] = id;
    JsonArray& gpioArray = response.createNestedArray("GPIO");
    for(int a=0;a<MAX_GPIO;a++){
      JsonObject& gpioObject = jsonBuffer.createObject();
  
      for(int a=0;a<MAX_GPIO;a++){
        if(bitRead(addressUpdated, a)){
          gpioObject["PIN"] = a + 1;
          gpioObject["VALUE"] = bitRead(expanderValue, a);
          gpioArray.add(gpioObject);

          bitWrite(addressUpdated, a, 0);
        }
      }
    }

    client.println("POST /api/values/post HTTP/1.1");
    client.println("Host: hostbin.org");
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
  digitalWrite(STATUS_PORT, HIGH);
  delay(ms);
  digitalWrite(STATUS_PORT, LOW);
  delay(ms);
}

void sendExpanderValue(bool value, byte port){
  bitWrite(expanderValue, port, value);
  Wire.beginTransmission(0x20);
  Wire.write(expanderValue);
  Wire.endTransmission(); 
}

