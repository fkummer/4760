
/* 
 * File:   Capacitor_Charging.c
 * Author: Douglas
 *
 * Created on August 31, 2015, 9:44 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pt_cornell_TFT.h"

#define _SUPPRESS_PLIB_WARNING 1
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include "plib.h"

#define SYS_FREQ 64000000

#define VDD 3.3
#define DISCHARGE_TIME 1000  // 1msec

#define RESISTOR 1075000
#define VREF 1.2

void capacitor(void);

volatile int charged;
volatile float charge_time;
float capacitance = 0.0;

static struct pt pt_cap, pt_blink, pt_cap_read;
//string buffer
char buffer[60];
static int xc=310, yc=230;

static PT_THREAD(protothread_cap(struct pt *pt)){
    PT_BEGIN(pt);
        
    while(1){
            // Discharge Capacitor
        
        // Set pin as output
        mPORTBSetPinsDigitalOut(BIT_3);    
        //Drive RB3 low
        mPORTBClearBits(BIT_3);

        // Yield until discharge is complete
        PT_YIELD_TIME_msec(2);
        Comp1Setup();
    // Charge Capacitor

        mPORTBSetPinsDigitalIn(BIT_3); 
        capTimerSetup();
        IC1setup();
        // Yield while waiting for event from comparator
        PT_YIELD_UNTIL(pt, charged);
        CloseTimer2();
        
        
        charged = 0;

        // Calculate capacitance in nanofarads
        capacitance =  (((charge_time*-1)/1000000)/(log(1-(VREF / VDD)) * RESISTOR))*1000000000;

//        if(capacitance < 1){
//            capacitance = 0;
//        }  // Check if capacitance is non present
//        else if(capacitance > 100){
//            capacitance = 999;
//        }  // Check if capacitance is out of range
        CMP1Close();
        PT_YIELD_TIME_msec(20);
    }
    PT_END(pt);
}




static PT_THREAD(protothread_blink(struct pt *pt)) {
    PT_BEGIN(pt);
        while(1) {
            //draw circle
            tft_fillCircle(xc, yc, 4, ILI9340_GREEN);
            //yield time 1 second
            PT_YIELD_TIME_msec(500);
            //erase circle
            tft_fillCircle(xc, yc, 4, ILI9340_BLACK);
            //yield time 1 second
            PT_YIELD_TIME_msec(500);
	}
    PT_END(pt);
}

static PT_THREAD(protothread_cap_read(struct pt *pt)) {
    PT_BEGIN(pt);
    tft_setCursor(0,0);
    tft_setTextColor(ILI9340_WHITE);
    tft_setTextSize(2);
    tft_writeString("Measuring capacitance:"); 
        while(1) {
            tft_setCursor(0,30);
            tft_setTextColor(ILI9340_YELLOW);
            tft_setTextSize(2);
            //if (capacitance < 1 || capacitance > 99) {
                //tft_writeString("Capacitor out of range!");
            //}
            //else {
                tft_fillRoundRect(0,30, 320, 40, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                sprintf(buffer, "%4.1f", capacitance);
                tft_writeString(buffer); 
            //}
            PT_YIELD_TIME_msec(200);
        }
    PT_END(pt);
}            


int main(int argc, char** argv) {
    PT_setup();
    
    INTEnableSystemMultiVectoredInt();
    PT_INIT(&pt_cap);
    
    
    PT_INIT(&pt_blink);
    PT_INIT(&pt_cap_read)
    
    //init the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    
    tft_setRotation(1);
    PerPinSetup();
    while(1){
        PT_SCHEDULE(protothread_blink(&pt_blink));
        PT_SCHEDULE(protothread_cap(&pt_cap));
        PT_SCHEDULE(protothread_cap_read(&pt_cap_read));
    }
  
}

//Input Capture Event Interrupt
void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl2) IC1Handler(void)
{
    charge_time = mIC1ReadCapture();
    CloseCapture1();
    mIC1ClearIntFlag();
    
    charged = 1;
}