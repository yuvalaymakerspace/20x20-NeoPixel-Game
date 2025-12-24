

#include <FastLED.h>
#include "include.h"

struct CRGB leds[NUM_LEDS_MAX];

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  pinMode(SW_Pin, INPUT);
  digitalWrite(SW_Pin, HIGH);
  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds,NUM_LEDS_MAX);

  FastLED.setBrightness(MaxBright);
  Serial.begin(9600);
  Serial.print("Setup 1");
  snakeList = NULL;
  SnakeInit();
  Serial.print("Setup 2");
  clearAllLed();
  Serial.print("Setup 3");
}

void displaySnake(int val){
  void * SnakePtr = snakeList;
  
  int Red = 0;
  int Green =0;
  unsigned char tmpx = 0;
  unsigned char LedNum = 0;
  //highlighting food

  if(FoodChange){
    if( (FoodPos.yPos%2) == 0)
      tmpx = FoodPos.xPos;
    else
      tmpx = abs(FoodPos.xPos - 9);
    LedNum = (FoodPos.yPos * 10) + tmpx; 
    if(ResetTimer == 0){
      leds[LedNum].setRGB(100,150,50);
    }
    else {
      leds[LedNum].setRGB(0,0,0);
    }
  }
  
  if(ResetTimer == 0){
    Green = val;
  }
  else{
    Red = val;
  }
  
  if(val == 0){
    void * tmpPtr = NULL;
    while(SnakePtr != NULL)
    {
      tmpPtr = SnakePtr;
      SnakePtr = GetNext(SnakePtr);
    }
    SnakePtr = tmpPtr;
  }
  if(LED_DBG)
    Serial.print("\n Led Num :");
  while(SnakePtr != NULL)
  {    
    Cordinates * Cordptr = GetData(SnakePtr);
    if( (Cordptr->yPos%2) == 0)
      tmpx = Cordptr->xPos;
    else
      tmpx = abs(Cordptr->xPos - 9);
    
    LedNum = (Cordptr->yPos * 10) + tmpx;
    if(LED_DBG) {
    Serial.print(LedNum); 
    Serial.print(" ");
    }
     
    if(LedNum < NUM_LEDS_MAX){
      leds[LedNum].setRGB(Red,Green,0);
    }

    SnakePtr = GetNext(SnakePtr);
  }
}

void clearAllLed()
{
  for(int i =0 ; i<NUM_LEDS_MAX ;i ++)
    leds[i].setRGB(0,0,0);
}

void loop() {
  // put your main code here, to run repeatedly:

  reset = digitalRead(SW_Pin);
  X_VAL = analogRead(X_Pin);
  Y_VAL = analogRead(Y_Pin);
  if(CON_DBG) {
  Serial.print("\nSwitch :");
  Serial.print(reset);
  Serial.print(" X :");
  Serial.print(X_VAL);
  Serial.print(" Y :");
  Serial.print(Y_VAL);
  }
  
  SetDirection(X_VAL,Y_VAL,reset);
  displaySnake(0);
  FastLED.show();
  if( ResetTimer == 0){
    MoveAhead();
  }
  else{
    ResetTimer--;
    if(ResetTimer == 0){
      clearAllLed();
      SnakeInit();
    }      
  }
  displaySnake(250);
  FastLED.show();
  delay(500); 

}
