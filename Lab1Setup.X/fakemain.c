
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


volatile int charged;  // Signals charging is complete
volatile float charge_time; // Time taken for the capacitor to charge
float capacitance = 0.0; // Value of capacitor in nF

static struct pt pt_cap, pt_blink, pt_cap_read;

//string buffer
char buffer[60];
static int xc=310, yc=230;


// This thread is responsible for all the code not involving the TFT Display, it handled discharging and charging the capacitor and calculating the 
// capacitance of the capacitor.
static PT_THREAD(protothread_cap(struct pt *pt)){
    PT_BEGIN(pt);
        
    while(1){
        // Discharge Capacitor
        // Set pin as output
        mPORTBSetPinsDigitalOut(BIT_3);    
        // Drive RB3 low so capacitor can discharge into the pin
        mPORTBClearBits(BIT_3);

        // Yield until discharge is complete 
        PT_YIELD_TIME_msec(2); // 2ms is given for the capacitor to discharge and for the display to update if needed
        Comp1Setup();
		
		// Charge Capacitor
		// Set RB3 as an input to detect capacitor's current charge
        mPORTBSetPinsDigitalIn(BIT_3); 
		// Set up the timer for charging the capcitor
        capTimerSetup();
		// Set up the input capture to capture when the capacitor voltage reaches the reference voltage
        IC1setup();
		
        // Yield while waiting for event from comparator
        PT_YIELD_UNTIL(pt, charged);
        CloseTimer2();
        
		// Reset thread wait variable
        charged = 0;

        // Calculate capacitance in nF
        capacitance =  (((charge_time*-1)/1000000)/(log(1-(VREF / VDD)) * RESISTOR))*1000000000;

        CMP1Close();
        PT_YIELD_TIME_msec(20);
    }
    PT_END(pt);
}



// This thread blinks a circle on the display every second as a heartbeat
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

// This thead displays the value of the capcitor if it is in the 1nF-100nF range
// or diplays a message if the capacitor is out of range
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
            if (capacitance < 1 || capacitance > 99) {
                tft_fillRoundRect(0,30, 320, 40, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                tft_writeString("Capacitor out of range!");
            }
            else {
                tft_fillRoundRect(0,30, 320, 40, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                sprintf(buffer, "%4.1f nF", capacitance);
                tft_writeString(buffer);
            }
            PT_YIELD_TIME_msec(200);
        }
    PT_END(pt);
}            


int main(int argc, char** argv) {
    PT_setup();
    
	// Enable multivector interrupts
    INTEnableSystemMultiVectoredInt();
	// Initialize threads
    PT_INIT(&pt_cap);
    
    
    PT_INIT(&pt_blink);
    PT_INIT(&pt_cap_read)
    
    //init the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    
	// Set orientation of the display
    tft_setRotation(1);
	
	// Set up pins
    PerPinSetup();
    while(1){
        PT_SCHEDULE(protothread_blink(&pt_blink));
        PT_SCHEDULE(protothread_cap(&pt_cap));
        PT_SCHEDULE(protothread_cap_read(&pt_cap_read));
    }
  
}

// Input Capture Event Interrupt
// On input capture this interrupt will get the time taken to charge the capacitor
// and signal the capacitor thread to continue
void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl2) IC1Handler(void)
{
    charge_time = mIC1ReadCapture();
    CloseCapture1();
    mIC1ClearIntFlag();
    
    charged = 1;
}