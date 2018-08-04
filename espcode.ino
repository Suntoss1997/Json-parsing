#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <HX711.h>
#include <SimpleDHT.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <ESP8266HTTPUpdateServer.h>



const char* ssid = "Smart";
const char* wifiPassword = "1234567890";


//define ticker;
Ticker flipper;



//Define webserver
ESP8266WebServer server ( 80 );
ESP8266HTTPUpdateServer httpUpdater;

//Define Scale 1
const byte scale1DataPin = 12;
const byte scale1ClockPin =13 ;
HX711 scale1(scale1DataPin, scale1ClockPin);

//Define scale 2
const byte scale2DataPin = 5;
const byte scale2ClockPin = 4;
HX711 scale2(scale2DataPin, scale2ClockPin);


//define dht11 temperature sensor
const byte dht11Pin = 14;
SimpleDHT11 dht11;
byte temperature = 0;
byte humidity = 0;
boolean measureDHTnow = true;

void measureNow(){
   measureDHTnow = true;
  }


//define relay
byte relayPin = 16;
int lampStatus = 0;


//load settings
 int lampSettings;

//array of mintemp
int settingsMinTemp [] = {NULL,NULL,35,32,29,26,23,20,16};
int settingsMaxTemp [] = {NULL,NULL,37,35,32,29,26,23,19};


void lampOn(){
  digitalWrite(relayPin, HIGH);
  }

void lampOff(){
  digitalWrite(relayPin, LOW);
  }
void lampManager(){
/*
 * lamp manager is periodically called by ticker, this function will check 
 * humidity and temperature are store on global variable
 * controls relay pin for maintaining temperature
 * 
 */
 Serial.print("starting lamp manager");
  if (dht11.read(dht11Pin, &temperature, &humidity, NULL)) {
    Serial.print("Read DHT11 failed.");
    return;
  }
  
    Serial.println("temperature read"+temperature);
  if(lampSettings == 0){
    lampOff();
  }else if(lampSettings == 1){
    lampOn();  
  }else{
    if(temperature < settingsMinTemp[lampSettings]){
      lampOn();        
    }else if(temperature > settingsMaxTemp[lampSettings]){
      lampOff();
      }   
   }

}


//root html page
void handleRoot() {
   Serial.println("Root page request received");
  //read scale 1;
  String scale1Reading = String((scale1.read()/1000*16/6));
  //read scale 2;
  String scale2Reading = String((scale2.read()/1000*16/6));
  
  
  Serial.println("weight = " + scale1Reading);
  String body = "{\"temperature\":\""+String(temperature)+"\",\"humidity\":\""+String(humidity)+"\",\"scale_1\":\""+scale1Reading+"\",\"scale_2\":\""+scale2Reading+"\"}";
  server.send(200, "text/plain", body);
}


//lamp settings page
void handleSettings() {

if(server.hasArg("lamp")){
    lampSettings = server.arg("lamp").toInt();
    EEPROM.write(1, lampSettings);
    EEPROM.commit();
    lampManager();
  }
  
  String body = "<h1>Lamp Settings</h1><div><form method='post'><input type='radio' name='lamp' value='0'> Always Off<br><input type='radio' name='lamp' value='1'> Always On<br><input type='radio' name='lamp' value='2'> Week 0 (35&#8451 - 37&#8451)<br><input type='radio' name='lamp' value='3'> Week 1 (32&#8451 - 35&#8451)<br><input type='radio' name='lamp' value='4'> Week 2 (29&#8451 - 32&#8451)<br><input type='radio' name='lamp' value='5'> Week 3 (26&#8451 - 29&#8451)<br><input type='radio' name='lamp' value='6'> Week 4 (23&#8451 - 26&#8451)<br><input type='radio' name='lamp' value='7'> Week 5-7 (20&#8451 - 23&#8451)<br><input type='radio' name='lamp' value='8'> Adult (16&#8451 - 19&#8451)<br><input type='submit' name='Update' value='Update'></form></div><script>document.querySelector('[value=\""+String(EEPROM.read(1))+"\"]').checked = true;</script>";
  server.send ( 200, "text/html", body);
  Serial.print ( "lamp settigns page; lamp setting integer is" );
  Serial.println(lampSettings);
  
  


}

//not found page

void handleNotFound() {
  server.send ( 404, "text/html", "<h2>Web Page not Found</h2>");
}



void setup() {
  
  // start serial communication for debugging
  Serial.begin(115200);
  delay(10);
  Serial.println("Serial communication started");
  WiFi.mode(WIFI_AP);
  delay(10);
  WiFi.softAP(ssid, wifiPassword);
  delay(10);
 Serial.println("Soft ap started");
  //relay
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  //eeeprom to store data
  EEPROM.begin(512);
  lampSettings = EEPROM.read(1);
  //html server
  server.on("/", handleRoot);
  server.on("/settings", handleSettings);
  server.onNotFound ( handleNotFound );
  httpUpdater.setup(&server);
  server.begin();


  
  //flipper
  flipper.attach(30, measureNow);
}

void loop() {
  
  server.handleClient();

  if(measureDHTnow){
  measureDHTnow =false;
  lampManager();
  }

}
