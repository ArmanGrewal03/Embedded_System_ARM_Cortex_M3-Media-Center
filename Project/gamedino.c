#include "LPC17xx.h"
#include "GLCD.h"
#include "KBD.h"
#include "gamedino.h"
#include <stdio.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define GROUND_Y 200
#define PLAYER_W 16
#define PLAYER_H 20
#define PLAYER_X 40

#define GRAVITY 2
#define JUMP_VEL -18

#define OBST_W 16
#define OBST_H 20
#define OBST_SPEED 4

typedef struct{
	int y;
	int vy;
	int onGround;
}Player;

typedef struct{
	int x;
	int y;
	int active;
}Obstacle;

static Player player;
static Obstacle obst;

static int score;
static int gameOver;

static void game_delay(void){
volatile int i, j;
	for (i=0; i < 8000; i++){
		for (j=0; j<50; j++){
		}
	}
}

static void drawHLine(int x0, int x1, int y){
	int x;
	if (x0 < 0) x0 = 0;
	if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
	if (y < 0 || y >= SCREEN_H) return;
	
	for (x = x0; x <= x1; x++){
		GLCD_PutPixel(x, (unsigned int)y);
	}
}

static void drawFilledRect(int x, int y, int w, int h){
	int i, j;
	
	if (x<0) x = 0;
	if (y<0) y = 0;
	if (x + w > SCREEN_W) w = SCREEN_W - x;
	if (y + h > SCREEN_H) h = SCREEN_H - y;
	
	for (i=0; i<h; i++){
		for (j=0; j < w; j++){
			GLCD_PutPixel((unsigned int)(x+j), (unsigned int)(y+i));
		}
	}
}

static void drawGround(void){
	GLCD_SetTextColor(White);
	drawHLine(0, SCREEN_W - 1, GROUND_Y);
}

static void drawPlayer(void){
	GLCD_SetTextColor(Green);
	drawFilledRect(PLAYER_X, player.y, PLAYER_W, PLAYER_H);
}

static void drawObstacle(void){
	if (!obst.active) return;
	
	GLCD_SetTextColor(Red);
	drawFilledRect(obst.x, obst.y, OBST_W, OBST_H);
}

static void drawHUD(void){
	char buf[20];
	
	GLCD_SetTextColor(White);
	GLCD_SetBackColor(Black);
	
	sprintf(buf, "Score: %d", score);
	GLCD_DisplayString(0, 0, 1, (unsigned char *)buf);
}

static void handleInput(void){
	uint32_t keys = KBD_get();
	
	if ((keys & KBD_UP) && player.onGround){
		player.vy = JUMP_VEL;
		player.onGround = 0;
	}
}

static void updateGame(void){
	if (!player.onGround){
		player.vy += GRAVITY;
		player.y += player.vy;
		
		if (player.y + PLAYER_H >= GROUND_Y){
			player.y = GROUND_Y - PLAYER_H;
			player.vy = 0;
			player.onGround = 1;
		}
	}
	
	if (obst.active){
		obst.x -= OBST_SPEED;
		
		if(obst.x + OBST_W < 0){
			obst.active = 0;
			score++;
		}
	}
	
 if (!obst.active) {
        obst.x = SCREEN_W + 40;
        obst.y = GROUND_Y - OBST_H;
        obst.active = 1;
    }
}

static int checkCollision(void) {
    int px1 = PLAYER_X;
    int py1 = player.y;
    int px2 = PLAYER_X + PLAYER_W;
    int py2 = player.y + PLAYER_H;

    int ox1 = obst.x;
    int oy1 = obst.y;
    int ox2 = obst.x + OBST_W;
    int oy2 = obst.y + OBST_H;

    int overlapX = (px1 < ox2) && (px2 > ox1);
    int overlapY = (py1 < oy2) && (py2 > oy1);

    return overlapX && overlapY;
}

static void game_init(void) {
    player.y = GROUND_Y - PLAYER_H;
    player.vy = 0;
    player.onGround = 1;

    obst.x = SCREEN_W + 40;
    obst.y = GROUND_Y - OBST_H;
    obst.active = 1;

    score = 0;
    gameOver = 0;

    GLCD_Clear(Black);
}

void game_run(void) {
    game_init();

    while (1) {
        handleInput();
        updateGame();

        if (checkCollision()) {
            gameOver = 1;
        }

        GLCD_Clear(Black);
        drawGround();
        drawPlayer();
        drawObstacle();
        drawHUD();

        if (gameOver) {
            GLCD_SetTextColor(White);
            GLCD_SetBackColor(Black);
            GLCD_DisplayString(5, 5, 1, (unsigned char *)"GAME OVER");
            GLCD_DisplayString(6, 5, 1, (unsigned char *)"Press any key");


            while (1) {
                if (KBD_get() & KBD_SELECT) {
                    return;
                }
            }
        }

        game_delay();
    }
}