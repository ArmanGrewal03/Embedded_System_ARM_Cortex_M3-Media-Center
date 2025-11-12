#include "GLCD.h"
#define __FI 1  /* Font index 16x24 */

#include "LPC17xx.h"
#include "system_LPC17xx.h"
#include <stdio.h>
#include <stdint.h>


#include "Board_LED.h"
#include "KBD.h"


#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "usbaudio.h"
#include "type.h"

#include "cat.h"   
#include "tree.h"  

#define IMG0_W    CAT_WIDTH
#define IMG0_H    CAT_HEIGHT
#define IMG0_DATA CAT_PIXEL_DATA

#define IMG1_W    TREE_WIDTH
#define IMG1_H    TREE_HEIGHT
#define IMG1_DATA TREE_PIXEL_DATA

static void ui_header(const char* title){
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_ClearLn(0, 1);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)title);
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
}

/******************************************************************************************** MENU *********************************************************************************************************/

static void menu_footer(int selected, int total){
  char f[32];
  snprintf(f, sizeof(f), "UP/DOWN  SELECT   %d/%d", selected + 1, total);
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
  GLCD_ClearLn(8, 1);
  GLCD_DisplayString(8, 0, 1, (unsigned char*)f);
}

static void initMenu(void){
  GLCD_Clear(White);
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(White);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)"     Menu    ");
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
}

/* Draws both rows with a '>' cursor and highlight on the selected row */
static void selectMenu(int selected){
  const int total = 2;
  for (int i = 0; i < total; i++){
    /* Clear the line first */
    GLCD_SetBackColor(White);
    GLCD_SetTextColor(Black);
    GLCD_ClearLn(2 + i, 1);

    /* Build line text with cursor */
    char line[32];
    const char *label =
      (i == 0) ? "Photo Gallery"
               : "MP3 Player";
    snprintf(line, sizeof(line), "%c %s", (i == selected) ? '>' : ' ', label);

    /* Highlight selected row */
    if (i == selected) {
      GLCD_SetBackColor(Green);   /* highlighted background */
      GLCD_SetTextColor(Black);   /* readable text on green */
    } else {
      GLCD_SetBackColor(White);
      GLCD_SetTextColor(Black);
    }

    GLCD_DisplayString(2 + i, 0, __FI, (unsigned char*)line);
  }

  /* Footer with position hint */
  menu_footer(selected, total);

  /* Restore defaults */
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);

}

/* Forward decls for entries */
static void Gallery(void);
static void runMusicPlayer(void);

static void runMenu(void){
  int selected = 0;
  selectMenu(selected);
  while(1){
    uint32_t key = get_button();
    if (key & KBD_UP) {
      if (selected > 0) selected--;
      selectMenu(selected);
    } else if (key & KBD_DOWN) {
      if (selected < 1) selected++;
      selectMenu(selected);
    } else if (key & KBD_SELECT) {
      GLCD_Clear(White);
      GLCD_SetTextColor(Black);
      GLCD_DisplayString(1, 0, __FI, (unsigned char*)"     Loading    ");
      switch (selected) {
        case 0: Gallery();        initMenu(); selectMenu(selected); break;
        case 1: runMusicPlayer(); initMenu(); selectMenu(selected); break;
      }
    }
    /* simple debounce-ish delay */
    for (volatile uint32_t d=0; d<40000; d++) __NOP();
  }
}
/******************************************************************************************** PHOTO GALLERY *********************************************************************************************************/

static int gallery_index = 0;
static int gallery_count = 0;

static void gallery_draw_current(void){
  GLCD_Clear(White);
  ui_header("Photo Gallery   <SELECT to Back>");

int gallery_counter = 0;

#if defined(IMG0_DATA)
if (gallery_index == gallery_counter++) {
    GLCD_Bitmap(0, 0, IMG0_W, IMG0_H, (const unsigned char*)IMG0_DATA);
}
#endif

#if defined(IMG1_DATA)
if (gallery_index == gallery_counter++) {
    GLCD_Bitmap(0, 0, IMG1_W, IMG1_H, (const unsigned char*)IMG1_DATA);
}
#endif

#if defined(IMG2_DATA)
if (gallery_index == gallery_counter++) {
    GLCD_Bitmap(0, 0, IMG2_W, IMG2_H, (const unsigned char*)IMG2_DATA);
}
#endif

#if defined(IMG3_DATA)
if (gallery_index == gallery_counter++) {
    GLCD_Bitmap(0, 0, IMG3_W, IMG3_H, (const unsigned char*)IMG3_DATA);
}
#endif


  GLCD_SetTextColor(Black);
  GLCD_DisplayString(9, 0, 1, (unsigned char*)"LEFT/RIGHT: Prev/Next   SELECT: Back");
}

static void Gallery(void){
#if defined(IMG1_DATA)
  gallery_count = 2;
#elif defined(IMG0_DATA)
  gallery_count = 1;
#else
  gallery_count = 1; /* stub page */
#endif

  int last = -1;
  gallery_draw_current();

  while(1){
    uint32_t key = get_button();
    if (key & KBD_SELECT) return;
    if (key & KBD_LEFT)  gallery_index--;
    if (key & KBD_RIGHT) gallery_index++;

    if (gallery_index < 0)               gallery_index = gallery_count-1;
    if (gallery_index >= gallery_count)  gallery_index = 0;

    if (last != gallery_index){
      last = gallery_index;
      gallery_draw_current();
    }
    for (volatile uint32_t d=0; d<40000; d++) __NOP();
  }
}
/******************************************************************************************** USB AUDIO PLAYER*********************************************************************************************************/

/* These globals mirror Keil USB audio example usage */
extern void SystemClockUpdate(void);
extern uint32_t SystemFrequency;

uint8_t  Mute;          /* Mute State (from USB control) */
uint32_t Volume;        /* Chained volume (USB curve * pot) */
static  uint8_t quit = 0;

#if USB_DMA
uint32_t *InfoBuf = (uint32_t *)(DMA_BUF_ADR);
short    *DataBuf = (short *)(DMA_BUF_ADR + 4*P_C);
#else
uint32_t InfoBuf[P_C];
short    DataBuf[B_S];
#endif

uint16_t DataOut;
uint16_t DataIn;
uint8_t  DataRun;
uint16_t PotVal;          /* Potentiometer raw */
uint32_t VUM;
uint32_t Tick;

/* VolCur is provided by usbaudio.c (current USB volume) */
extern uint16_t VolCur;

/* Read potentiometer into PotVal (matches Keil example flow) */
static void get_potval(void){
  uint32_t val;
  LPC_ADC->ADCR |= 0x01000000;              /* Start A/D Conversion */
  do {
    val = LPC_ADC->ADGDR;                   /* Read A/D Data Register */
  } while ((val & 0x80000000) == 0);
  LPC_ADC->ADCR &= ~0x01000000;             /* Stop A/D Conversion */
  PotVal = ((val >> 8) & 0xF8) + ((val >> 7) & 0x08);
}

/* Lightweight on-screen HUD for volume; called from ISR */
static uint8_t s_last_vol = 255;
static void media_update_volume_ui(uint8_t vol_percent){
  if (vol_percent == s_last_vol) return;
  s_last_vol = vol_percent;

  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
  GLCD_DisplayString(2, 0, 1, (unsigned char*)"USB Audio: CONNECTED        ");
  GLCD_DisplayString(3, 0, 1, (unsigned char*)"Volume (pot):               ");

  char buf[16];
  sprintf(buf, "%3u%%", vol_percent);
  GLCD_DisplayString(3, 16, 1, (unsigned char*)buf);

  GLCD_Bargraph(20, 5*16, 280, 16, vol_percent);
}

/* Timer ISR: feeds DAC with samples and updates volume/VU */
void TIMER0_IRQHandler(void){
  long      val;
  uint32_t  cnt;

  if (DataRun) {
    val = DataBuf[DataOut];
    cnt = (DataIn - DataOut) & (B_S - 1);
    if (cnt == (B_S - P_C*P_S)) { DataOut++; }        /* too much -> skip */
    if (cnt > (P_C*P_S))        { DataOut++; }        /* enough -> advance */
    DataOut &= B_S - 1;

    if (val < 0) VUM -= val; else VUM += val;

    val  *= Volume;             /* Apply volume */
    val >>= 16;
    val  += 0x8000;
    val  &= 0xFFFF;
  } else {
    val = 0x8000;               /* DAC mid */
  }

  if (Mute) {
    val = 0x8000;
  }

  LPC_DAC->DACR = val & 0xFFC0;

  if ((Tick++ & 0x03FF) == 0) {        /* every 1024 ticks */
    get_potval();

    if (VolCur == 0x8000) {            /* minimum level -> no sound */
      Volume = 0;
    } else {
      Volume = VolCur * PotVal;        /* chained volume */
    }

    long vu = (long)(VUM >> 20);
    VUM = 0;
    if (vu > 7) vu = 7;

    /* ---- UI Volume % (map PotVal ~0..1023 to 0..100) ---- */
    uint32_t pot_raw = PotVal & 0x3FF;                   /* 0..1023 */
    uint8_t  vol_pct = (uint8_t)((pot_raw * 100U) / 1023U);
    media_update_volume_ui(vol_pct);
  }

  LPC_TIM0->IR = 1;  /* Clear interrupt */

  /* Quick exit path on SELECT */
  uint32_t joy = get_button();
  if (joy & KBD_SELECT){
    NVIC_DisableIRQ(TIMER0_IRQn);
    NVIC_DisableIRQ(USB_IRQn);
    quit = 1;
  }
}

/* Entry invoked from menu */
static void runMusicPlayer(void){
  /* Banner */
  GLCD_Clear(White);
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_DisplayString(0, 0, 1, (unsigned char*)"  MP3 Player (USB) ");
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
  GLCD_DisplayString(1, 0, 1, (unsigned char*)"Press SELECT to stop and return");

  /* Clocks & pins (as per Keil example) */
  volatile uint32_t pclkdiv, pclk;
  SystemClockUpdate();

  /* P0.25 AIN2, P0.26 AOUT */
  LPC_PINCON->PINSEL1 &= ~((0x03<<18)|(0x03<<20));
  LPC_PINCON->PINSEL1 |=  ((0x01<<18)|(0x02<<20));

  LPC_SC->PCONP |= (1 << 12);        /* ADC power */

  LPC_ADC->ADCR  = 0x00200E04;       /* ADC: AIN2 @ ~4MHz (per Keil ex.) */
  LPC_DAC->DACR  = 0x00008000;       /* DAC mid */

  /* PCLK for TIMER0 */
  pclkdiv = (LPC_SC->PCLKSEL0 >> 2) & 0x03;
  switch (pclkdiv){
    case 0x01: pclk = SystemFrequency;    break;
    case 0x02: pclk = SystemFrequency/2;  break;
    case 0x03: pclk = SystemFrequency/8;  break;
    case 0x00:
    default:   pclk = SystemFrequency/4;  break;
  }

  LPC_TIM0->MR0 = pclk/DATA_FREQ - 1;  /* audio sample timer */
  LPC_TIM0->MCR = 3;                   /* IRQ + reset on MR0 */
  LPC_TIM0->TCR = 1;                   /* enable */
  NVIC_EnableIRQ(TIMER0_IRQn);

  USB_Init();
  USB_Connect(TRUE);

  quit = 0;
  while(1){
    if (quit){ quit = 0; return; }
    /* idle loop; audio handled in ISR/USB */
  }
}

/******************************************************************************************** MAIN STARTING POINT *********************************************************************************************************/
int main (void) {
  LED_Initialize();
  GLCD_Init();
  KBD_Init();

  GLCD_Clear(White);
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)"     COE718 Final Project   ");
  GLCD_SetTextColor(White);
  GLCD_DisplayString(1, 0, __FI, (unsigned char*)"     Arman Grewal   ");
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Red);

  initMenu();
  selectMenu(0);
  runMenu();
}