#ifndef _SNAKE_FUNC_C_
#define _SNAKE_FUNC_C_


#define MaxX 10
#define MaxY 10

#include "snake.h"
#include<stdio.h>
#include<stdlib.h>

void * snakeList;
Cordinates CurPos;
Cordinates FoodPos;
unsigned char Direction;
unsigned char ResetTimer;
unsigned char FoodChange;

void MoveAhead(){
  setNewCords(&CurPos,Direction);
  switch(checkNewCords(&CurPos))
  {
    default:
    case MOVE_AHEAD:
      break;
    case SELF_BITE:
      ResetTimer = 3;
      return;
    case FOOD_BITE:
      InsertCurPosNode();
      GenerateFood();
      return;
  }
  snakeList = RemoveTail(snakeList);
  InsertCurPosNode();  
}

int checkNewCords(Cordinates * Curpos){
  void * SnakePtr = snakeList;

  if(IsSameCords(Curpos, &FoodPos)){
    return FOOD_BITE;
  }
  
  while(SnakePtr != NULL)
  {    
    Cordinates * Cordptr = GetData(SnakePtr);

    if(IsSameCords(Cordptr, Curpos)){
      return SELF_BITE;
    }
    SnakePtr = GetNext(SnakePtr);
  }

  return MOVE_AHEAD;
}

unsigned char IsSameCords(Cordinates * first, Cordinates * sec){
  if(first->yPos == sec->yPos && first->xPos == sec->xPos){
    return 1;
  }
  return 0;
}

void setNewCords(Cordinates *Cord,unsigned char dir )
{
  switch(dir)
  {
    default:
    case MOVE_UP:
      Cord->yPos++;
      if(Cord->yPos >= MaxY){
       Cord->yPos = 0; 
      }
      break;
    case MOVE_RIGHT:
      Cord->xPos--;
      if(Cord->xPos >= MaxX){
       Cord->xPos = MaxX-1; 
      }
      break;
    case MOVE_LEFT:
      Cord->xPos++;
      if(Cord->xPos >= MaxX){
       Cord->xPos = 0; 
      }
      break;
    case MOVE_DOWN:
      Cord->yPos--;
      if(Cord->yPos >= MaxY){
       Cord->yPos = MaxY-1; 
      }
      break;
  }
}

void SnakeInit(){
  while(NULL != snakeList){
      snakeList = RemoveHead(snakeList);
  }
  Direction = MOVE_DOWN;
  /*
  CurPos.xPos = 4;
  CurPos.yPos = 4;
  InsertCurPosNode();
  CurPos.xPos++;
  InsertCurPosNode();
  CurPos.xPos++;
  InsertCurPosNode();
  CurPos.xPos++;
  InsertCurPosNode();
  CurPos.xPos++;
  InsertCurPosNode();
  */
  ResetTimer = 0;
  GenerateFood();
}
void GenerateFood(){
  Cordinates tmp= {rand()%10,rand()%10};
  int i =0;
  int j= 0;
  for(;i<MaxY; i++)
  {
    for(;j<MaxX;j++){
      setNewCords(&tmp, MOVE_RIGHT);
      switch(checkNewCords(&tmp)){
        case MOVE_AHEAD:
          FoodChange =1;
          FoodPos.xPos = tmp.xPos;
          FoodPos.yPos = tmp.yPos;
          return;
        default:
        case SELF_BITE:
        case FOOD_BITE:
          break;
      }
    }
    setNewCords(&tmp, MOVE_UP);
  }
}

void SetDirection(unsigned int xVal,unsigned int yVal, unsigned char Reset)
{
  if(0 == Reset){
    ResetTimer = 3;
    return;
  }

  if(xVal < 100){
    return ChangeDir(MOVE_RIGHT);
  }
  else if(xVal > 800){
    return ChangeDir(MOVE_LEFT);
  }
  else if(yVal > 800){
    return ChangeDir(MOVE_UP);
  }
  else if(yVal < 100){
    return ChangeDir(MOVE_DOWN);
  }
}

void ChangeDir(unsigned char dir){

  if(dir >= MOVE_MAX || Direction == dir)
    return;
  switch(Direction)
  {
    case MOVE_UP:
      if(dir != MOVE_DOWN)
        Direction = dir;
      break;
    case MOVE_RIGHT:
      if(dir != MOVE_LEFT)
        Direction = dir;
       break;
    case MOVE_LEFT:
      if(dir != MOVE_RIGHT)
        Direction = dir;
      break;
    case MOVE_DOWN:
      if(dir != MOVE_UP)
        Direction = dir;
      break;
  }
}

void InsertCurPosNode(){
  Cordinates * tmp = (Cordinates *) malloc(sizeof(Cordinates));
  tmp->xPos = CurPos.xPos;
  tmp->yPos = CurPos.yPos;
  snakeList = InsertAtFirst(snakeList, tmp);  
}
#endif //_SNAKE_FUNC_C_-*
