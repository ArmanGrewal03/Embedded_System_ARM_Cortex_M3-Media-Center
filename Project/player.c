#include <stdio.h>
#include <stdint.h>

#include "LPC17xx.h"
#include "system_LPC17xx.h"
#include "GLCD.h"
#include "KBD.h"

#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "usbaudio.h"
#include "type.h"

#include "player.h"

#define __FI 1

extern void SystemClockUpdate(void);
extern uint32_t SystemFrequency;

/* --- Audio globals expected by Keil USB audio stack (like usbmain.c) --- */
uint8_t  Mute;          /* Mute state */
uint32_t Volume;        /* Volume level used in ISR */

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
uint16_t PotVal;
uint32_t VUM;
uint32_t Tick;

extern uint16_t VolCur; /* from adcuser.c */

/* Volume percentage for UI (written in ISR, read in main loop) */
static volatile uint8_t ui_vol_pct = 0;

/* ============================================================
   Potentiometer: read AIN2 (P0.25) into PotVal
   ============================================================ */
static void get_potval(void){
  uint32_t val;

  /* start A/D conversion */
  LPC_ADC->ADCR |= 0x01000000;
  do {
    val = LPC_ADC->ADGDR;                 /* read global data register */
  } while ((val & 0x80000000) == 0);      /* wait end of conversion    */
  LPC_ADC->ADCR &= ~0x01000000;           /* stop conversion           */

  PotVal = ((val >> 8) & 0xF8) +          /* extract potentiometer     */
           ((val >> 7) & 0x08);
}

/* ============================================================
   Small splash screen
   ============================================================ */
static void show_splash_screen(void){
  GLCD_Clear(Blue);
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(White);

  GLCD_DisplayString(3, 2, __FI, (unsigned char*)"  USB MP3 Player  ");
  GLCD_DisplayString(5, 4, __FI, (unsigned char*)"   Loading...   ");

  /* short delay */
  for (volatile uint32_t d = 0; d < 150000; d++) {
    __NOP();
  }
}

/* ============================================================
   Simple volume UI: just a number, no bar
   ============================================================ */
static uint8_t last_vol_pct = 255;

static void draw_volume_ui(uint8_t vol_pct){
  if (vol_pct == last_vol_pct) return;
  last_vol_pct = vol_pct;

  GLCD_Clear(White);

  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)"  MP3 Player (USB)  ");
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);

  GLCD_DisplayString(2, 0, 1, (unsigned char*)"USB Audio: CONNECTED");
  GLCD_DisplayString(4, 0, 1, (unsigned char*)"Volume (POT):");

  char buf[16];
  sprintf(buf, "%3u%%", vol_pct);
  GLCD_DisplayString(4, 14, 1, (unsigned char*)buf);

  GLCD_DisplayString(7, 0, 1, (unsigned char*)"SELECT = Exit");
}

/* ============================================================
   TIMER0 IRQ: audio out + pot/volume update (NO GLCD here)
   ============================================================ */
void TIMER0_IRQHandler(void){
  long      val;
  uint32_t  cnt;

  if (DataRun) {
    val = DataBuf[DataOut];                 /* get audio sample           */
    cnt = (DataIn - DataOut) & (B_S - 1);   /* buffer fill level          */

    if (cnt == (B_S - P_C*P_S)) { DataOut++; }  /* too much data, skip one */
    if (cnt >  (P_C*P_S))       { DataOut++; }  /* still enough data       */
    DataOut &= B_S - 1;

    if (val < 0) VUM -= val; else VUM += val;   /* accumulate for VU meter */

    val  *= Volume;                        /* apply volume               */
    val >>= 16;
    val  += 0x8000;                        /* add bias                   */
    val  &= 0xFFFF;
  } else {
    val = 0x8000;                          /* DAC mid point              */
  }

  if (Mute) {
    val = 0x8000;
  }

  /* write to DAC */
  LPC_DAC->DACR = val & 0xFFC0;

  /* every 1024th tick (~31 Hz @ 32 kHz sample rate):
     update pot, Volume, and ui_vol_pct */
  if ((Tick++ & 0x03FF) == 0) {
    get_potval();

    /* chained USB volume (VolCur) and pot (PotVal) */
    if (VolCur == 0x8000) {        /* minimum level (mute) */
      Volume = 0;
    } else {
      Volume = VolCur * PotVal;
    }

    long vu = (long)(VUM >> 20);   /* VU meter (not displayed here) */
    VUM = 0;
    if (vu > 7) vu = 7;

    /* map pot (0..~255) to 0..100% for UI */
    uint32_t pot_raw = PotVal & 0x3FF;  /* 0..1023 */
    ui_vol_pct = (uint8_t)((pot_raw * 100U) / 1023U);
  }

  LPC_TIM0->IR = 1;                /* clear interrupt flag */
}

/* ============================================================
   Main MP3 Player mode
   ============================================================ */
void runMusicPlayer(void){
  volatile uint32_t pclkdiv, pclk;

  /* 1) Splash screen */
  show_splash_screen();

  /* 2) System clock update */
  SystemClockUpdate();

  /* 3) ADC + DAC pins: P0.25 = AD0.2, P0.26 = AOUT */
  LPC_PINCON->PINSEL1 &= ~((0x03<<18)|(0x03<<20));
  LPC_PINCON->PINSEL1 |=  ((0x01<<18)|(0x02<<20));

  /* 4) Enable ADC peripheral clock */
  LPC_SC->PCONP |= (1 << 12);

  /* 5) ADC & DAC setup (same as Keil USB audio example) */
  LPC_ADC->ADCR  = 0x00200E04;             /* ADC: 10-bit AIN2 @ 4MHz    */
  LPC_DAC->DACR  = 0x00008000;             /* DAC mid point              */

  /* 6) Timer0: 32 kHz sample rate */
  pclkdiv = (LPC_SC->PCLKSEL0 >> 2) & 0x03;
  switch (pclkdiv){
    case 0x01: pclk = SystemFrequency;    break;
    case 0x02: pclk = SystemFrequency/2;  break;
    case 0x03: pclk = SystemFrequency/8;  break;
    case 0x00:
    default:   pclk = SystemFrequency/4;  break;
  }

  LPC_TIM0->MR0 = pclk/DATA_FREQ - 1;
  LPC_TIM0->MCR = 3;                      /* interrupt + reset on MR0   */
  LPC_TIM0->TCR = 1;                      /* enable timer               */
  NVIC_EnableIRQ(TIMER0_IRQn);

  /* 7) USB init & connect (low-level handled in usbhw.c) */
  USB_Init();
  USB_Connect(TRUE);

  /* 8) Initial UI */
  draw_volume_ui(0);
  uint8_t last_drawn = 0;

  /* 9) Main loop: joystick + volume display */
  while (1){
    uint32_t key = get_button();

    /* SELECT (center click) => stop + disconnect + return to menu */
    if (key & KBD_SELECT) {
      NVIC_DisableIRQ(TIMER0_IRQn);
      NVIC_DisableIRQ(USB_IRQn);
      USB_Connect(FALSE);
      GLCD_Clear(White);
      return;
    }

    /* Update volume display if changed (value fed by ISR) */
    uint8_t v = ui_vol_pct;
    if (v != last_drawn){
      last_drawn = v;
      draw_volume_ui(v);
    }

    /* Small debounce / update delay like your Gallery code */
    for (volatile uint32_t d = 0; d < 40000; d++) {
      __NOP();
    }
  }
}
