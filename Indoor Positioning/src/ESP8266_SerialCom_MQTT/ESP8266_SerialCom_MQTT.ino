//Gerekli Kütüphaneler
#include <ESP8266WiFi.h> 
#include "Adafruit_MQTT.h" 
#include "Adafruit_MQTT_Client.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdbool.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

//WiFi ve MQTT Parametreleri
#define WLAN_SSID       "AET_2.4GHz"            //WiFi SSID
#define WLAN_PASS       "16megabit"             //WiFi Password
#define MQTT_SERVER      "192.168.1.147"        //MQTT Static IP Adress
#define MQTT_PORT         1883                  //MQTT Port Adress  
#define MQTT_USERNAME    ""                     //MQTT Server Username
#define MQTT_PASSWORD         ""                //MQTT Server Password

//MQTT Publish (Yazma) ve Subscribe (Okuma) Değişkenleri
WiFiClient client; 
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD); //MQTT Client nesneni oluştur
Adafruit_MQTT_Publish TX = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "ESP32TX");         //MQTT Veri Gönderme Publish Topic Adres
Adafruit_MQTT_Subscribe RX = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "ESP32RX");     //MQTT Veri Okuma Subscribe Topic Adres

/***************************** Time Variables *********************************/
const long utcOffsetInSeconds = 3600;
char daysOfTheWeek[7][12] = {"Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi", "Pazar"};
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

char inChar;
String valueFromArduino = "";
short int countBT = 0;
boolean completeReading = false;

boolean ENABLE = true;
String ESP32_RSSI = "";

void setup() {
  Serial.begin(115200);                     //Seri Haberleşmeyi 115200 baud rate ayarlanıyor.
  //Serial.print("Connecting to "); 
  //Serial.println(WLAN_SSID); 
  WiFi.begin(WLAN_SSID, WLAN_PASS);         //WiFi Connection
  while (WiFi.status() != WL_CONNECTED) {   //Tekrar Bağlan
    delay(500); 
 }
 mqtt.subscribe(&RX);                       //MQTT Dinleme Topic Adresi
 timeClient.begin();                        //Anlık Zaman Bilgisi Al
 delay(1000);
 TX.publish("Kart Calisiyor");
}

void loop() {
  MQTT_connect();         //MQTT Connection
  timeClient.update();    //Time Update
  //Zaman Hesaplama
  String gun = daysOfTheWeek[timeClient.getDay()];
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
  String zamanBilgisi = saat + ":" + dakika + ":" + saniye;//Anlık Zamanı Saklayacak
  
  serialCommunication();
  if(completeReading == true){
//  Serial.println("Gelen Veri : " + valueFromArduino);
    ESP32_RSSI = valueFromArduino;
//    String str = "Gelen Veri : " + String(valueFromArduino) + "," + String(zamanBilgisi);
//    int str_len = str.length() + 1; 
//    char txBuffer[str_len];
//    str.toCharArray(txBuffer, str_len);
//    TX.publish(txBuffer);
  }
  delay(20);
 Adafruit_MQTT_Subscribe *subscription; 
 while ((subscription = mqtt.readSubscription())) {
   if (subscription == &RX) {
     char *message = (char *)RX.lastread;
     String comingValue = String(message);
     Serial.println(comingValue);
     if(comingValue == "STA2ENABLE"){
       Serial.println("ENB1");
       TX.publish("ESP32 STA2 ENABLED");
     }
     else if(comingValue == "ALLSTAENABLE"){
       Serial.println("ENB1");
       TX.publish("ESP32 STA2 ENABLED");
     }
     else if(comingValue == "STA2DISABLE"){
      TX.publish("ESP32 STA2 DISABLED");
     }
     else if(comingValue == "ALLSTADISABLE"){
      TX.publish("ESP32 STA2 DISABLED");
     }
     else if(comingValue[0] == 'T'){
        //Orjinal hali
        String str =String(ESP32_RSSI) + "," + String(zamanBilgisi);
        str = encryiption("AET", str);
        int str_len = str.length() + 1; 
        char txBuffer[str_len];
        str.toCharArray(txBuffer, str_len);
        TX.publish(txBuffer);
     }
//     Serial.print(F("Got: ")); 
//     Serial.println(message);
      //Serial.println("MQTT: " + String(message));
   }
 }
}

void serialCommunication(){
  if(completeReading == true){
    countBT = 0;
    valueFromArduino = "";
  }
  completeReading = false;
  if(Serial.available() > 0){
    inChar = Serial.read();
    if(inChar != '\n'){
      valueFromArduino += inChar;
      countBT += 1;
    }
    else{
      completeReading = true;
    }
  }
}

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

String encryiption(String key, String data){//33-125
  int dataLen = data.length();
  int keyLen = key.length();
  String output = data;
  for(int i = 0; i < dataLen; i++){
    char dataCH = data[i];
    char keyCH = key[i%keyLen];
    output[i] = char((int(dataCH) + int(keyCH))%92 + 33);
  }
  return output;
}

String decryiption(String key, String data){
  int dataLen = data.length();
  int keyLen = key.length();
  String output = data;
  for(int i = 0; i < dataLen; i++){
    char dataCH = data[i];
    char keyCH = key[i%keyLen];
    output[i] = char((int(dataCH) - int(keyCH)) + 59);
  }
  return output;
}
