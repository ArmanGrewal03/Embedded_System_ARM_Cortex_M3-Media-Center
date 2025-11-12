#include "GLCD.h"
#define __FI        1                      /* Font index 16x24               */
#include "LPC17xx.h"
#include <stdio.h>
#include "menu.h"
#include "KBD.h"
#include "library.h"
#include "media.h"
#include "gamedino.h"

void selectMenu(int selected, int mode){
	for (int i = 0; i < 3; i++){
        if (i == selected)
            GLCD_SetTextColor(Yellow);
        else
            GLCD_SetTextColor(White);
				if(mode == 1){
					switch (i) {
            case 0: GLCD_DisplayString(1, 0, __FI, "     Dino Game    "); break;
            case 1: GLCD_DisplayString(2, 0, __FI, "     Snake Game       "); break;
            case 2: GLCD_DisplayString(3, 0, __FI, "     Back            "); break;
					}
				}
				else{
					switch (i) {
							case 0: GLCD_DisplayString(1, 0, __FI, "     Photo Gallery    "); break;
							case 1: GLCD_DisplayString(2, 0, __FI, "     MP3 Player       "); break;
							case 2: GLCD_DisplayString(3, 0, __FI, "     Games            "); break;
					}
			  }
		}
}


void gameSelect(){
	initMenu();
	int selected = 0;
	selectMenu(selected, 1);
	while(1){
		uint32_t key = get_button();
		if (key & KBD_UP) {
        if (selected > 0) selected--;
        selectMenu(selected, 1);
    } 
		else if (key & KBD_DOWN) {
        if (selected < 2) selected++;
            selectMenu(selected, 1);
        } 
		else if (key & KBD_SELECT) {
        GLCD_Clear(White);
        GLCD_DisplayString(1, 0, __FI, "     Loading    ");
        switch (selected) {
             case 0: game_run(); initMenu(); break;
             case 1: initMenu(); break;
             case 2: return;
         }
     }
			//may need delays
	}
}

void runMenu(){
	int selected = 0;
	selectMenu(selected, 0);
	while(1){
		uint32_t key = get_button();
		if (key & KBD_UP) {
        if (selected > 0) selected--;
        selectMenu(selected, 0);
    } 
		else if (key & KBD_DOWN) {
        if (selected < 2) selected++;
            selectMenu(selected, 0);
        } 
		else if (key & KBD_SELECT) {
        GLCD_Clear(White);
        GLCD_DisplayString(1, 0, __FI, "     Loading    ");
        switch (selected) {
             case 0: Gallery(); initMenu(); break;
             case 1: runMusicPlayer(); initMenu(); break;
             case 2: gameSelect(); initMenu(); break;
         }
     }
			//may need delays
	}
}

void initMenu(){
	GLCD_Clear(White);                         /* Clear graphical LCD display   */
  GLCD_SetBackColor(Blue);
	GLCD_DisplayString(0, 0, __FI, "     Menu    ");
}

