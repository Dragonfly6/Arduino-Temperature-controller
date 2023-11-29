#include <UTFT.h>
#include <ITDB02_Touch.h>

#define aref_voltage 3.3

#define RUNNING 1
#define STOPPED 0

#define HEATING 1
#define COOLING 0

#define ON 1
#define OFF 0

extern uint8_t BigFont[];
extern uint8_t SmallFont[];

UTFT          myGLCD(SSD1289, 38, 39, 40, 41);
ITDB02_Touch  myTouch(6, 5, 4, 3, 2);

int targetT = 20;
int currentT = 0;
int ambientT = 0;

int const maxT = 80;
int const minT = -20;

int const directionPin = 10;
int const switchPin = 11;
int const powerBlockPin = 12;
int const sensorPin = A0; // Peltier sensor input
int const ambientSensorPin = A1; // ambient sensor input


long sendDataTime = 0;
long adjustmentTime = 0;


//modes
int onOffState = STOPPED;
int mode = HEATING;
int pwmPower = 0;
int pwmChangeDelay = 1000;
int powerState = OFF;



void setup() {
  //pins setup
  pinMode(switchPin, OUTPUT);
  //digitalWrite(switchPin, LOW);
  analogWrite(switchPin, pwmPower);
  pinMode(directionPin, OUTPUT);
  digitalWrite(directionPin, LOW);
  Serial.begin(9600);
  sendDataTime = millis();
  analogReference(EXTERNAL);
  float voltage = (analogRead(sensorPin)/1024.0) * aref_voltage;
  
  // LCD setup
  myGLCD.InitLCD(LANDSCAPE);
  myGLCD.clrScr();

  myTouch.InitTouch(LANDSCAPE);
  myTouch.setPrecision(PREC_MEDIUM);

  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(0, 0, 0);
  drawInterface();
  Serial.begin(9600);
  pinMode(powerBlockPin, OUTPUT);
  digitalWrite(powerBlockPin, LOW);
}

void loop() {
  if(abs(currentT - targetT) <= 6 && abs(currentT - targetT) > 2) pwmChangeDelay = 2000;
  else if (abs(currentT - targetT) <= 1) pwmChangeDelay = 4000;
  else pwmChangeDelay = 1000;
  
  while(myTouch.dataAvailable()){
    myTouch.read();
    int y = myTouch.getY();
    int x = myTouch.getX();
    if(y > 0 && y < 40){
      //slider is pressed
      updatePointer(constrain(myTouch.getX(), 10, myGLCD.getDisplayXSize() - 10));
    }
    else if (x > 250 && x < 300){
      if(y > 60 && y < 110){
        //"+" is pressed
        updateTargetT(targetT + 1);
      } else if (y > 120 && y < 170){
        //"-" is pressed
        updateTargetT(targetT - 1);
      }
    } else if(y > 180 && y < 230){
        if(x > 200 && x < 300)
          // start/stop is pressed
          if(onOffState == STOPPED){
            onOffState = RUNNING;
            myGLCD.setColor(0, 255, 0);
            myGLCD.fillRect(200, 180, 300, 230);
            myGLCD.setBackColor(0, 255, 0);
            myGLCD.setColor(0, 0, 255);
            myGLCD.print("Stop", 215, 197, 0);
      
            //reset back color
            myGLCD.setBackColor(0, 0, 0);
            delay(100);
          } else{
            onOffState = STOPPED;
            digitalWrite(switchPin, LOW);
            pwmPower = 0;
            myGLCD.setColor(0, 255, 0);
            myGLCD.fillRect(200, 180, 300, 230);
            myGLCD.setBackColor(0, 255, 0);
            myGLCD.setColor(0, 0, 255);
            myGLCD.print("Start", 215, 197, 0);
      
            //reset back color
            myGLCD.setBackColor(0, 0, 0);
            delay(100);
          }
          else if(x > 140 && x < 190){
            //ON-OFF is pressed
            if(powerState == ON){
              powerState = OFF;
              digitalWrite(powerBlockPin, LOW);
              myGLCD.setColor(100, 100, 100);
              myGLCD.fillRect(140, 180, 190, 230);
              myGLCD.setBackColor(100, 100, 100);
              myGLCD.setColor(0, 0, 255);
              myGLCD.print("OFF", 140, 197, 0);
              
              //reset back color
              myGLCD.setBackColor(0, 0, 0);
              delay(100);
            } else {
              powerState = ON;
              digitalWrite(powerBlockPin, HIGH);
              myGLCD.setColor(100, 100, 100);
              myGLCD.fillRect(140, 180, 190, 230);
              myGLCD.setBackColor(100, 100, 100);
              myGLCD.setColor(0, 0, 255);
              myGLCD.print("ON", 150, 197, 0);
              
              //reset back color
              myGLCD.setBackColor(0, 0, 0);
              delay(100);
            }
          }
    }
  }

  
  
  if(onOffState == RUNNING){
    if(mode == HEATING){
      if(currentT < targetT) {
        if(millis() - adjustmentTime > pwmChangeDelay){
          pwmPower = constrain(pwmPower++, 0, 255);
          analogWrite(switchPin, pwmPower);
          adjustmentTime = millis();
        }
      }
      else if (currentT > targetT){
        if(millis() - adjustmentTime > pwmChangeDelay){
          pwmPower = constrain(pwmPower--, 0, 255);
          analogWrite(switchPin, pwmPower);
          adjustmentTime = millis();
        }
      }      
    } else {
      if(currentT > targetT) {
        if(millis() - adjustmentTime > pwmChangeDelay){
          pwmPower = constrain(pwmPower++, 0, 255);
          analogWrite(switchPin, pwmPower);
          adjustmentTime = millis();
        }
      }
      else if (currentT < targetT){
        if(millis() - adjustmentTime > pwmChangeDelay){
          pwmPower = constrain(pwmPower--, 0, 255);
          analogWrite(switchPin, pwmPower);
          adjustmentTime = millis();
        }
      }
    }
  }

  if((millis() - sendDataTime) > 1000) {
    float voltage = (analogRead(sensorPin)/1024.0) * aref_voltage;
    float alpha = 1.0 * voltage / aref_voltage;
    float resistance = 9920.0 * alpha / (1 - alpha);
    currentT = -252.0  * log(0.09867 * log(resistance));
    Serial.print("mode: ");
    Serial.println(mode);
    Serial.print("PWM: ");
    Serial.println(pwmPower);
    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(10, 160, 135, 190);
    myGLCD.setColor(255, 255, 255);
    myGLCD.setColor(255, 255, 255);
    myGLCD.printNumI(currentT, 10, 160, 3, '0');
    Serial.print("current T: ");
    Serial.println(currentT);
    
   //ambient sensor readings
    voltage = (analogRead(ambientSensorPin)/1024.0) * aref_voltage;
    alpha = 1.0 * voltage / aref_voltage;
    resistance = 9920.0 * alpha / (1 - alpha);
    ambientT = -252.0  *log(0.09867 * log(resistance));
    Serial.print("ambient T: ");
    Serial.println(ambientT);
    Serial.println();
    sendDataTime = millis();
  }
}

void updateTargetT(int T){
  targetT = constrain(T, minT, maxT);
  if(targetT >= ambientT) {
    mode = HEATING;
    digitalWrite(directionPin, HIGH);
  }
  else {
    mode = COOLING;
    digitalWrite(directionPin, LOW);
  }
  
  //hide old pointer and target temperature
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(1, 31, myGLCD.getDisplayXSize()-1, 60);
  
  //update target temperature
  String targetTLabel = "Target T:";
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(targetTLabel, 10, 60, 0);
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(10, 90, 100, 116);
  myGLCD.setColor(255, 255, 255);
  myGLCD.printNumI(targetT, 10, 90);
  
  //update pointer
  int x = map(targetT, minT, maxT, 10, myGLCD.getDisplayXSize() - 10);
  for(int i = 0; i < 10; i++){
    myGLCD.drawLine(x - i, 31 + i, x + i - 1, 31 + i);
  }
}

void drawInterface(){
  drawSlider();
  drawControlButtons();
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Current T:", 10, 130, 0);
  myGLCD.print("Power:", 30, 197, 0);
}

void drawSlider(){
//draw temperature scale
  for(int i = 10; i <= myGLCD.getDisplayXSize() - 10; i++){
    myGLCD.setColor(constrain(i, 0, 255), 0, 255 - constrain(i, 0, 255));
    myGLCD.drawLine(i, 10, i, 30);
  }

//draw pointer
  updateTargetT(targetT);
}

void updatePointer(int x){
  //hide old pointer and target temperature
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(1, 31, myGLCD.getDisplayXSize()-1, 60);
  
  //update target temperature
  targetT = map(x, 10, myGLCD.getDisplayXSize() - 10, minT, maxT);
  
  if(targetT >= ambientT) {
    mode = HEATING;
    digitalWrite(directionPin, HIGH);
  }
  else {
    mode = COOLING;
    digitalWrite(directionPin, LOW);
  }
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Target T:", 10, 60, 0);
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(10, 90, 100, 116);
  myGLCD.setColor(255, 255, 255);
  myGLCD.printNumI(targetT, 10, 90);
  
  //update pointer
  for(int i = 0; i < 10; i++){
    myGLCD.drawLine(x - i, 31 + i, x + i - 1, 31 + i);
  }
}

void drawControlButtons(){
  //"+" button
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(250, 60, 300, 110);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("+", 267, 77, 0);

  //"-" button
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRect(250, 120, 300, 170);
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("-", 267, 137, 0);

  //"start-stop" button
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRect(200, 180, 300, 230);
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("Start", 215, 197, 0);

  //power button
  myGLCD.setColor(100, 100, 100);
  myGLCD.fillRect(140, 180, 190, 230);
  myGLCD.setBackColor(100, 100, 100);
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("OFF", 140, 197, 0);
  
  //reset back color
  myGLCD.setBackColor(0, 0, 0);
}
