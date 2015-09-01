/*
 * File: Lab1_TFT.c
 * Author: XiaoXing Zhao
 * Adapted from: TFT_test_BRL4.c by Bruce Land
 * Target PIC: PIC32MX250F128B
 */

//graphics libraries
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"
//need for various functions
#include <stdlib.h>
//threading library
//config.h sets 40 MHz
#define SYS_REQ 40000000
#include "pt_cornell_TFT.h"

//string buffer
char buffer[60];

static int xc=10, yc=150
static PT_THREAD(protothread_blink(struct pt *pt)) {
    PT_BEGIN(pt);
        while(1) {
            //draw circle
            tft_fillCircle(xc, yc, 4, ILI93240_GREEN);
            //yield time 1 second
            PT_YIELD_TIME_msec(1000);
            //erase circle
            tft_fillCircle(xc, yc, 4, ILI93240_BLACK);
            //yield time 1 second
            PT_YIELD_TIME_msec(1000);
	}
    PT_END(pt)
}

static PT_THREAD(protothread_cap_read(struct pt *pt)) {
    PT_BEGIN(pt);
    tft_setCursor(0,0);
    tft_setTextColor(ILI9340_WHITE);
    tft_setTextSize(3);
    tft_writeString("Measuring capacitance:\n"    
        while(1) {
            tft_setCursor(0,10);
            tft_setTextColor(ILI9340_YELLOW);
            tft_setTextSize(3);
            if (capacitance == 0) {
                tft_writeString("No capacitor detected");
            }
            else if (capacitance == 999) {
                tft_writeString("Capacitor out of range!");
            }
            else {
                sprintf(buffer, "%5.1f", capacitance);
            }
            tft_writeString(buffer); 
            PT_YIELD_TIME_msec(200);
        }
    PT_END(pt);
}            

