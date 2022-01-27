/*************************************************** 
  NodeMCU
****************************************************/ 
#include <ESP8266WiFi.h> 
#include "Adafruit_MQTT.h" 
#include "Adafruit_MQTT_Client.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdbool.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
/************************* WiFi Access Point *********************************/ 
#define WLAN_SSID       "AET_2.4GHz" 
#define WLAN_PASS       "16megabit" 
#define MQTT_SERVER      "192.168.1.147" // static ip address
#define MQTT_PORT         1883                    
#define MQTT_USERNAME    "" 
#define MQTT_PASSWORD         "" 
/************ Global State ******************/ 

WiFiClient client; 
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD); 
Adafruit_MQTT_Publish TX = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "ESP32RX"); 
Adafruit_MQTT_Subscribe RX = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "DATETIMETOPIC");
/*************************** Sketch Code ************************************/

unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

int previousTime = 0;
int currentTime = 0;

/***************************** Time Variables *********************************/
const long utcOffsetInSeconds = 3600;
char daysOfTheWeek[7][12] = {"Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi", "Pazar"};
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

boolean ENABLE = true;

void MQTT_connect(); 

void setup() { 
  
 Serial.begin(115200); 
 delay(10); 
 Serial.println(F("RPi-ESP-MQTT")); 
 // Connect to WiFi access point. 
 Serial.println(); Serial.println(); 
 Serial.print("Connecting to "); 
 Serial.println(WLAN_SSID); 
 WiFi.begin(WLAN_SSID, WLAN_PASS); 
 while (WiFi.status() != WL_CONNECTED) { 
   delay(500); 
   Serial.print("."); 
 } 
 Serial.println(); 
 Serial.println("WiFi connected"); 
 Serial.println("IP address: "); Serial.println(WiFi.localIP()); 
 // Setup MQTT subscription for esp8266_led feed. 
 mqtt.subscribe(&RX);
 timeClient.begin();//For getting current time
 timeClient.update();
 previousTime = int(timeClient.getSeconds());
} 
uint32_t x=0; 

void loop() {
  //Zaman Hesaplama
  timeClient.update();
  String gun = daysOfTheWeek[(timeClient.getDay() + 6)%7];
  String saat = String((timeClient.getHours() + 2));
  String dakika = String(timeClient.getMinutes());
  String saniye = String(timeClient.getSeconds());
  if(saniye.toInt() < 10){
    saniye = "0" + saniye;
  }
  if(dakika.toInt() < 10){
    dakika = "0" + dakika;
  }
  if(saat.toInt() < 10){
    saat = "0" + saat;
  }
  //Serial.println("Data Time -> " + String(saat) + " : " + String(dakika) + " : " + String(saniye));
//  currentTime = saniye.toInt();
//  if(currentTime != previousTime){
//    String str = String(saat) + ":" + String(dakika) + ":" + String(saniye);
//    int str_len = str.length() + 1; 
//    char txBuffer[str_len];
//    str.toCharArray(txBuffer, str_len);
//    TX.publish(txBuffer);//parametre char tipinde array olmalı !
//    previousTime = currentTime;
//  }
  currentMillis = millis();
 if(((currentMillis - previousMillis) > 3100) && ENABLE == true){
    String str = "T " + String(gun) + " -> " + String(saat) + ":" + String(dakika) + ":" + String(saniye);
    int str_len = str.length() + 1; 
    char txBuffer[str_len];
    str.toCharArray(txBuffer, str_len);
    TX.publish(txBuffer);//parametre char tipinde array olmalı !
    previousMillis = currentMillis;
 }
  
  
 MQTT_connect(); 
 
 Adafruit_MQTT_Subscribe *subscription; 
 while ((subscription = mqtt.readSubscription())) {
   if (subscription == &RX) {
     char *message = (char *)RX.lastread; 
     Serial.print(F("Got: ")); 
     Serial.println(message);
     String string = String(message);
     if(string == "DATETIMETOPICENABLE" || string == "ENABLE"){
      ENABLE = true;
     }
     else if(string == "DATETIMETOPICDISABLE" || string == "DISABLE"){
      ENABLE = false;
     }
   }
 }
 delay(20);
}//loop end

// Function to connect and reconnect as necessary to the MQTT server. 
void MQTT_connect() {
 int8_t ret; 
 // Stop if already connected. 
 if (mqtt.connected()) { 
   return; 
 } 
 Serial.print("Connecting to MQTT... "); 
 uint8_t retries = 3; 
 while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected 
      Serial.println(mqtt.connectErrorString(ret)); 
      Serial.println("Retrying MQTT connection in 5 seconds..."); 
      mqtt.disconnect(); 
      delay(5000);  // wait 5 seconds 
      retries--; 
      if (retries == 0) { 
        // basically die and wait for WDT to reset me 
        while (1); 
      } 
 } 
 Serial.println("MQTT Connected!"); 
} 
