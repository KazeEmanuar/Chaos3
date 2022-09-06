/*
Setup:
* Copy pong.c and pong.h to src/game
* Go to hud.c, and add `#include "pong.h"` to the list of includes
* Right at the end of `render_hud`, add `run_pong()`
* Follow the notes I left for you here (marked with 'KAZE:')
*/

#include <sm64.h>
#include "engine/math_util.h"
#include "engine/behavior_script.h"
#include "game_init.h"
#include "level_update.h"
#include "mario.h"
#include "print.h"
//#include "codeActive.h" //KAZE: wherever your codeActive function is declared

//Pong constants
#define PONG_WIDTH ((SCREEN_WIDTH / 2) - 24)
#define PONG_HEIGHT ((SCREEN_HEIGHT / 2) - 8)

#define PADDLE_SPEED (2.5f)
#define PADDLE_XRAD (12)
#define PADDLE_YRAD (24)

#define BALL_SPEED (6.0f)
#define BALL_ANGLE (0x3000)

//Pong types
typedef struct
{
	float x, y;
} Paddle;

typedef struct
{
	float x, y;
	float xsp, ysp;
} Ball;

typedef struct
{
	//Pong state
	char running, ended;
	char wait;
	
	//Ball and paddles
	Ball ball;
	Paddle paddle[2];
} Pong;

//Paddle object
s8 ai_paddle(Paddle *paddle, Ball *ball)
{
	if (paddle->y - (PADDLE_YRAD - 6) > ball->y)
		return -1;
	if (paddle->y + (PADDLE_YRAD - 6) < ball->y)
		return 1;
	return 0;
}

void update_paddle(Paddle *paddle, s8 move)
{
	//Move paddle
	paddle->y += move * PADDLE_SPEED;
	
	//Keep paddle in bounds
	if (paddle->y < -(PONG_HEIGHT - PADDLE_YRAD))
		paddle->y = -(PONG_HEIGHT - PADDLE_YRAD);
	if (paddle->y >  (PONG_HEIGHT - PADDLE_YRAD))
		paddle->y =  (PONG_HEIGHT - PADDLE_YRAD);
}

void draw_paddle(Paddle *paddle)
{
	//Draw paddle (two 'I's drawn on top of eachother)
	print_text((SCREEN_WIDTH / 2) + (int)paddle->x - 8, (SCREEN_HEIGHT / 2) - (int)paddle->y - 16, "I");
	print_text((SCREEN_WIDTH / 2) + (int)paddle->x - 8, (SCREEN_HEIGHT / 2) - (int)paddle->y, "I");
}

//Ball object
void set_ball_angle(Ball *ball, u16 angle)
{
	ball->xsp = coss(angle) * BALL_SPEED;
	ball->ysp = sins(angle) * BALL_SPEED;
}

void update_ball(Ball *ball)
{
	//Move ball
	ball->x += ball->xsp;
	ball->y += ball->ysp;
	
	//Bounce off top and bottom
	if (ball->y <= -PONG_HEIGHT || ball->y >= PONG_HEIGHT)
	{
		ball->y -= ball->ysp;
		ball->ysp *= -1.0f;
	}
}

void draw_ball(Ball *ball)
{
	//Draw ball as Mario's head
	print_text((SCREEN_WIDTH / 2) + (int)ball->x - 8, (SCREEN_HEIGHT / 2) - (int)ball->y - 8, ",");
}

void collide_ball(Ball *ball, Paddle *paddle)
{
	//Check if we're touching the paddle
	if (ball->x >= (paddle->x - PADDLE_XRAD) && ball->x <= (paddle->x + PADDLE_XRAD) &&
		ball->y >= (paddle->y - PADDLE_YRAD) && ball->y <= (paddle->y + PADDLE_YRAD))
	{
		//Bounce off the paddle
		if (ball->x > paddle->x)
		{
			set_ball_angle(ball, (0x0000 + BALL_ANGLE) - (random_u16() % (BALL_ANGLE * 2)));
			ball->x = paddle->x + PADDLE_XRAD + ball->xsp;
		}
		else
		{
			set_ball_angle(ball, (0x8000 + BALL_ANGLE) - (random_u16() % (BALL_ANGLE * 2)));
			ball->x = paddle->x - PADDLE_XRAD + ball->xsp;
		}
	}
}

//Pong game
Pong pong;

void init_pong()
{
	//Initialize pong state
	pong.running = FALSE;
	pong.ended = FALSE;
	pong.wait = 20;
	
	//Initialize ball
	pong.ball.x = 0.0f;
	pong.ball.y = 0.0f;
	if (random_u16() & 1)
        set_ball_angle(&pong.ball, (0x0000 + 0x1000) - (random_u16() % (0x2000)));
    else
        set_ball_angle(&pong.ball, (0x8000 + 0x1000) - (random_u16() % (0x2000)));
	
	//Initialize paddles
	pong.paddle[0].x = -(PONG_WIDTH - 16);
	pong.paddle[0].y = 0.0f;
	
	pong.paddle[1].x = PONG_WIDTH - 16;
	pong.paddle[1].y = 0.0f;
}

void run_pong()
{
	if (codeActive(83))
	{
		if (!pong.running)
		{
			init_pong();
			pong.running = TRUE;
			pong.ended = FALSE;
		}
	}
	else
	{
		if (pong.ended)
		{
			pong.running = FALSE;
			pong.ended = FALSE;
		}
	}
	
	if (pong.running && !pong.ended)
	{
		//Determine player's input
		s8 ai_move;
		s8 player_move = 0;
		if (gControllers[0].stickY > 8)
			player_move = -gControllers[0].stickY/16;
		else if (gControllers[0].stickY < -8)
			player_move = -gControllers[0].stickY/16;
		
		//Determine AI's input
		ai_move = ai_paddle(&pong.paddle[1], &pong.ball);
		
		//Update game
		if (pong.wait == 0 || pong.wait-- == 0)
		{
			update_paddle(&pong.paddle[0], player_move);
			update_paddle(&pong.paddle[1], ai_move);
			
			update_ball(&pong.ball);
			
			collide_ball(&pong.ball, &pong.paddle[0]);
			collide_ball(&pong.ball, &pong.paddle[1]);
		}
		
		//Draw game
		draw_paddle(&pong.paddle[0]);
		draw_paddle(&pong.paddle[1]);
		draw_ball(&pong.ball);
		
		//If the ball is off the left or right, end
		if (pong.ball.x < -PONG_WIDTH)
		{
			//It's off the player's side, kill Mario and stop the code
			gMarioState->health = 0;
			pong.ended = TRUE;
		}
		else if (pong.ball.x > PONG_WIDTH)
		{
			//It's off the AI's side, just stop the code
			pong.ended = TRUE;
		}
	}
}
