/*
 * Originally from
 *
 * https://www.instructables.com/id/Arduino-Pong-2/
 *
 * Cleaned and modified by Conrad Green <j05230@kcis.com.tw>
 *
 * How to wire up the composite video depends on exactly which
 * kind of Arduino it is.  See the table at the TVout repo
 *
 * https://github.com/Avamander/arduino-tvout
 *
 * For Leonardo, sync is on pin 9 and video on pin 8
 */

#include <TVout.h>
#include <video_gen.h>

/*
 * font related includes
 */

#include "/home/loldenium/Arduino/libraries/TVout/TVoutfonts/fontALL.h"
#include "/home/loldenium/Arduino/libraries/TVout/TVoutfonts/font8x8.cpp"
#include "/home/loldenium/Arduino/libraries/TVout/TVoutfonts/font4x6.cpp"

/* Arduino connections */

#define APIN_WHEEL_LEFT			0
#define APIN_WHEEL_RIGHT		1
#define DPIN_BUTTON			2
 
#define PADDLE_HEIGHT			14
#define PADDLE_WIDTH			2
 
#define RIGHT_PADDLE_X			(TV.hres()-4)
#define LEFT_PADDLE_X			2

typedef enum {
  STATE_IN_MENU,
	STATE_IN_GAME,
	STATE_GAME_OVER,
  STATE_WAIT_BUTTON_UP,
} game_state_t;

#define LEFT_SCORE_X			(TV.hres() / 2 - 15)
#define RIGHT_SCORE_X			(TV.hres() / 2 + 10)
#define SCORE_Y 4
 
#define MAX_Y_VELOCITY 6
#define WIN_SCORE 7
 
typedef enum {
	PLAYER_LEFT,
	PLAYER_RIGHT
} player_t;

TVout TV;
 
char button1Status = 0;
char button_filter = 0;
 
int wheelOnePosition;
int wheelTwoPosition;
int rightPaddleY;
int leftPaddleY;

#define BALLS 2

typedef struct ball {
  unsigned char x, y;
  char volx, voly;
} ball_t;

ball_t ball[BALLS];
 
int score[2];
 
int frame = 0;
game_state_t state = STATE_IN_MENU;

void init_ball(ball_t *b, int i)
{
  b->x = TV.hres() / 2;
  b->y = (rand() % TV.vres());
  b->volx = (rand() & 1) + 1;
  b->voly = (rand() & 1) + 1;

  if (i & 1)
    b->volx = -b->volx;
}

void
processInputs(void)
{
  char b = button1Status;
  
	wheelOnePosition = analogRead(APIN_WHEEL_LEFT);
	wheelTwoPosition = analogRead(APIN_WHEEL_RIGHT);
  
  /* debounce the button */

	b = !digitalRead(DPIN_BUTTON);
  if (b == !button1Status)
    button_filter++;
  else
    button_filter = 0;

  if (button_filter == 5)
    button1Status = b;
}
 
void
drawGameScreen(void)
{
	unsigned char x,y;
	int n;

	rightPaddleY = ((wheelOnePosition / 8) * (TV.vres() - PADDLE_HEIGHT)) / 128;
	x = RIGHT_PADDLE_X;
	for (n = 0; n < PADDLE_WIDTH; n++)
		TV.draw_line(x + n, rightPaddleY, x + n, rightPaddleY + PADDLE_HEIGHT, 1);

	leftPaddleY = ((wheelTwoPosition / 8) * (TV.vres()-PADDLE_HEIGHT))/ 128;
	x = LEFT_PADDLE_X;
	for (n = 0; n < PADDLE_WIDTH; n++)
		TV.draw_line(x + n, leftPaddleY, x + n, leftPaddleY + PADDLE_HEIGHT, 1);

	TV.print_char(LEFT_SCORE_X, SCORE_Y, '0' + score[PLAYER_LEFT]);
	TV.print_char(RIGHT_SCORE_X,SCORE_Y, '0' + score[PLAYER_RIGHT]);

 for (n = 0; n < BALLS; n++) {
	TV.set_pixel(ball[n].x, ball[n].y, 1);
    TV.set_pixel(ball[n].x - 1, ball[n].y, 1);
    TV.set_pixel(ball[n].x + 1, ball[n].y, 1);
        TV.set_pixel(ball[n].x, ball[n].y - 1, 1);
    TV.set_pixel(ball[n].x, ball[n].y + 1, 1);
 }
}
 
void
playerScored(player_t player)
{
	score[player]++;

	if (score[player] == WIN_SCORE)
		state = STATE_GAME_OVER;
}
 
void
drawBox(void)
{
	int n;

	TV.clear_screen();

	/* the net */ 

	for (n = 1; n < TV.vres() - 4; n += 6)
		TV.draw_line(TV.hres() / 2, n, TV.hres() / 2, n + 3, 1);

	/* box is smaller for TVs with overscan */

	TV.draw_line(  0,   0,   0,  95,   1 ); // left
	TV.draw_line(  0,   0, 126,   0,   1 ); // top
	TV.draw_line(126,   0, 126,  95,   1 ); // right
	TV.draw_line(  0,  95, 126,  95,   1 ); // bottom
}

void
drawMenu(void)
{
	TV.clear_screen();
	TV.select_font(font8x8);
	TV.print(10, 5, "Arduino Pong");
	TV.select_font(font4x6);
	TV.print(22, 35, "Press Button");
	TV.print(30, 45, "To Start");
}
 
void
setup(void)
{
  int n;

	TV.begin(_NTSC);       //for devices with only 1k sram(m168) use TV.begin(_NTSC,128,56)

	pinMode(DPIN_BUTTON, INPUT);

      drawMenu();
}
 
void loop(void)
{
  int n;
 
	processInputs();

	switch (state) {
	case STATE_IN_MENU:
   
  if (!button1Status)
    break;

      for (n = 0; n < BALLS; n++)
    init_ball(&ball[n], n);

  TV.select_font(font4x6);
  state = STATE_IN_GAME;
		break;

	case STATE_IN_GAME:

		if (frame % 3)
			break;

  drawBox();

  for (n = 0; n < BALLS; n++) {

		ball[n].x += ball[n].volx;
    ball[n].y += ball[n].voly;

		// change if hit top or bottom

		if (ball[n].y <= 1 || ball[n].y >= TV.vres() - 1) {
			ball[n].voly = -ball[n].voly;
			TV.tone(2000, 30);
		}

		// test left side for wall hit
    
		if (ball[n].volx < 0 && 
		    ball[n].x == LEFT_PADDLE_X + PADDLE_WIDTH - 1 &&
		    ball[n].y >= leftPaddleY &&
		    ball[n].y <= leftPaddleY + PADDLE_HEIGHT) {
			ball[n].volx = -ball[n].volx;
		   ball[n].voly += 2 * ((ball[n].y - leftPaddleY) - (PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2);

			TV.tone(2000, 30);  
		}

		// test right side for wall hit
  
		if (ball[n].volx > 0 &&
		    ball[n].x == RIGHT_PADDLE_X &&
		    ball[n].y >= rightPaddleY &&
		    ball[n].y <= rightPaddleY + PADDLE_HEIGHT) {
			ball[n].volx = -ball[n].volx;
			ball[n].voly += 2 * ((ball[n].y - rightPaddleY) - (PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2);
			TV.tone(2000, 30);
		}

		// limit vertical speed

		if (ball[n].voly > MAX_Y_VELOCITY)
			ball[n].voly = MAX_Y_VELOCITY;
		if (ball[n].voly < -MAX_Y_VELOCITY)
			ball[n].voly = -MAX_Y_VELOCITY;

		// Scoring
		if (ball[n].x <= 1) {
			playerScored(PLAYER_LEFT);
        ball[n].volx = -ball[n].volx;
			TV.tone(500, 300);  
		}
		if (ball[n].x >= TV.hres() - 1) {
			playerScored(PLAYER_RIGHT);
             ball[n].volx = -ball[n].volx;
			TV.tone(500, 300);
		}
  }

		drawGameScreen();
		break;

	case STATE_GAME_OVER:
 
		// drawGameScreen();
		TV.select_font(font8x8);
		TV.print(29,25,"GAME");
		TV.print(68,25,"OVER");
		if (!button1Status)
   break;

		TV.select_font(font4x6); //reset the font

		/* reset the scores */

		score[PLAYER_LEFT] = 0;
		score[PLAYER_RIGHT] = 0;

		state = STATE_WAIT_BUTTON_UP;
		break;
  case STATE_WAIT_BUTTON_UP:
      if (button1Status)
          break;
              drawMenu();
          state = STATE_IN_MENU;
          break;    
	}

	TV.delay_frame(1);
	if (++frame == 60)
		frame = 0;
}
