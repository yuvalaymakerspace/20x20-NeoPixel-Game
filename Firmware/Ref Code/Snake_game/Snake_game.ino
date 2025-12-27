
/********************************************************************************************
 * Project Name : LED Matrix Snake Game (20x20) — ESP32 FreeRTOS Edition
 * Platform     : ESP32 + WS2811 / WS2812 LED Matrix
 * Author       : Prince Sharma
 * Organization : Yuvalay Makerspace
 * Firmware Version : v3.0.0
 * Last Updated : 28-Dec-2025
 *
 * Description :
 * This firmware implements an advanced Snake Game on a 20x20 LED matrix using
 * FreeRTOS multitasking for smooth gameplay and responsive joystick input.
 *
 * Key Features :
 *  - Two-task architecture (Input Task + Game Task)
 *  - Start button controlled gameplay state
 *  - Dynamic game speed scaling based on snake length
 *  - Bonus food with countdown-based score reward
 *  - Blinking snake animation on Game Over
 *  - Screen clear on restart to prevent flicker
 *
 * FreeRTOS Architecture :
 *  InputTask()
 *    - Reads joystick periodically
 *    - Debounces start button
 *    - Writes intended direction into g_nextDirection
 *
 *  GameTask()
 *    - Moves snake
 *    - Handles collisions and score logic
 *    - Draws pixels and controls timing
 *
 * Bonus Food Logic :
 *  - Appears every 5 normal food pickups
 *  - Stays active for limited time
 *  - Score depends on pickup speed:
 *      — Early pickup   = +5 points
 *      — Medium delay   = +3 points
 *      — Late pickup    = +2 points
 *
 * Dynamic Speed :
 *  The snake moves faster as it grows.
 *
 * Hardware :
 *  - ESP32 Dev Board
 *  - WS2811 / WS2812 LED Matrix (20x20)
 *  - Analog Joystick (X/Y)
 *  - Start Button (active-LOW)
 *
 * Dependencies :
 *  - FastLED
 *  - FreeRTOS (native ESP32)
 *
 * Revision History :
 *  v3.0.0  Added RTOS tasks, bonus scoring, and documentation
 *  v2.2.0  Added brightness control + ADC scaling
 *  v2.0.0  Added Start/Restart button logic
 *  v1.0.0  Initial LED matrix snake implementation
 ********************************************************************************************/

#include <Arduino.h>
#include <FastLED.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

/******************************** MATRIX CONFIGURATION ********************************/
#define NUM_LEDS        400
#define DATA_PIN        19
#define MATRIX_WIDTH    20
#define MATRIX_HEIGHT   20
#define LED_BRIGHTNESS  255

/******************************** SPEED CONTROL ********************************/
#define BASE_MOVE_DELAY_MS  200   // Initial movement delay
#define SPEED_STEP_MS         3   // Speed increase per snake segment
#define MIN_MOVE_DELAY_MS    50   // Hard lower-limit to avoid instability

/******************************** GAME-OVER VISUAL EFFECT ********************************/
#define GAME_OVER_BLINK_TIME_MS 3000
#define GAME_OVER_BLINK_PERIOD  300

/******************************** BONUS FOOD SETTINGS ********************************/
#define BONUS_FOOD_TIMEOUT_MS 7000

/******************************** JOYSTICK INPUT ********************************/
#define ANALOG_X_PIN 35
#define ANALOG_Y_PIN 34
#define GAME_BUTTON_PIN 13

// Calibration offsets — joystick resting position
#define ANALOG_X_CORRECTION 131
#define ANALOG_Y_CORRECTION 126

// Dead-zone filtering band
#define DEAD_MIN  -53
#define DEAD_MAX   54

/******************************** DATA TYPES ********************************/
typedef struct {
  uint8_t x;
  uint8_t y;
} Position_t;

typedef enum {
  DIR_LEFT = 0,
  DIR_RIGHT,
  DIR_UP,
  DIR_DOWN
} Direction_t;

#define MAX_SNAKE_LEN 100

typedef struct {
  uint8_t length;
  Direction_t direction;
  Position_t body[MAX_SNAKE_LEN];
} Snake_t;

/******************************** GLOBAL VARIABLES ********************************/
CRGB leds[NUM_LEDS];
Snake_t snake;
Position_t food, bonusFood;

volatile bool game_Status = false;            // Game running / idle state
volatile Direction_t g_nextDirection;         // Direction set by InputTask

uint16_t score = 0;
uint8_t normalFoodCount = 0;

bool bonusActive = false;

/******************************** BONUS TIMER ********************************/
TimerHandle_t bonusTimer;

/******************************** FUNCTION PROTOTYPES ********************************/
uint16_t get_led_number(Position_t pos);

void init_snake(void);
void move_snake(void);
void draw_snake(CRGB color);
void clear_snake(void);
void clear_Screen(void);

void spawn_food(void);
void spawn_bonus_food(void);
bool is_food_on_snake(Position_t p);
bool check_self_collision(void);

void gameOverBlink(void);

void bonusTimerCallback(TimerHandle_t xTimer);

Direction_t read_joystick_direction(void);
bool is_reverse_direction(Direction_t cur, Direction_t next);

byte readAnalogAxisLevel(int pin);
uint16_t getSnakeSpeedDelay(void);

/******************************** INPUT TASK ********************************
 * Purpose :
 *  - Read joystick periodically
 *  - Debounce and monitor Start button
 *  - Update requested direction safely
 *
 * Runs independently from game logic (non-blocking)
 *********************************************************************************/
void InputTask(void *pv) {
  while (1) {

    // Start game when button is pressed
    if (!game_Status && digitalRead(GAME_BUTTON_PIN) == LOW) {
      vTaskDelay(pdMS_TO_TICKS(50));   // debounce
      if (digitalRead(GAME_BUTTON_PIN) == LOW)
        game_Status = true;
    }

    // Read joystick direction
    Direction_t newDir = read_joystick_direction();

    // Prevent instant reverse direction
    if (!is_reverse_direction(snake.direction, newDir))
      g_nextDirection = newDir;

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

/******************************** GAME TASK ********************************
 * Purpose :
 *  - Handles gameplay logic and motion
 *  - Detects collision events
 *  - Manages scoring and bonus food
 *  - Draws LED frames and timing
 *********************************************************************************/
void GameTask(void *pv) {
  while (1) {

    if (!game_Status) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    clear_snake();

    // Apply direction provided by InputTask
    snake.direction = g_nextDirection;
    move_snake();

    /*************** SELF COLLISION → GAME OVER ***************/
    if (check_self_collision()) {

      Serial.println("===== GAME OVER =====");
      Serial.print("FINAL SCORE: ");
      Serial.println(score);

      gameOverBlink();         // Game over animation
      xTimerStop(bonusTimer,0);

      clear_Screen();
      FastLED.show();

      // Reset gameplay state
      init_snake();
      spawn_food();

      score = 0;
      normalFoodCount = 0;
      bonusActive = false;
      game_Status = false;

      draw_snake(CRGB::Green);
      leds[get_led_number(food)] = CRGB::Red;
      FastLED.show();
      continue;
    }

    /*************** NORMAL FOOD PICKUP ***************/
    if (snake.body[0].x == food.x && snake.body[0].y == food.y) {

      if (snake.length < MAX_SNAKE_LEN)
        snake.length++;

      score++;
      normalFoodCount++;

      // Spawn bonus food after every 5 fruits
      if (normalFoodCount % 5 == 0)
        spawn_bonus_food();

      spawn_food();
    }

    /*************** BONUS FOOD PICKUP ***************/
    if (bonusActive &&
        snake.body[0].x == bonusFood.x &&
        snake.body[0].y == bonusFood.y) {

      // Compute how long player took to reach bonus food
      TickType_t remaining =
        xTimerGetExpiryTime(bonusTimer) - xTaskGetTickCount();

      uint32_t elapsed =
        BONUS_FOOD_TIMEOUT_MS - (remaining * portTICK_PERIOD_MS);

      uint8_t bonusScore;

      if (elapsed <= 2000)       bonusScore = 5; // Fast pickup
      else if (elapsed <= 4000)  bonusScore = 3; // Medium delay
      else                       bonusScore = 2; // Late pickup

      score += bonusScore;

      // Slightly shrink snake as challenge reward
      if (snake.length > 3)
        snake.length--;

      bonusActive = false;
      xTimerStop(bonusTimer, 0);
    }

    /*************** DRAW FRAME ***************/
    draw_snake(CRGB::Green);
    leds[get_led_number(food)] = CRGB::Red;

    if (bonusActive)
      leds[get_led_number(bonusFood)] = CRGB::Blue;

    FastLED.show();

    vTaskDelay(pdMS_TO_TICKS(getSnakeSpeedDelay()));
  }
}

/******************************** BONUS TIMER CALLBACK ********************************
 * Triggered when bonus food timeout expires
 * Removes bonus food safely
 *********************************************************************************/
void bonusTimerCallback(TimerHandle_t xTimer) {
  bonusActive = false;
  leds[get_led_number(bonusFood)] = CRGB::Black;
  FastLED.show();
}

/******************************** SETUP ********************************/
void setup() {
  Serial.begin(9600);

  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();

  pinMode(GAME_BUTTON_PIN, INPUT_PULLUP);

  randomSeed(esp_random());

  init_snake();
  spawn_food();
  g_nextDirection = snake.direction;

  draw_snake(CRGB::Green);
  leds[get_led_number(food)] = CRGB::Red;
  FastLED.show();

  // Create single-shot timer for bonus food timeout
  bonusTimer = xTimerCreate(
    "BonusTimer",
    pdMS_TO_TICKS(BONUS_FOOD_TIMEOUT_MS),
    pdFALSE,
    NULL,
    bonusTimerCallback);

  // Run input on Core 0 (fast + responsive)
  xTaskCreatePinnedToCore(InputTask, "InputTask", 2048, NULL, 2, NULL, 0);

  // Run game logic on Core 1
  xTaskCreatePinnedToCore(GameTask, "GameTask", 4096, NULL, 1, NULL, 1);
}

void loop() {}   // Empty — system runs fully under FreeRTOS

/******************************** GAME LOGIC ********************************/
void init_snake(void) {
  snake.length = 3;
  snake.direction = DIR_RIGHT;

  snake.body[0] = {1,1};
  snake.body[1] = {1,2};
  snake.body[2] = {1,3};
}

/**
 * Moves snake forward and wraps boundaries
 */
void move_snake(void) {

  // Shift body segments
  for (int i = snake.length - 1; i > 0; i--)
    snake.body[i] = snake.body[i - 1];

  Position_t *h = &snake.body[0];

  switch (snake.direction) {
    case DIR_RIGHT: h->x++; break;
    case DIR_LEFT:  h->x--; break;
    case DIR_UP:    h->y--; break;
    case DIR_DOWN:  h->y++; break;
  }

  // Screen wrap-around logic
  if (h->x < 1) h->x = MATRIX_WIDTH;
  if (h->x > MATRIX_WIDTH) h->x = 1;
  if (h->y < 1) h->y = MATRIX_HEIGHT;
  if (h->y > MATRIX_HEIGHT) h->y = 1;
}

/**
 * Spawns regular food at random position
 */
void spawn_food(void) {
  do {
    food.x = random(1, MATRIX_WIDTH + 1);
    food.y = random(1, MATRIX_HEIGHT + 1);
  } while (is_food_on_snake(food));
}

/**
 * Spawns bonus food and starts timer
 */
void spawn_bonus_food(void) {
  do {
    bonusFood.x = random(1, MATRIX_WIDTH + 1);
    bonusFood.y = random(1, MATRIX_HEIGHT + 1);
  } while (is_food_on_snake(bonusFood));

  bonusActive = true;

  xTimerStop(bonusTimer, 0);
  xTimerStart(bonusTimer, 0);
}

/**
 * Returns true if position overlaps snake body
 */
bool is_food_on_snake(Position_t p) {
  for (int i = 0; i < snake.length; i++)
    if (snake.body[i].x == p.x && snake.body[i].y == p.y)
      return true;
  return false;
}

/**
 * Detects self-collision with snake body
 */
bool check_self_collision(void) {
  for (int i = 1; i < snake.length; i++)
    if (snake.body[0].x == snake.body[i].x &&
        snake.body[0].y == snake.body[i].y)
      return true;
  return false;
}

/**
 * Dynamic speed scaling based on snake size
 */
uint16_t getSnakeSpeedDelay(void) {
  int d = BASE_MOVE_DELAY_MS - snake.length * SPEED_STEP_MS;
  return (d < MIN_MOVE_DELAY_MS) ? MIN_MOVE_DELAY_MS : d;
}

/**
 * Draw snake body with selected color
 */
void draw_snake(CRGB color) {
  for (int i = 0; i < snake.length; i++)
    leds[get_led_number(snake.body[i])] = color;
}

void clear_snake(void) {
  for (int i = 0; i < snake.length; i++)
    leds[get_led_number(snake.body[i])] = CRGB::Black;
}

void clear_Screen(void) {
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CRGB::Black;
}

/**
 * Game over blinking animation
 */
void gameOverBlink(void) {
  uint32_t t = 0;
  bool on = true;

  while (t < GAME_OVER_BLINK_TIME_MS) {
    if (on) draw_snake(CRGB::Red);
    else clear_snake();

    FastLED.show();

    on = !on;
    vTaskDelay(pdMS_TO_TICKS(GAME_OVER_BLINK_PERIOD));
    t += GAME_OVER_BLINK_PERIOD;
  }
}

/******************************** INPUT / CONTROL ********************************/
Direction_t read_joystick_direction(void) {
  short x = readAnalogAxisLevel(ANALOG_X_PIN) - ANALOG_X_CORRECTION;
  short y = readAnalogAxisLevel(ANALOG_Y_PIN) - ANALOG_Y_CORRECTION;

  if (x < DEAD_MIN && y > DEAD_MIN && y < DEAD_MAX) return DIR_DOWN;
  if (x > DEAD_MAX && y > DEAD_MIN && y < DEAD_MAX) return DIR_UP;
  if (y < DEAD_MIN && x > DEAD_MIN && x < DEAD_MAX) return DIR_RIGHT;
  if (y > DEAD_MAX && x > DEAD_MIN && x < DEAD_MAX) return DIR_LEFT;

  return snake.direction;
}

/**
 * Prevent opposite direction reversal
 */
bool is_reverse_direction(Direction_t c, Direction_t n) {
  return ((c == DIR_UP && n == DIR_DOWN) ||
          (c == DIR_DOWN && n == DIR_UP) ||
          (c == DIR_LEFT && n == DIR_RIGHT) ||
          (c == DIR_RIGHT && n == DIR_LEFT));
}

/**
 * ESP32 ADC 0–4095 → 0–255 scaling
 */
byte readAnalogAxisLevel(int pin) {
  return map(analogRead(pin), 0, 4095, 0, 255);
}

/******************************** LED INDEX MAPPING ********************************/
uint16_t get_led_number(Position_t pos) {
  uint8_t x0 = pos.x - 1;
  uint8_t y0 = pos.y - 1;

  return (pos.y & 1)
           ? (y0 * MATRIX_WIDTH + x0)
           : (y0 * MATRIX_WIDTH + MATRIX_WIDTH - 1 - x0);
}
