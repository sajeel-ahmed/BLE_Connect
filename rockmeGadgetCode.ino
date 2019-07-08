/*
 * Version Updates
 * Storing Multiple WiFi Credentials
 * Connecting with WiFi having strong connection
 * Try to reconnect with WiFi if there is no WiFi 
 * Reconnect with WiFi ,whose credentials are in its memory, when user switch its place
 * Recieve FireBase Credentials and saving to Config.json
 * Change Full LED Panel Color when data in Firebase is writen as  
 * User-RockMe/Message/Red
 * User-RockMe/Message/Green
 * User-RockMe/Message/Blue
 * User-RockMe/Message/White
 * 
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <FirebaseCloudMessaging.h>

#define USE_SERIAL Serial

#include <ESP8266WiFiMulti.h>   // Include the Wi-Fi-Multi library

//#include <ArduinoJson.h>
#include "FS.h"

#include <FirebaseArduino.h>
//#include <FirebaseHttpClient.h>
#include <Adafruit_NeoPixel.h>

/**********************************************APA102*****************************************************************/

#define dataPin 13  // GPIO-PIN 16
#define NUMPIXELS    386
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, dataPin, NEO_GRB + NEO_KHZ800);
/**********************************************FIREBASE***************************************************************/

// Set these to run Connect with Firebase.
#define FIREBASE_HOST "rockme-938be.firebaseio.com/"
#define FIREBASE_AUTH ""
String Firebase_User = "84:F3:EB:5D:DA:DE/Command";
#define SERVER_KEY "AIzaSyAsR1mPoOTmPjE1vrGykKb7boiHVHx94qY"
/***************************************** VARIABLES GLOBALES ***************************************************/

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

int contconexion = 0;
unsigned long previousMillis = 0;

char ssid[50];      
char pass[50];

const char* ssid_SPIFFS;
const char* pass_SPIFFS;

const char *ssidConf = "RockMe";
const char *passConf = "12345678";

String message = "";

//--------------------------------------------------------------
WiFiClient espClient;
ESP8266WebServer server(80);

/***************************************** Functions to Deal with SPIFFS ***************************************************/


//-------------------SPIFFS + JSON + Load--------------------------
bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

//  StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  Serial.println('\n');
  json.prettyPrintTo(Serial);
  Serial.println('\n');
  
  int count = int(json["Count"]);
  
  for(int i=0;i<count;i++){
  ssid_SPIFFS = json["Wifi"][i]["SSID"];
  pass_SPIFFS = json["Wifi"][i]["PASSWORD"];

  // Real world application would store these values in some variables for
  // later use.

  Serial.print("Loaded SSID: ");
  Serial.println(ssid_SPIFFS);
  Serial.print("Loaded PASS: ");
  Serial.println(pass_SPIFFS);
  wifiMulti.addAP(ssid_SPIFFS, pass_SPIFFS);   // add Wi-Fi networks you want to connect to
  }
  return true;
}

//-------------------SPIFFS + JSON + Save + WIFI --------------------------
bool saveConfig(char *ssid_new,char *pass_new) {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  configFile.close();
  
//  StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  json.prettyPrintTo(Serial);

  int count = int(json["Count"]) + 1;
  json["Count"] = count;
  
  JsonObject& new_obj = json["Wifi"].asArray().createNestedObject();
  new_obj["SSID"] = ssid_new;
  new_obj["PASSWORD"] = pass_new;
  
  json.prettyPrintTo(Serial);
  File configFileWrite = SPIFFS.open("/config.json", "w");
  if (!configFileWrite) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFileWrite);
  configFileWrite.close();
  return true;
}

//-------------------SPIFFS + JSON + Save + WIFI --------------------------
bool saveConfigFirebase(char *fire_new) {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  configFile.close();
  
// StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  json.prettyPrintTo(Serial);

  json["Firebase"] = fire_new;
  Firebase_User = fire_new;

  json.prettyPrintTo(Serial);
  File configFileWrite = SPIFFS.open("/config.json", "w");
  if (!configFileWrite) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFileWrite);
  configFileWrite.close();
  return true;
}

/***************************************** Functions to Deal with HTML ***************************************************/

//----------- HTML CODE CONFIGURATION PAGE ---------------
String page = "<!DOCTYPE html>"
"<html>"
"<head>"
"<title>RockME Credentials</title>"
"<meta charset='UTF-8'>"
"</head>"
"<body>"
"</form>"

"<form action='saveConf' method='get'>"
"SSID:<br>"
"<input class='input1' name='ssid' type='text'><br>"
"PASSWORD:<br>"
"<input class='input1' name='pass' type='password'><br>"
"<input class='button' type='submit' value='SAVE'/><br><br>"
"</form>"

"<form action='saveFireConf' method='get'>"
"FireBase Credentionals:<br>"
"<input class='input2' name='FireCred' type='text'><br>"
"<input class='button' type='submit' value='SAVE'/><br><br>"
"</form>"

"<form action='saveDataConf' method='get'>"
"Data Configurations:<br>"
"<input class='input2' name='LedData' type='text'><br>"
"<input class='button' type='submit' value='SAVE'/><br><br>"
"</form>"

"<a href='scanWiFi'><button class='button'>SCAN</button></a><br><br>";

String endPage = "</body>"
"</html>";

//-------------------Configuration PAGE--------------------
void pageConf() {
  server.send(200, "text/html", page + message + endPage); 
}

//-------------------Firebase Reading--------------------
char data_buf[150];
unsigned int data_lenght=0, check = 0;

//void saveFirebaseData() {
//  String firebaseData = Firebase.getString(Firebase_User);
//  data_lenght = firebaseData.length();
//  
//  Serial.print(data_lenght);
//  Serial.print("  ");
//  
//  Serial.println(firebaseData);
//  firebaseData.toCharArray(data_buf,150);
//  check = 0;
//  
//}

void saveFirebaseData(){
    HTTPClient http;
    
    //USE_SERIAL.print("[HTTP] begin...\n");
    // configure traged server and url
    http.begin("https://rockme-938be.firebaseio.com/84:F3:EB:5D:DA:DE/Command.json", "6F D0 9A 52 C0 E9 E4 CD A0 D3 02 A4 B7 A1 92 38 2D CA 2F 26"); //HTTPS
    //http.begin("https://rockme-938be.firebaseio.com/60:01:94:53:10:9D.json"); //HTTP
    int httpCode = http.GET();

    //httpCode will be negative on error
      if (httpCode == HTTP_CODE_OK) {
        String firebase_Data = http.getString(); 
         data_lenght = firebase_Data.length();
  
          //Serial.print(data_lenght);
          //Serial.print("  ");
          //Serial.println(firebase_Data);
          firebase_Data.toCharArray(data_buf,150);
          /*for(int m=0;m<150;m++)
          {
            Serial.print(data_buf[m]);
          }
          Serial.println();*/
          check = 0;
          http.end();
      }

  }
 

//--------------------MODE_CONFIGURATION------------------------
void modeConf() {
  
  WiFi.softAP(ssidConf, passConf);
  IPAddress myIP = WiFi.softAPIP(); 
  Serial.println('\n');
  Serial.print("Access point IP: ");
  Serial.println(myIP);
  Serial.println("WebServer started...");

  server.on("/", pageConf); //this is the configuration page

  server.on("/saveConf", saveConf); //Record the configuration in the SPIFFS

  server.on("/saveFireConf", saveFireConf); //Record the configuration in the SPIFFS

  server.on("/saveDataConf", saveDataConf); //Record the configuration in the SPIFFS
  
  server.on("/UID", UniqueID); //Scan the available Wi-Fi networks 

  server.on("/scanWiFi", scanWiFi); //Scan the available Wi-Fi networks 
  
  server.begin();

}

//---------------------SAVE CONFIGURATIONS-------------------------
void saveConf() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  Serial.println('\n');
  Serial.println(ssid);//sends the values received by the web form GET
  
  char ssid_buf[30];
  char pass_buf[30];
  ssid.toCharArray(ssid_buf,30);
  pass.toCharArray(pass_buf,30);
  
  if (!saveConfig(ssid_buf,pass_buf)) {
    Serial.println("Failed to save config");
  } else {
    Serial.println("Config saved");
  }
  wifiMulti.addAP(ssid_buf,pass_buf);   // add Wi-Fi networks you want to connect to
  message = "WiFi Credentials Saved...";
  
  pageConf();
}

void saveFireConf() {
  String fire = server.arg("FireCred");
  
  Serial.println('\n');
  Serial.println(fire);//sends the values received by the web form GET
  
  char fire_buf[150];
  
  fire.toCharArray(fire_buf,150);
  
  if (!saveConfigFirebase(fire_buf)) {
    Serial.println("Failed to save config");
  } else {
    Serial.println("Config saved");
  }
  pageConf();
}
  
void saveDataConf() {
  pageConf();
}

void UniqueID(){
  server.send(200, "text/html", WiFi.macAddress());
  }
/***************************************** Functions to Deal with WiFi ***************************************************/

//---------------------------WiFi SCAN----------------------------
void scanWiFi() {  
  int n = WiFi.scanNetworks(); //returns the number of networks found
  Serial.println("scan finished");
  if (n == 0) { //if you can not find a 
    Serial.println("no networks found");
    message = "no networks found";
  }  
  else
  {
    Serial.print(n);
    Serial.println(" network found");
    message = "";
    for (int i = 0; i < n; ++i)
    {
      // add to the STRING "message" the information of the networks found 
      message = (message) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i) + " </p>\r\n";
      //WiFi.encryptionType 5: WEP 2: WPA / PSK 4: WPA2 / PSK 7: open network 8: WPA / WPA2 / PSK 
      //delay(10);
    }
    Serial.println(message);
    pageConf();
  }
}


//------------------------MONITOR WIFI---------------------

boolean connectioWasAlive = true;

void monitorWiFi()
{
  if (wifiMulti.run() != WL_CONNECTED)
  {
    if (connectioWasAlive == true)
    {
      connectioWasAlive = false;
      Serial.println('\n');
      Serial.print("Looking for WiFi ");
    }
    Serial.print(".");
  }
  else if (connectioWasAlive == false)
  {
    connectioWasAlive = true;
    Serial.println('\n');
    Serial.printf("Connected to %s\n", WiFi.SSID().c_str());
   
    Serial.print("IP address:  ");
    Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
  }
}

//------------------------SETUP-----------------------------
void setup() {
  /**********************************************APA 102***************************************************************/
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  
  // Initialize Serial
  Serial.begin(115200);
  Serial.println("");
  
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  
  pinMode(4, INPUT);  //D5
  if (digitalRead(4) == 0) {
    modeConf();
  }
  
  if (!loadConfig()) {
    Serial.println();
    Serial.println("Failed to load config");
  } else {
    Serial.println();
    Serial.println("Config Loaded");
  }
  
  WiFi.mode(WIFI_AP_STA); //so that it does not start the SoftAP in the normal
  modeConf();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.println(WiFi.macAddress());
//---------------------------------
  if (wifiMulti.run() != WL_CONNECTED)
  {
    if (connectioWasAlive == true)
    {
      connectioWasAlive = false;
      Firebase.set("OFF",0);
    }
  }
  else if (connectioWasAlive == false)
  {
    connectioWasAlive = true;
    Firebase.set("ON",0);
  }
  //---------------------------
  
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(25);
  pixels.show(); // This sends the updated pixel color to the hardware
}


//.........................................................................................................................
unsigned int hexToDec(char hexString) {
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < 1; i++) {
    
    nextInt = int(hexString);
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}

 
//--------------------------LOOP--------------------------------
//char charBuf[3];
  void loop() {
  unsigned int dec=0;
  int count=0;
  unsigned int num=0,lastNum=0;
  unsigned int color_seperation_check = 0;
  unsigned int color_seperation_value = 1,color_seperation_value_last = 0;
  unsigned int data_lenght=0, check = 0;
  char data_buf[150]={0};
  unsigned int color[3]={0};
  monitorWiFi();
/**********************************************FIREBASE***************************************************************/
    HTTPClient http;

    http.begin("https://rockme-938be.firebaseio.com/84:F3:EB:5D:DA:DE/Command.json", "6F D0 9A 52 C0 E9 E4 CD A0 D3 02 A4 B7 A1 92 38 2D CA 2F 26"); //HTTPS
    int httpCode = http.GET();
      if (httpCode>0) {
        String firebase_Data = http.getString(); 
        data_lenght = firebase_Data.length();
        firebase_Data.toCharArray(data_buf,150);
        check = 0;
      }
      http.end();
/**********************************************APA 102***************************************************************/
  //server.handleClient();
  //delay(1);

   for(int i=data_lenght-1; i>0; i--){
    if(data_buf[i]==';' && check == 0){
      color_seperation_check++;
      }
  }
  //--------------------------------------------------------------------------------------
 for(int g = color_seperation_check; g>0; g--){
    check = 1;
    Serial.print("String Lenght:  ");
    Serial.println(data_lenght);
    Serial.println("String Decode"); 
    if(data_buf[color_seperation_value +0]=='#'){
     
     for(int i=1; i<=7; i++){
      if(data_buf[color_seperation_value + i]=='$'){
        break;
        }
      if(i==1 || i==3 || i==5){
        dec=hexToDec(data_buf[color_seperation_value + i]);
        dec=dec*16;
        }
      if(i==2 || i==4 || i==6){
        dec= dec + hexToDec(data_buf[color_seperation_value + i]);
        int k=(i/2)-1;
        color[k]=dec;
        }     
      } 
      
      if(data_buf[color_seperation_value + 7]=='$'){
        count = 0;
        byte numArrayCheck = 0;
        byte firstNumCheck = 0;
        
        for(int j=8;j<data_lenght-1;j++){ // data_lenght = 15
          
          if(data_buf[j+color_seperation_value] == ',' || data_buf[j+color_seperation_value] == '-' || data_buf[j+color_seperation_value] == ';' ){
            
            if(count==1){
              num=hexToDec(data_buf[color_seperation_value + j-1]);
              }
            else if(count==2){
              dec=10 * hexToDec(data_buf[color_seperation_value + j-2]); 
              dec=dec + hexToDec(data_buf[color_seperation_value + j-1]); 
              num=dec;}    
            else if(count==3){
              dec=100 * hexToDec(data_buf[color_seperation_value + j-3]); 
              dec=dec + (10 * hexToDec(data_buf[color_seperation_value + j-2])); 
              dec=dec + hexToDec(data_buf[color_seperation_value + j-1]); 
              num=dec;
              }
            count = -1;        
     

            if(firstNumCheck == 0 || numArrayCheck == 0){
//              Serial.println(g);
              lastNum = num;
              }
              unsigned int k = num;
              for(k = lastNum; k <= num ; k++){
              //do{
              pixels.setPixelColor(k, pixels.Color(color[0],color[1],color[2])); // Moderately bright green color.}
              //delay(10);
              Serial.print(k);
              Serial.print("  ");
              //k++;
//              Serial.print(k);
//              Serial.print("  ");
//              } 
              //}while (k > lastNum);
              }
              if(data_buf[j+color_seperation_value] == '-'){
                 numArrayCheck = 1;
                 firstNumCheck = 1; 
//                 Serial.println(g+1);
                 
              }
              else{ 
                numArrayCheck = 0;
                firstNumCheck = 1; 
//                Serial.println(g+2);
              }

            Serial.print(color[0],HEX);
            Serial.print(color[1],HEX);
            Serial.println(color[2],HEX);
            
            }
              
          if(data_buf[j+color_seperation_value]==';'){
              int k = color_seperation_value_last;
              color_seperation_value_last = color_seperation_value;
              color_seperation_value = color_seperation_value+j+1;
              break;
            }  
          count++;
          lastNum = num;
         }
     }
     //data_buf[color_seperation_value_last + 0]='1';
    }

  }     
  pixels.show();
  //delay(10);
    if (wifiMulti.run() != WL_CONNECTED)
  {
    if (connectioWasAlive == true)
    {
      connectioWasAlive = false;
      Firebase.set("&0",0);
    }
  }
  else if (connectioWasAlive == false)
  {
    connectioWasAlive = true;
    Firebase.set("&1",0);
  }
}

