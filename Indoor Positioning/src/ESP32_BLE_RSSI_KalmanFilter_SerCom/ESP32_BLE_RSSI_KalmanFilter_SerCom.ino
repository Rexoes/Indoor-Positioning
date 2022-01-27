#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Kalman.h>

//Devices SSID Values
#define ESP32_STA_SSID "STA4"
#define ESP32_AP_SSID  "AP1"
boolean ENABLE = false;

//BLE RSSI Hesaplama Değişkenleri
int scanTime = 3;
int count = 0;
int toplam = 0; //Histogram RSSI Hesaplama
double distance = 0.0, MeasuredPower = -69;
int ESP32_RSSI = 0;
int rssi_array[50]; // Index0 -> -50 dBm Index1 -> -51dBm
int indis = 0;

//Kalman Filter Değişkenler
int filteredMeasurement;
Kalman myFilter(0.125,32,1023,0); //suggested initial values for high noise filtering
int toplamKalman = 0;
int RSSI_KALMAN = 0;

//Serial Haberleşme Değişkenleri
char inChar;
String valueFromPC = "";
int countSerial = 0;
boolean completeReading = false;

#define nodeMCU Serial2
String valueFromNodeMCU = "";
boolean completeReadingNodeMCU = false;

//BLE RSSI Callback Metodu
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      if(String(advertisedDevice.getName().c_str())=="ACCESPOİNT"){
        Serial.printf("Cihazın mac adresi : %s", advertisedDevice.getAddress().toString().c_str());
        Serial.print(" RSSI Degeri: ");
        Serial.println(advertisedDevice.getRSSI());
        
        filteredMeasurement = myFilter.getFilteredValue(advertisedDevice.getRSSI());
        Serial.println("Kalman filtresi sonucu rssi değeri ="+String(filteredMeasurement));
        toplamKalman+=myFilter.getFilteredValue(advertisedDevice.getRSSI());;
        indis = abs(advertisedDevice.getRSSI()) - 50;// -52dBm -> 52 - 50 = 2. indis ++
        rssi_array[indis]++;
        count++;
        toplam += advertisedDevice.getRSSI();
      }
    }
};

void setup() {
  Serial.begin(115200);
  nodeMCU.begin(115200);
  BLEDevice::init("");
  delay(2000);
}

void loop() {
  serialCommunication();        //Seri Haberleşme Arayüzünden gelen değeri nodeMCU'ya bas.
  if(completeReading == true){
    //Serial.println("Gelen Veri : " + valueFromPC);
    nodeMCU.println(valueFromPC);
  }
  delay(10);
  
  nodeMCU_Communication();            //ESP8266'nın MQTT'den aldığı veriyi seri haberleşme ile ESP32 ye basar.
  if(completeReadingNodeMCU == true){
    Serial.println("ESP8266 NodeMCU'dan Gelen Veri : " + String(valueFromNodeMCU));
    if(valueFromNodeMCU[0] == 'E' && valueFromNodeMCU[1] == 'N' && valueFromNodeMCU[2] == 'B'){
      if(valueFromNodeMCU[3] == '1'){
        ENABLE = true;
        Serial.println("BLE RSSI SCAN ENABLE");
      }
      else if(valueFromNodeMCU[3] == '0'){
        ENABLE = false;
        Serial.println("BLE RSSI SCAN DISABLE");
      }
    }
  }
  if(ENABLE == true){         //BLE RSSI Scan Aktif Edildiyse
    //BLE RSSI Hesaplama
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(scanTime);
  
    if(count == 0){
      Serial.println("Ortalama RSSI Deger : -");
      ESP32_RSSI = 0;
      RSSI_KALMAN = 0;
      Serial.println("Distance : - m");
      delay(10);
      nodeMCU.println(String(ESP32_STA_SSID) + "," + String(ESP32_AP_SSID) + "," + String(RSSI_KALMAN));
    }
    else{
      Serial.println("Aritmetik Ortalama RSSI Deger : " + String(toplam/count));
      //Serial.println("Utku's RSSI Deger : " + String(maxIndexFind() + 50));
      Serial.println("Kalman Ortalama RSSI Deger : " + String(toplamKalman/count));
      RSSI_KALMAN = toplamKalman / count;
      nodeMCU.println(String(ESP32_STA_SSID)+","+String(ESP32_AP_SSID)+","+String(RSSI_KALMAN));
  //    ESP32_RSSI = toplam / count;
  //    double temp = (MeasuredPower - ESP32_RSSI)/10.0/2.0 ;
  //    distance = pow(10, temp);
      //Serial.println("Distance : " + String(distance) + " m");
    }
    count = 0;
    toplam = 0;
    toplamKalman=0;
  }
  delay(20);
}

void nodeMCU_Communication(){
  if(completeReadingNodeMCU == true){
    countSerial = 0;
    valueFromNodeMCU = "";
  }
  completeReadingNodeMCU = false;
  if(nodeMCU.available() > 0){
    inChar = nodeMCU.read();
    if(inChar != '\n'){
      valueFromNodeMCU += inChar;
      countSerial += 1;
    }
    else{
      completeReadingNodeMCU = true;
    }
  }
}

void serialCommunication(){
  if(completeReading == true){
    countSerial = 0;
    valueFromPC = "";
  }
  completeReading = false;
  if(Serial.available() > 0){
    inChar = Serial.read();
    if(inChar != '\n'){
      valueFromPC += inChar;
      countSerial += 1;
    }
    else{
      completeReading = true;
    }
  }
}
