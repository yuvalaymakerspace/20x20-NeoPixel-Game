/********************************************************************************************
 * Project Name : LED Matrix Snake Game (20x20)
 * Platform     : Arduino + WS2811 / WS2812 LED Matrix
 * Author       : Prince Kumar
 * Organization : Yuvalay Makerspace
 * Version      : V 0.1.0
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
#define DATA_PIN        3
#define MATRIX_WIDTH    20
#define MATRIX_HEIGHT   20
#define MOVE_DELAY_MS   150   // Delay between each snake movement step

/******************************** JOYSTICK INPUTS ********************************/
#define ANALOG_X_PIN A2
#define ANALOG_Y_PIN A3

// Joystick center calibration offsets
#define ANALOG_X_CORRECTION 126
#define ANALOG_Y_CORRECTION 123

// Dead-zone limits (prevents unwanted jitter)
#define DEAD_MIN   -53
#define DEAD_MAX    54

/******************************** DATA STRUCTURES ********************************/
/// Position of a pixel in the matrix
typedef struct
{
    uint8_t x;
    uint8_t y;
} Position_t;

/// Direction of snake movement
typedef enum
{
    DIR_LEFT = 0,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
} Direction_t;

#define MAX_SNAKE_LEN 100  // Upper safety limit for snake length

/// Snake object (head, body, direction, length)
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

/******************************** FUNCTION PROTOTYPES ********************************/
uint16_t get_led_number(Position_t pos);

void init_snake(void);
void move_snake(void);
void draw_snake(void);
void clear_snake(void);

void spawn_food(void);
bool is_food_on_snake(Position_t p);
bool check_self_collision(void);

Direction_t read_joystick_direction(void);
bool is_reverse_direction(Direction_t cur, Direction_t next);

byte readAnalogAxisLevel(int pin);

/******************************** SETUP ********************************/
/**
 * This function runs once at startup.
 * It initializes LEDs, seeds the random generator,
 * creates the snake, and spawns initial food.
 */
void setup()
{
    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();

    // Seed randomness using analog noise
    randomSeed(analogRead(A0));

    init_snake();
    spawn_food();
}

/******************************** MAIN LOOP ********************************/
/**
 * This function runs repeatedly.
 * It updates snake direction, movement, collision detection,
 * food handling, and matrix drawing.
 */
void loop()
{
    clear_snake();  // Remove previous snake pixels

    // Read joystick and update direction safely
    Direction_t newDir = read_joystick_direction();
    if (!is_reverse_direction(snake.direction, newDir))
    {
        snake.direction = newDir;
    }

    move_snake();   // Move snake one step ahead

    // GAME OVER: Restart if snake hits itself
    if (check_self_collision())
    {
        delay(500);
        init_snake();
        spawn_food();
        return;
    }

    // FOOD EAT EVENT
    if (snake.body[0].x == food.x && snake.body[0].y == food.y)
    {
        if (snake.length < MAX_SNAKE_LEN)
            snake.length++;   // Increase snake size

        spawn_food();         // Generate a new food location
    }

    draw_snake(); // Draw updated snake body

    // Draw food pixel in red color
    leds[get_led_number(food)] = CRGB::Red;

    FastLED.show();
    delay(MOVE_DELAY_MS);
}

/******************************** SNAKE FUNCTIONS ********************************/
/**
 * Initializes snake properties such as length, position, and direction.
 * This function is also used during game restart.
 */
void init_snake(void)
{
    snake.length    = 3;
    snake.direction = DIR_RIGHT;

    // Start snake in a compact group
    snake.body[0] = (Position_t){1, 3};
    snake.body[1] = (Position_t){1, 3};
    snake.body[2] = (Position_t){1, 3};
}

/**
 * Moves the snake forward by shifting body segments.
 * The head advances in the direction of travel.
 * Wrap-around logic keeps snake inside matrix bounds.
 */
void move_snake(void)
{
    // Move body segments forward
    for (int i = snake.length - 1; i > 0; i--)
        snake.body[i] = snake.body[i - 1];

    Position_t *h = &snake.body[0];

    // Update head based on direction
    switch (snake.direction)
    {
        case DIR_RIGHT: h->x++; break;
        case DIR_LEFT:  h->x--; break;
        case DIR_UP:    h->y--; break;
        case DIR_DOWN:  h->y++; break;
    }

    // Wrap around matrix edges
    if (h->x < 1) h->x = MATRIX_WIDTH;
    if (h->x > MATRIX_WIDTH) h->x = 1;
    if (h->y < 1) h->y = MATRIX_HEIGHT;
    if (h->y > MATRIX_HEIGHT) h->y = 1;
}

/**
 * Checks whether the snake head has collided with its body.
 * Returns true if collision occurs.
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
 * Draws each snake body segment in green color.
 */
void draw_snake(void)
{
    for (int i = 0; i < snake.length; i++)
        leds[get_led_number(snake.body[i])] = CRGB::Green;
}

/**
 * Clears snake pixels from previous frame.
 */
void clear_snake(void)
{
    for (int i = 0; i < snake.length; i++)
        leds[get_led_number(snake.body[i])] = CRGB::Black;
}

/******************************** FOOD FUNCTIONS ********************************/
/**
 * Randomly generates a food position.
 * Ensures food does not spawn on snake body.
 */
void spawn_food(void)
{
    do
    {
        food.x = random(1, MATRIX_WIDTH + 1);
        food.y = random(1, MATRIX_HEIGHT + 1);
    } while (is_food_on_snake(food));
}

/**
 * Checks whether a given position lies on the snake body.
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

/******************************** JOYSTICK INPUT ********************************/
/**
 * Reads joystick movement and converts it into direction.
 * Includes dead-zone filtering to avoid flicker or noise.
 */
Direction_t read_joystick_direction(void)
{
    short x = readAnalogAxisLevel(ANALOG_X_PIN) - ANALOG_X_CORRECTION;
    short y = readAnalogAxisLevel(ANALOG_Y_PIN) - ANALOG_Y_CORRECTION;

    // Vertical movement
    if (x < DEAD_MIN && y > DEAD_MIN && y < DEAD_MAX) return DIR_UP;
    if (x > DEAD_MAX && y > DEAD_MIN && y < DEAD_MAX) return DIR_DOWN;

    // Horizontal movement
    if (y < DEAD_MIN && x > DEAD_MIN && x < DEAD_MAX) return DIR_RIGHT;
    if (y > DEAD_MAX && x > DEAD_MIN && x < DEAD_MAX) return DIR_LEFT;

    // No significant movement — retain last direction
    return snake.direction;
}

/**
 * Prevents immediate opposite direction reversal.
 * Example: Right → Left (illegal)
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
 * Converts matrix X,Y coordinates to LED index number.
 * Handles serpentine wiring layout.
 */
uint16_t get_led_number(Position_t pos)
{
    uint8_t x0 = pos.x - 1;
    uint8_t y0 = pos.y - 1;

    // Odd rows go left-to-right, even rows right-to-left
    if (pos.y & 1)
        return (y0 * MATRIX_WIDTH) + x0;
    else
        return (y0 * MATRIX_WIDTH) + (MATRIX_WIDTH - 1 - x0);
}

/******************************** ANALOG INPUT ********************************/
/**
 * Reads analog joystick value and scales it to 0-255 range.
 */
byte readAnalogAxisLevel(int pin)
{
    return map(analogRead(pin), 0, 1023, 0, 255);
}
