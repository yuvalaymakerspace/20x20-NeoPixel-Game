/********************************************************************************************
 * Project Name : LED Matrix Snake Game (20x20)
 * Platform     : Arduino + WS2811 / WS2812 LED Matrix
 * Author       : Prince Kumar
 * Organization : Yuvalay Makerspace
 * Version      : V 1.0.0
 * Last Updated : 25-Dec-2025
 *
 * Description :
 * This firmware implements a simple snake game on a 20x20 LED matrix.
 * The snake moves using an analog joystick and grows whenever it eats food.
 * If the snake collides with itself, the game restarts automatically.
 *
 * Hardware Used :
 *  - 20x20 LED Matrix (WS2811 / WS2812)
 *  - Arduino Board
 *  - Analog Joystick Module
 *
 * Control Scheme :
 *  - Joystick X / Y movement controls snake direction
 *  - Edges are wrapped (snake teleports across borders)
 *
 * Notes :
 *  - Movement speed is configurable using MOVE_DELAY_MS
 *  - Snake wrap-around is enabled instead of wall collision death
 *  - Self collision resets the game
 *
 * Dependencies :
 *  - FastLED Library
 *
 * Revision History :
 *  v1.2.0  Added structured comments, documentation, and professional header
 *  v1.1.0  Added joystick dead-zone handling and direction filtering
 *  v1.0.0  Initial implementation
 ********************************************************************************************/

#include <Arduino.h>
#include <FastLED.h>

/******************************** MATRIX CONFIGURATION ********************************/
#define NUM_LEDS        400
#define DATA_PIN        19
#define MATRIX_WIDTH    20
#define MATRIX_HEIGHT   20
#define MOVE_DELAY_MS   150   // Controls snake movement speed

/******************************** JOYSTICK CONFIGURATION ********************************/
#define ANALOG_X_PIN        35
#define ANALOG_Y_PIN        34
#define GAME_BUTTON_PIN     13   // Start / Restart button (Active LOW)

#define ANALOG_X_CORRECTION 126  // Joystick center trim offset
#define ANALOG_Y_CORRECTION 123

// Dead zone range to prevent joystick noise flicker
#define DEAD_MIN   -53
#define DEAD_MAX    54

/******************************** DATA STRUCTURES ********************************/
typedef struct
{
    uint8_t x;
    uint8_t y;
} Position_t;

typedef enum
{
    DIR_LEFT = 0,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
} Direction_t;

#define MAX_SNAKE_LEN 100

typedef struct
{
    uint8_t     length;
    Direction_t direction;
    Position_t  body[MAX_SNAKE_LEN];
} Snake_t;

/******************************** GLOBAL VARIABLES ********************************/
CRGB leds[NUM_LEDS];
Snake_t snake;
Position_t food;

// Game running state
bool game_Status = false;

/******************************** FUNCTION DECLARATIONS ********************************/
uint16_t get_led_number(Position_t pos);

void init_snake(void);
void move_snake(void);
void draw_snake(void);
void clear_snake(void);
void clear_Screen(void);

void spawn_food(void);
bool is_food_on_snake(Position_t p);
bool check_self_collision(void);

Direction_t read_joystick_direction(void);
bool is_reverse_direction(Direction_t cur, Direction_t next);

byte readAnalogAxisLevel(int pin);

/******************************** SETUP ********************************/
/**
 * Runs once at boot.
 * Initializes LEDs, button, snake, and places food.
 */
void setup()
{
    Serial.begin(9600);

    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();

    pinMode(GAME_BUTTON_PIN, INPUT_PULLUP); // Button = active LOW

    randomSeed(analogRead(A0));

    init_snake();
    spawn_food();

    // Show initial preview state (idle mode)
    draw_snake();
    leds[get_led_number(food)] = CRGB::Red;
    FastLED.show();
}

/******************************** MAIN LOOP ********************************/
void loop()
{
    /*************** GAME WAITING MODE — PRESS BUTTON TO START ***************/
    if (!game_Status)
    {
        bool cur = digitalRead(GAME_BUTTON_PIN);

        // Button debounced check
        if (cur == LOW)
        {
            delay(100);
            if (digitalRead(GAME_BUTTON_PIN) == LOW)
            {
                game_Status = true;   // Start game
            }
        }

        Serial.println(cur);
        return; // Stay idle until button is pressed
    }

    /*************** GAME ACTIVE MODE ***************/
    clear_snake(); // Remove previous frame snake pixels

    // Read joystick movement
    Direction_t newDir = read_joystick_direction();
    if (!is_reverse_direction(snake.direction, newDir))
    {
        snake.direction = newDir;
    }

    move_snake();

    /*************** SELF COLLISION — GAME OVER ***************/
    if (check_self_collision())
    {
        delay(500);

        // Re-initialize for restart state
        init_snake();
        spawn_food();

        leds[get_led_number(food)] = CRGB::Red;
        FastLED.show();

        clear_Screen(); // Prevent flicker after reset
        draw_snake();
        FastLED.show();

        delay(MOVE_DELAY_MS);

        game_Status = false; // Return to WAIT MODE
        return;
    }

    /*************** FOOD EAT EVENT ***************/
    if (snake.body[0].x == food.x && snake.body[0].y == food.y)
    {
        if (snake.length < MAX_SNAKE_LEN)
            snake.length++;   // Increase snake length

        spawn_food();         // Re-place food
    }

    draw_snake();

    // Draw food pixel
    leds[get_led_number(food)] = CRGB::Red;

    FastLED.show();
    delay(MOVE_DELAY_MS);
}

/******************************** SNAKE FUNCTIONS ********************************/
/**
 * Initializes snake shape and starting direction.
 */
void init_snake(void)
{
    snake.length    = 3;
    snake.direction = DIR_RIGHT;

    snake.body[0] = (Position_t){1, 1};
    snake.body[1] = (Position_t){1, 2};
    snake.body[2] = (Position_t){1, 3};
}

/**
 * Moves snake by shifting all body segments forward
 * and advancing head in current direction.
 */
void move_snake(void)
{
    // Shift all body segments forward
    for (int i = snake.length - 1; i > 0; i--)
        snake.body[i] = snake.body[i - 1];

    Position_t *h = &snake.body[0];

    switch (snake.direction)
    {
        case DIR_RIGHT: h->x++; break;
        case DIR_LEFT:  h->x--; break;
        case DIR_UP:    h->y--; break;
        case DIR_DOWN:  h->y++; break;
    }

    // Edge wrap-around logic
    if (h->x < 1) h->x = MATRIX_WIDTH;
    if (h->x > MATRIX_WIDTH) h->x = 1;
    if (h->y < 1) h->y = MATRIX_HEIGHT;
    if (h->y > MATRIX_HEIGHT) h->y = 1;
}

/**
 * Checks whether the snake head hits its own body.
 */
bool check_self_collision(void)
{
    for (int i = 1; i < snake.length; i++)
    {
        if (snake.body[0].x == snake.body[i].x &&
            snake.body[0].y == snake.body[i].y)
        {
            return true;
        }
    }
    return false;
}

/**
 * Draws all snake body segments in green.
 */
void draw_snake(void)
{
    for (int i = 0; i < snake.length; i++)
        leds[get_led_number(snake.body[i])] = CRGB::Green;
}

/**
 * Clears snake pixels from last frame.
 */
void clear_snake(void)
{
    for (int i = 0; i < snake.length; i++)
        leds[get_led_number(snake.body[i])] = CRGB::Black;
}

/******************************** FOOD FUNCTIONS ********************************/
/**
 * Generates a food position that does NOT overlap snake.
 */
void spawn_food(void)
{
    do
    {
        food.x = random(1, MATRIX_WIDTH + 1);
        food.y = random(1, MATRIX_HEIGHT + 1);
    }
    while (is_food_on_snake(food));
}

/**
 * Returns true if a position lies on the snake body.
 */
bool is_food_on_snake(Position_t p)
{
    for (int i = 0; i < snake.length; i++)
    {
        if (snake.body[i].x == p.x && snake.body[i].y == p.y)
            return true;
    }
    return false;
}

/******************************** JOYSTICK PROCESSING ********************************/
/**
 * Converts joystick analog readings into direction control.
 * Dead-zone prevents noise and micro-movements.
 */
Direction_t read_joystick_direction(void)
{
    short x = readAnalogAxisLevel(ANALOG_X_PIN) - ANALOG_X_CORRECTION;
    short y = readAnalogAxisLevel(ANALOG_Y_PIN) - ANALOG_Y_CORRECTION;

    if (x < DEAD_MIN && y > DEAD_MIN && y < DEAD_MAX) return DIR_UP;
    if (x > DEAD_MAX && y > DEAD_MIN && y < DEAD_MAX) return DIR_DOWN;

    if (y < DEAD_MIN && x > DEAD_MIN && x < DEAD_MAX) return DIR_RIGHT;
    if (y > DEAD_MAX && x > DEAD_MIN && x < DEAD_MAX) return DIR_LEFT;

    return snake.direction;
}

/**
 * Prevents instant reverse direction to avoid self crash.
 */
bool is_reverse_direction(Direction_t c, Direction_t n)
{
    return ((c == DIR_UP && n == DIR_DOWN) ||
            (c == DIR_DOWN && n == DIR_UP) ||
            (c == DIR_LEFT && n == DIR_RIGHT) ||
            (c == DIR_RIGHT && n == DIR_LEFT));
}

/******************************** LED MAPPING ********************************/
/**
 * Converts (x,y) matrix coordinates to LED index
 * supports serpentine wiring layout.
 */
uint16_t get_led_number(Position_t pos)
{
    uint8_t x0 = pos.x - 1;
    uint8_t y0 = pos.y - 1;

    if (pos.y & 1)
        return (y0 * MATRIX_WIDTH) + x0;
    else
        return (y0 * MATRIX_WIDTH) + (MATRIX_WIDTH - 1 - x0);
}

/******************************** ANALOG UTILITIES ********************************/
/**
 * Reads joystick analog value and scales to 0-255.
 */
byte readAnalogAxisLevel(int pin)
{
    return map(analogRead(pin), 0, 1023, 0, 255);
}

/**
 * Clears entire LED matrix.
 * Helps avoid flicker during restart.
 */
void clear_Screen(void)
{
    for (int i = 1; i <= MATRIX_HEIGHT; i++)
    {
        for (int j = 1; j <= MATRIX_WIDTH; j++)
        {
            Position_t p;
            p.x = j;
            p.y = i;
            leds[get_led_number(p)] = CRGB::Black;
        }
    }
}
