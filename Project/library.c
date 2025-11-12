#include "GLCD.h"
#include "KBD.h"
#include "LPC17xx.h"
#include <stdio.h>
#include "image.c"
#include "image2.c"
#include "image3.c"
#define __FI        1                      /* Font index 16x24               */
#include "menu.h"

#define NUM_IMAGES 3

static unsigned char* images[NUM_IMAGES] = { image1, image2, image3};

void Gallery(void){
	int selected = 0;
	GLCD_Clear(Black);
  GLCD_SetBackColor(Black);
  GLCD_SetTextColor(White);
	GLCD_DisplayString(0, 0, __FI, "     Photo Gallery    ");
  //GLCD_Bitmap(0, 40, 240, 240, images[selected]);  // Draw first image
	while(1){
		uint32_t key = get_button();
		if (key & KBD_LEFT) {
            if (selected > 0) selected--;
            else selected = NUM_IMAGES - 1;
						GLCD_Clear(Black);
            //GLCD_Bitmap(0, 40, 240, 240, images[selected]); //240 might need to be calibrated to the actual size of the image //images might be too big need to resize?
        } else if (key & KBD_RIGHT) {
            if (selected < NUM_IMAGES - 1) selected++;
            else selected = 0;
						GLCD_Clear(Black);
            //GLCD_Bitmap(0, 40, 240, 240, images[selected]);
        } else if (key & KBD_SELECT) {
            GLCD_Clear(White);
            GLCD_DisplayString(1, 0, __FI, "     Loading    ");
						return;
        }
			//may need delays
	}
}