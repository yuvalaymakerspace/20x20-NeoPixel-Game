#ifndef _SNAKE_FUNC_H_
#define _SNAKE_FUNC_H_

enum _MOVEMET_DIRECTION_ {
  MOVE_RIGHT = 0,
  MOVE_UP,
  MOVE_LEFT,
  MOVE_DOWN,
  MOVE_MAX,
};

enum _VALIDATE_SNAKE_CORDS_ {
  MOVE_AHEAD = 0,
  SELF_BITE,
  FOOD_BITE,
};

typedef struct{
    unsigned char xPos;
    unsigned char yPos;
}Cordinates;

void MoveAhead();
void SnakeInit();
void InsertCurPosNode();
void setNewCords(Cordinates *Cord,unsigned char dir );
int checkNewCords(Cordinates * first);
unsigned char IsSameCords(Cordinates * first, Cordinates * sec);
void GenerateFood();
void ChangeDir(unsigned char dir);
void SetDirection(unsigned int xVal, unsigned int yVal, unsigned char Reset);

#endif //_SNAKE_FUNC_H_
