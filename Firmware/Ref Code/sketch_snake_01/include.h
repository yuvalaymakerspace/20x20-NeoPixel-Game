#ifndef _INCLUDE_H_
#define _INCLUDE_H_


#define MaxHorizontalLed 10
#define MaxVerticalLed 10

#define LED_DT 6
#define COLOR_ORDER RGB
#define LED_TYPE WS2812
#define NUM_LEDS_MAX MaxHorizontalLed*MaxVerticalLed

#define LED_DBG 0
#define CON_DBG 1


const int SW_Pin = 2;
const int X_Pin = 0;
const int Y_Pin = 1;

unsigned char reset = 1;
unsigned int X_VAL = 0;
unsigned int Y_VAL = 0;

unsigned char MaxBright = 200;

#include <stdlib.h>
#include <math.h>
#include "LinkList.c"
#include "Snake.c"

#endif //_INCLUDE_H_
