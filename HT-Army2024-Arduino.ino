/*Connect
HT22
HT22 <---> D1
VCC <---> 3V3,
GND <---> GND
LED2.4Display
LED <---> 3V3
CS <---> D2 
RST <---> D3 
D/C <---> D4 
MOSI <---> D7 
SCK <---> D5 
VCC <---> 3V3,l
GND <---> GND
*/

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include "DHT.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <NTPClient.h>
#include <TridentTD_LineNotify.h>

String DEVICE_ID = "S003";  //edit=====================================
#define SCREEN_VERSION 1 //edit=====================================

#define TFT_CS         D2   //14
#define TFT_RST        D3   //15
#define TFT_DC         D4   //32
#define DHTPIN D1   
#define DHTTYPE DHT22 

#if SCREEN_VERSION == 1
  #define ST77XX_B 0X04FF
  #define ST77XX_M 0XF81F
  #define ST77XX_CYAN 0X07FF
#elif SCREEN_VERSION == 2
  #define ST77XX_B 0X04FF
  #define ST77XX_M 0x07E0
  #define ST77XX_BLACK 0xFFFF
  #define ST77XX_RED 0xFFE0
  #define ST77XX_GREEN 0xF81F
  #define ST77XX_WHITE 0x0000
  #define ST77XX_BLUE 0xF800
  #define ST77XX_CYAN 0x07FF
  #define ST77XX_MAGENTA 0xF81F
#endif

// if(SCREEN_VERSION==1){
//   #define ST77XX_B 0X04FF
//   #define ST77XX_M 0XF81F
//   #define ST77XX_CYAN 0X07FF
// }
// else if(SCREEN_VERSION==2){
//   #define ST77XX_B 0X04FF
//   #define ST77XX_M 0x07E0  //0XF81F
//   #define ST77XX_BLACK 0xFFFF//0x0000
//   #define ST77XX_RED 0xFFE0//0x001F
//   #define ST77XX_GREEN 0xF81F //0x07E0
//   #define ST77XX_WHITE 0x0000//0xFFFF
//   #define ST77XX_BLUE 0xF800
//   #define ST77XX_CYAN 0x07FF
//   #define ST77XX_MAGENTA 0xF81F
// }

bool firsttime = true;
float humid,temp,hic,water,pm25;
int train,rest,timestamp;
String flag,status;
float adjust_temp = 0;
float adjust_humid = 0;
float adjust_pm25 = 0;
const long offsetTime = 25200;       // หน่วยเป็นวินาที จะได้ 7*60*60 = 25200
String LINE_TOKEN1,LINE_TOKEN2,UNIT;
String sheet_api = ""; //"https://script.google.com/macros/s/AKfycbx1nHCA01C2U0NdpsnPdO0Oc5xEjLgfOZWIOwu1f0DX72OGHOHHBRdRqwZyNO-EENF1xg/exec?action=addData";
String url1 = "https://raw.githubusercontent.com/phawitb/HT-Army2024/main/config.txt";
String url2 = "https://raw.githubusercontent.com/phawitb/HT-Army2024/main/config_googlesheetAPi.txt";
bool SEND_LINE = false;
bool SEND_DATA = false;

WiFiUDP ntpUDP;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
DHT dht(DHTPIN, DHTTYPE);
WiFiManager wm;
NTPClient timeClient(ntpUDP, "pool.ntp.org", offsetTime);

float stringToFloat(String str) {
  char charArray[str.length() + 1];
  str.toCharArray(charArray, str.length() + 1);
  
  return atof(charArray);
}

String splitString(String str,String id) {
  char charArray[str.length() + 1];  
  str.toCharArray(charArray, sizeof(charArray));  
  char* token = strtok(charArray, "\n");
  while (token != NULL) {
    String d = String(token[0])+String(token[1])+String(token[2])+String(token[3]);
    if(d==id){
      return token;
    }
    token = strtok(NULL, "\n");
    
  }
  return "None";
}

void splitString2(String data) {
  Serial.print("xxxx");
  Serial.print(data);

  String parts[7];

  int index = 0;
  while(data.length() > 0) {
      int commaIndex = data.indexOf(',');
      if(commaIndex >= 0) {
          parts[index] = data.substring(0, commaIndex);
          data = data.substring(commaIndex + 1);
      } else {
          parts[index] = data;
          break;
      }
      index++;
  }
  for(int i = 0; i < 7; i++) {
      Serial.println(parts[i]);
  }

  adjust_temp = stringToFloat(parts[1]);
  adjust_humid = stringToFloat(parts[2]);
  adjust_pm25 = stringToFloat(parts[3]);
  LINE_TOKEN1 = parts[4];
  LINE_TOKEN2 = parts[5];
  UNIT = parts[6];
  Serial.print("adjust_temp"); Serial.println(adjust_temp);
  Serial.print("adjust_humid"); Serial.println(adjust_humid);
  Serial.print("adjust_pm25"); Serial.println(adjust_pm25);
  Serial.print("LINE_TOKEN1"); Serial.println(LINE_TOKEN1);
  Serial.print("LINE_TOKEN2"); Serial.println(LINE_TOKEN2);
  Serial.print("UNIT"); Serial.println(UNIT);

}

int update_api(String DEVICE_ID,float temp,float humid,float hic,String flag){
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  int httpResponseCode;
  if(https.begin(*client, sheet_api)) {    
      // String p = "{\"date\":20240502,\"id\":\"S002\",\"temp\":12.33,\"humid\":43.3,\"hic\":60.78,\"flag\":\"red\"}";
      String p = "{\"date\":" + String(timeClient.getEpochTime()) +
                  ",\"id\":\"" + DEVICE_ID +
                  "\",\"temp\":" + String(temp) +
                  ",\"humid\":" + String(humid) +
                  ",\"hic\":" + String(hic) +
                  ",\"flag\":\"" + flag + "\"}";

      https.addHeader("Content-Type", "application/json");
      httpResponseCode = https.POST(p);
      String content = https.getString();
      Serial.print("\nhttpResponseCode"); Serial.print(httpResponseCode);
      Serial.print("content"); Serial.println(content);
      https.end();
    }
    else{
      Serial.printf("[HTTPS] Unable to connect\n");
    }
    return httpResponseCode;
    
}

void updateScreen(String flag,float temp,float humid,String status,int bat_percentage) {
  tft.fillScreen(ST77XX_BLACK);
  if(status=="online"){
    tft.setTextColor(ST77XX_M);
  }
  else{
    tft.setTextColor(ST77XX_RED);
  }
  if(temp == -99){
    tft.setTextColor(ST77XX_BLACK);
  }
  tft.setCursor(0, 30);
  tft.setTextSize(10);
  tft.print(String(int(floor(temp))));
  tft.setTextSize(3);
  tft.print("O");
  tft.setTextColor(ST77XX_B);
  if(temp == -99){
    tft.setTextColor(ST77XX_BLACK);
  }
  tft.setCursor(167, 30);
  tft.setTextSize(10);
  tft.print(String(int(floor(humid))));
  tft.setTextSize(4);
  tft.print("%");
  tft.setTextSize(4);
  tft.setCursor(110, 72);
  if(status=="online"){
    tft.setTextColor(ST77XX_M);
  }
  else{
    tft.setTextColor(ST77XX_RED);
  }
  if(temp == -99){
    tft.setTextColor(ST77XX_BLACK);
  }
  tft.print("." + String(int(10*(temp-floor(temp)))));
  tft.setCursor(270, 72);
  tft.setTextColor(ST77XX_B);
  if(temp == -99){
    tft.setTextColor(ST77XX_BLACK);
  }
  tft.print("." + String(int(10*(humid-floor(humid)))));
  tft.setCursor(0, 160);
  tft.setTextSize(7);
  if(flag=="GREEN"){
    tft.fillRect(0,140,350,140,ST77XX_GREEN);  //bg
    tft.setTextColor(ST77XX_BLACK); //text
    tft.println(" GREEN ");   //7
  }
  else if (flag=="RED") {
    tft.fillRect(0,140,350,140,ST77XX_RED);
    tft.setTextColor(ST77XX_WHITE);
    tft.println("  RED  ");   //7
  } 
  else if (flag=="YELLOW") {
    tft.fillRect(0,140,350,140,ST77XX_YELLOW);
    tft.setTextColor(ST77XX_BLACK);
    tft.println(" YELLOW ");   //7
  }
  else if (flag=="BLACK") {
    tft.fillRect(0,140,350,140,ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.println(" BLACK ");   //7
  }  
  else if (flag=="WHITE") {
    tft.fillRect(0,140,350,140,ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.println(" WHITE ");   //7
  }
  tft.setCursor(5, 220);
  tft.setTextSize(2);
  tft.println(String(hic,1));
  tft.setTextSize(1);
  tft.setCursor(55, 217);
  tft.print("o");
  if(status == "online"){
    tft.setCursor(240, 220);    
    if(flag=="GREEN"){
      tft.setTextColor(ST77XX_BLUE);
    }
    else{
      tft.setTextColor(ST77XX_GREEN);
    }
  }
  else{
    tft.setCursor(230, 220);
    if(flag=="RED"){
      tft.setTextColor(ST77XX_BLACK);
    }
    else{
      tft.setTextColor(ST77XX_RED);
    }    
  }
  if(status=="severfail"){
    tft.setCursor(210, 220);
  }
  tft.setTextSize(2);
  String s = status;
  s.toUpperCase();
  tft.print(s);
}

void setup(){
  Serial.begin(115200);

  dht.begin();

  //setup screen
  if(SCREEN_VERSION==1){
    tft.init(240, 320);
    tft.invertDisplay(false);
    tft.setRotation(3); 
  }
  else if(SCREEN_VERSION==2){
    tft.init(240, 320);
    tft.setRotation(1);   

  }
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(0, 40);
  tft.setTextSize(5);
  tft.println(DEVICE_ID);

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    

  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);
  if(wm.autoConnect("AutoConnectAP")){
      Serial.println("connected...yeey :)");
  }
  else {
      Serial.println("Configportal running");
  }

}

void loop(){

  Serial.print("sheet_api"); Serial.println(sheet_api);

  int bat_percentage = analogRead(A0);
  Serial.print("Batterry Percen...");
  Serial.println(bat_percentage);
  Serial.print("A0...");
  Serial.println(analogRead(A0));

  if(WiFi.status() == WL_CONNECTED && firsttime == true){
    firsttime = false;
    timeClient.begin();

    //GET CONFIG
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    //get config1 error and token===========================
    HTTPClient https1;
    Serial.println("Requesting " + url1);
    if (https1.begin(*client, url1)) {
      int httpCode1 = https1.GET();
      String content1 = https1.getString();
      Serial.println("===================================== Response code: " + String(httpCode1) + content1);
      String b = splitString(content1,DEVICE_ID);   //"202306002");
      Serial.println(b);
      splitString2(b);

    }
    //get config2 sheet api url from github ===========================
    HTTPClient https2;
    Serial.println("Requesting " + url2);
    if (https2.begin(*client, url2)) {
      int httpCode2 = https2.GET();
      String content2 = https2.getString();
      Serial.println("xxxxxxxxxx Response code: xxxxxxxxx");
      Serial.println(String(httpCode2));
      Serial.println(String(content2));
      if(content2 != ""){
        sheet_api = content2;
      }
      Serial.println(sheet_api);
      Serial.println("xxxxxxxxxx ============== xxxxxxxxx");
     
    }

    //show config on screen
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(0, 40);
    tft.setTextSize(5);
    tft.println(DEVICE_ID);
    
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(4);
    tft.setCursor(0, 120);
    String s = "";
    if(adjust_temp >0){
        s = "+";
    }
    tft.println("Temp "+s+String(adjust_temp));
    tft.setCursor(0, 170);
    s = "";
    if(adjust_humid >0){
        s = "+";
    }
    tft.println("Humid "+s+String(adjust_humid));

    delay(5000);

  }

  //read DHT
  humid = dht.readHumidity() + adjust_humid;
  temp = dht.readTemperature() + adjust_temp;
  if(isnan(humid) || isnan(temp)) {
    humid = -99;
    temp = -99;
    hic = -99;
  }
  else{
    hic = dht.computeHeatIndex(temp, humid, false);
  }
  if(hic == -99){
    flag = "none";
    water = -1;
    train = -1;
    rest = -1;
  }    
  else if(hic<27){
    flag = "WHITE";
    water = 0.5;
    train = 60;
    rest = 0;
  }
  else if(hic<32){
    flag = "GREEN";
    water = 1.5;
    train = 50;
    rest = 10;
  }
  else if(hic<41){
    flag = "YELLOW";
    water = 1;
    train = 45;
    rest = 15;
  }
  else if(hic<55){
    flag = "RED";
    water = 1;
    train = 30;
    rest = 30;
  }
  else{
    flag = "BLACK";
    water = 1;
    train = 20;
    rest = 40;
  }
  Serial.print("temp");
  Serial.println(temp);
  Serial.print("humid");
  Serial.println(humid);
  Serial.print("adjust_temp");
  Serial.println(adjust_temp);
  Serial.print("adjust_humid");
  Serial.println(adjust_humid);
    
  if(WiFi.status() == WL_CONNECTED){
    
    status = "online";
    timeClient.update();
    Serial.print("timeClient.getMinutes()"); Serial.println(timeClient.getMinutes());
    if(SEND_LINE==false && timeClient.getMinutes()==0){

      String msg = UNIT + "\n";
      msg += "●Flag: " + flag + "\n";
      msg += "●Temp: " + String(temp) + "°C\n";
      msg += "●Humidity: " + String(humid) + "%\n";
      msg += "●Train/Rest: " + String(train) + "/" + String(rest) + " min.\n";
      msg += "●Water: " + String(water) + " L/hr\n";
            
      LINE.setToken(LINE_TOKEN1);
      LINE.notify(msg);
      LINE.setToken(LINE_TOKEN2);
      LINE.notify(msg);
      LINE.setToken(UNIT);
      
      SEND_LINE = true;

    }
    if(SEND_DATA==false && timeClient.getMinutes()%10==0){
      int httpResponseCode = update_api(DEVICE_ID, temp, humid, hic, flag);
      if(httpResponseCode != 201){
        status = "severfail";
      }
      SEND_DATA = true;
      
    }
    if(timeClient.getMinutes()%10!=0){
      SEND_LINE = false;
      SEND_LINE = false;

    }
  }
  else{
    status = "offline";
  }

  //update screen
  updateScreen(flag,temp,humid,status,bat_percentage);

  wm.process();
  delay(5000);

}
