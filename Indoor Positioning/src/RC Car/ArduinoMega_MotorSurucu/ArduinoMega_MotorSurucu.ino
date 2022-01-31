#include <AFMotor.h>

AF_DCMotor sol_on(1); 
AF_DCMotor sol_arka(4); 
AF_DCMotor sag_on(3);
AF_DCMotor sag_arka(2);

char inChar;              //Serial haberleşmeden gelen veriyi kararkter bazlı okumak için
String stringValue = "";  //Serial haberleşmeden gelen veriyi kararkter bazlı okumak için
short int len = 0;        //UART'tan gelen verinin byte uzunluğu
boolean completeReading = false; //Serial Buffer üzerindeki bütün değerlerin okunduğunu anlamak için kullanılan bool değişken

String valueFromNodeMCU = "";
boolean completeReadingNodeMCU = false;

#define nodeMCU Serial2
short int hiz = 210;
short int Saghiz = hiz + 45;
String aracHareket = "Dur";

struct Coordinate{
  int x;
  int y;
};
struct Alan{
  int xMin;
  int xMax;
  int yMin;
  int yMax;
};
Coordinate coordinate = {0, 0};
Alan       alan       = {0,0,0,0};

//Dikey Hareketler;
//F -> Forward
//B -> Backward
//R -> Right
//L -> Left
//S -> Stop
//Çapraz Hareketler;
//1 -> Left Diagonal Forward
//3 -> Right Diagonal Forward
//7 -> Left Diagonal Backward
//9 -> Right Diagonal Backward
//* -> Turn Around Clock Wise
//# -> Turn Around Anti Clock Wise

//GND Hatları Ortak Bağla

void setup() {
  Serial.begin(115200);
  nodeMCU.begin(115200);
  Stop();
  delay(1000);
}
void loop() {
   serialCommunication();
  if(completeReading == true){
    Serial.print("Gelen Veri : ");
    Serial.println(stringValue);
    if(stringValue.equals("forward")){
      forward();
      aracHareket = "Ileri";
    }
    else if(stringValue.equals("backward")){
      backward();
      aracHareket = "Geri";
    }
    else if(stringValue.equals("right")){
      right();
      aracHareket = "Sag";
    }
    else if(stringValue.equals("left")){
      left();
      aracHareket = "Sol";
    }
    else if(stringValue.equals("ClockWise")){
      turnAroundClockWise();
    }
    else if(stringValue.equals("AntiClockWise")){
      turnAroundAntiClockWise();
    }
    else if(stringValue.equals("Stop")){
      Stop();
    }
  }
  delay(10);
  nodeMCUCommunication();
  if(completeReadingNodeMCU == true){
    Serial.println("NodeMCU'dan Gelen Veri : " + valueFromNodeMCU);
    if(valueFromNodeMCU.substring(0,6) == "SETPWM"){
      hiz = String(String(valueFromNodeMCU[6]) + String(valueFromNodeMCU[7]) + String(valueFromNodeMCU[8])).toInt();
      if(hiz > 210){
        hiz = 210;
      }
      else if(hiz < 0){
        hiz = 0;
      }
      Saghiz = hiz + 45;
    }
    else if(valueFromNodeMCU.substring(0,4) == "left"){
      left();
      aracHareket = "Sol";
    }
    else if(valueFromNodeMCU.substring(0,5) == "right"){
      right();
      aracHareket = "Sag";
    }
    else if(valueFromNodeMCU.substring(0,7) == "forward"){
      forward();
      aracHareket = "Ileri";
    }
    else if(valueFromNodeMCU.substring(0,8) == "backward"){
      backward();
      aracHareket = "Geri";
    }
    else if(valueFromNodeMCU.substring(0,4) == "stop"){
      Stop();
      aracHareket = "Dur";
    }
    else if(valueFromNodeMCU.substring(0,8) == "solileri"){
      leftDiagonalForward();
      aracHareket = "Sol Ileri";
    }
    else if(valueFromNodeMCU.substring(0,7) == "solgeri"){
      leftDiagonalBackward();
      aracHareket = "Sol Geri";
    }
    else if(valueFromNodeMCU.substring(0,8) == "sagileri"){
      rightDiagonalForward();
      aracHareket = "Sag Ileri";
    }
    else if(valueFromNodeMCU.substring(0,7) == "saggeri"){
      rightDiagonalBackward();
      aracHareket = "Sag Geri";
    }
    else if(valueFromNodeMCU.substring(0,10) == "COORDINATE"){
      delay(100);
      int index = 10;
      String xx = "";
      String yy = "";
      while(true){
        if(valueFromNodeMCU[index] == ','){
          index++;
          break;
        }
        else{
          xx += valueFromNodeMCU[index];
          index++;
        }
      }
      coordinate.x = xx.toInt();
      while(true){
        if(valueFromNodeMCU[index] == '\0'){
          index++;
          break;
        }
        else{
          yy += valueFromNodeMCU[index];
          index++;
        }
      }
      coordinate.y = yy.toInt();
    }
    else if(valueFromNodeMCU.substring(0,4) == "ALAN"){
      String xMin = "", xMax = "", yMin = "", yMax = "";
      int index = 4;
      while(true){//xMin
        if(valueFromNodeMCU[index] == ','){
          index++;
          break;
        }
        else{
          xMin += valueFromNodeMCU[index];
          index++;
        }
      }
      while(true){//xMax
        if(valueFromNodeMCU[index] == ','){
          index++;
          break;
        }
        else{
          xMax += valueFromNodeMCU[index];
          index++;
        }
      }
      while(true){//yMin
        if(valueFromNodeMCU[index] == ','){
          index++;
          break;
        }
        else{
          yMin += valueFromNodeMCU[index];
          index++;
        }
      }
      while(true){//yMax
        if(valueFromNodeMCU[index] == '\0'){
          index++;
          break;
        }
        else{
          yMax += valueFromNodeMCU[index];
          index++;
        }
      }
      alan.xMin = xMin.toInt();
      alan.xMax = xMax.toInt();
      alan.yMin = yMin.toInt();
      alan.yMax = yMax.toInt();
    }
    nodeMCU.println(aracHareket + " Hiz : " + String(hiz) + " Koordinat -> X : " + String(coordinate.x) + " Y : " + String(coordinate.y) + "Alan -> Xmin : " + String(alan.xMin) + " Xmax : " + String(alan.xMax) + " Ymin : " + String(alan.yMin) + " Ymax : " + String(alan.yMax));
  }
  delay(10);
}

void nodeMCUCommunication(){
  if(completeReadingNodeMCU == true){
    len = 0;
    valueFromNodeMCU = "";
  }
  completeReadingNodeMCU = false;
  if(nodeMCU.available() > 0){ //Megaya Göre Serial Portu Ayarla
    inChar = nodeMCU.read();
    if(inChar != '\n'){
      valueFromNodeMCU += inChar;
      len += 1;
    }
    else{
      completeReadingNodeMCU = true;
    }
  }
}

void serialCommunication(){
  if(completeReading == true){
    len = 0;
    stringValue = "";
  }
  completeReading = false;
  if(Serial.available() > 0){ //Megaya Göre Serial Portu Ayarla
    inChar = Serial.read();
    if(inChar != '\n'){
      stringValue += inChar;
      len += 1;
    }
    else{
      completeReading = true;
    }
  }
}

                                                            //setSpeed() metodu hız kontrol etmek için parametre olarak 0 - 255 arasında int veri alır. Bunun sebebi 8 bitlik sürücü olmasıdır
                                                            //run() metodu motorun yönünü kontrol etmek için parametre olarak enum veri tipindeki FORWARD BACKWARD ve RELEASE parametrelerini alır.
void forward()              
{
  Serial.println("İleri");
  sol_on.setSpeed(hiz); //Define maximum velocity
  sol_on.run(FORWARD); //rotate the motor clockwise
  sol_arka.setSpeed(hiz); //Define maximum velocity
  sol_arka.run(FORWARD); //rotate the motor clockwise
  sag_on.setSpeed(Saghiz);//Define maximum velocity
  sag_on.run(FORWARD); //rotate the motor clockwise
  sag_arka.setSpeed(Saghiz);//Define maximum velocity
  sag_arka.run(FORWARD); //rotate the motor clockwise
}

void backward()
{
  Serial.println("Geri");
  sol_on.setSpeed(hiz); //Define maximum velocity
  sol_on.run(BACKWARD); //rotate the motor anti-clockwise
  sol_arka.setSpeed(hiz); //Define maximum velocity
  sol_arka.run(BACKWARD); //rotate the motor anti-clockwise
  sag_on.setSpeed(Saghiz); //Define maximum velocity
  sag_on.run(BACKWARD); //rotate the motor anti-clockwise
  sag_arka.setSpeed(Saghiz); //Define maximum velocity
  sag_arka.run(BACKWARD); //rotate the motor anti-clockwise
}

void left()
{
  Serial.println("Sol");
  sol_on.setSpeed(255); //Define maximum velocity
  sol_on.run(BACKWARD); //rotate the motor anti-clockwise
  sol_arka.setSpeed(255); //Define maximum velocity
  sol_arka.run(BACKWARD); //rotate the motor anti-clockwise
  sag_on.setSpeed(255); //Define maximum velocity
  sag_on.run(FORWARD);  //rotate the motor clockwise
  sag_arka.setSpeed(255); //Define maximum velocity
  sag_arka.run(FORWARD);  //rotate the motor clockwise
}

void right()
{
  Serial.println("Sag");
  sol_on.setSpeed(255); //Define maximum velocity
  sol_on.run(FORWARD); //rotate the motor clockwise
  sol_arka.setSpeed(255); //Define maximum velocity
  sol_arka.run(FORWARD); //rotate the motor clockwise
  sag_on.setSpeed(255); //Define maximum velocity
  sag_on.run(BACKWARD); //rotate the motor anti-clockwise
  sag_arka.setSpeed(255); //Define maximum velocity
  sag_arka.run(BACKWARD); //rotate the motor anti-clockwise
}

void leftDiagonalForward()
{
  Serial.println("Sol Ileri");
  sag_on.setSpeed(Saghiz);
  sag_on.run(FORWARD);
  sol_arka.setSpeed(hiz-70);
  sol_arka.run(FORWARD);
  sol_on.setSpeed(hiz-70);
  sol_on.run(FORWARD);
  sag_arka.setSpeed(Saghiz);
  sag_arka.run(FORWARD);
}

void leftDiagonalBackward()
{
  Serial.println("Sol Geri");
  sag_on.setSpeed(Saghiz);
  sag_on.run(BACKWARD);
  sol_arka.setSpeed(hiz-70);
  sol_arka.run(BACKWARD);
  sol_on.setSpeed(hiz-70);
  sol_on.run(BACKWARD);
  sag_arka.setSpeed(Saghiz);
  sag_arka.run(BACKWARD);
}

void rightDiagonalForward()
{
  Serial.println("Sag İleri");
  sag_on.setSpeed(Saghiz-70);
  sag_on.run(FORWARD);
  sol_arka.setSpeed(hiz);
  sol_arka.run(FORWARD);
  sol_on.setSpeed(hiz);
  sol_on.run(FORWARD);
  sag_arka.setSpeed(Saghiz-70);
  sag_arka.run(FORWARD); 
}

void rightDiagonalBackward()
{
  Serial.println("Sag Geri");
  sag_on.setSpeed(Saghiz-70);
  sag_on.run(BACKWARD);
  sol_arka.setSpeed(hiz);
  sol_arka.run(BACKWARD);
  sol_on.setSpeed(hiz);
  sol_on.run(BACKWARD);
  sag_arka.setSpeed(Saghiz-70);
  sag_arka.run(BACKWARD);
}

void turnAroundClockWise()//Right
{
  Serial.println("Saat Yönünde Dön");
  sag_on.setSpeed(0);
  sag_on.run(RELEASE);
  sol_arka.setSpeed(0);
  sol_arka.run(RELEASE);
  sol_on.setSpeed(255);
  sol_on.run(FORWARD);
  sag_arka.setSpeed(0); //Bu kısmı değiştir
  sag_arka.run(RELEASE);
}

void turnAroundAntiClockWise()//Left
{
  Serial.println("Saat Yönü Tersine Dön");
  sag_on.setSpeed(255);
  sag_on.run(FORWARD);
  sol_arka.setSpeed(0); //Bu kısmı değiştir
  sol_arka.run(RELEASE);
  sol_on.setSpeed(0);
  sol_on.run(RELEASE);
  sag_arka.setSpeed(0);
  sag_arka.run(RELEASE);
}

void Stop()
{
  Serial.println("Dur");
  sol_on.setSpeed(0); //Define minimum velocity
  sol_on.run(RELEASE); //stop the motor when release the button
  sol_arka.setSpeed(0); //Define minimum velocity
  sol_arka.run(RELEASE); //rotate the motor clockwise
  sag_on.setSpeed(0); //Define minimum velocity
  sag_on.run(RELEASE); //stop the motor when release the button
  sag_arka.setSpeed(0); //Define minimum velocity
  sag_arka.run(RELEASE); //stop the motor when release the button
}
