/*
 * File:        TFT_keypad_BRL4.c
 * Author:      Bruce Land
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

// graphics libraries
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>

// threading library
#include <plib.h>
// config.h sets 40 MHz
#define	SYS_FREQ 40000000
#include "pt_cornell_TFT.h"

#define NoPush 0
#define MaybePush 1
#define Pushed 2
#define MaybeNoPush 3

/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// string buffer
char buffer[60];
char DialBuffer[60];

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_key, pt_key_state ;

// system 1 second interval tick
int sys_time_seconds ;

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
     tft_setCursor(0, 0);
     tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
     tft_writeString("Time in seconds since boot\n");
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        
        // draw sys_time
        tft_fillRoundRect(0,10, 100, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
        tft_writeString(buffer);
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === Keypad Thread =============================================
// connections:
// A0 -- row 1 -- thru 300 ohm resistor -- avoid short when two buttons pushed
// A1 -- row 2 -- thru 300 ohm resistor
// A2 -- row 3 -- thru 300 ohm resistor
// A3 -- row 4 -- thru 300 ohm resistor
// B7 -- col 1 -- 10k pulldown resistor -- avoid open circuit input when no button pushed
// B8 -- col 2 -- 10k pulldown resistor
// B9 -- col 3 -- 10k pulldown resistor

int press;

static PT_THREAD (protothread_key(struct pt *pt))
{
    PT_BEGIN(pt);
    static int keypad, i, pattern;
    // order is 0 thru 9 then * ==10 and # ==11
    // no press = -1
    // table is decoded to natural digit order (except for * and #)
    // 0x80 for col 1 ; 0x100 for col 2 ; 0x200 for col 3
    // 0x01 for row 1 ; 0x02 for row 2; 0x04 for row 3; 0x08 for row 4
    static int keytable[12]={0x101, 0x208, 0x108, 0x88, 0x204, 0x104, 0x84, 0x202, 0x102, 0x82, 0x201, 0x81};
    
    // init the keypad pins A0-A3 and B7-B9
    // PortA ports as digital outputs
    mPORTASetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 | BIT_3);    //Set port as output
    // PortB as inputs
    mPORTBSetPinsDigitalIn(BIT_7 | BIT_8 | BIT_9);    //Set port as input

      while(1) {

        // read each row sequentially
        mPORTAClearBits(BIT_0 | BIT_1 | BIT_2 | BIT_3);
        pattern = 1; mPORTASetBits(pattern);
        
        // yield time
        PT_YIELD_TIME_msec(30);

        for (i=0; i<4; i++) {
            keypad  = mPORTBReadBits(BIT_7 | BIT_8 | BIT_9);
            if(keypad!=0) {keypad |= pattern ; break;}
            mPORTAClearBits(pattern);
            pattern <<= 1;
            mPORTASetBits(pattern);
        }

        // search for keycode
        if (keypad > 0){ // then button is pushed
            for (i=0; i<12; i++){
                if (keytable[i]==keypad) {
                        press = i;      //set press to indicate the key
                        break;
                }
            }
        }
        else {
            i = -1; // no button pushed
            press = -1;      //reset press
        }

        // draw key number
        tft_fillRoundRect(30,200, 100, 28, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(30, 200);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(4);
        sprintf(buffer,"%d", i);
        if (i==10)sprintf(buffer,"*");
        if (i==11)sprintf(buffer,"#");
        tft_writeString(buffer);

        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // keypad thread

int PushState = NoPush;
int DialArray[12];
int ArrayPtr = 0;
int CursorX = 3;

static PT_THREAD (protothread_key_state(struct pt *pt))
{
  PT_BEGIN(pt);
  
  while(1) {
    PT_YIELD_TIME_msec(30) ;

    switch (PushState) {

       case NoPush: 
          if (press > -1) PushState=MaybePush;
          else PushState=NoPush;
          break;

       case MaybePush:
          if (press > -1) {
             PushState=Pushed;   
             if (press < 10) {      //Key is from 0-9
                if (ArrayPtr < 12) {
                    DialArray[ArrayPtr] = press; //insert key to array
                    ArrayPtr++; //increment ArrayPtr
                    tft_setCursor(CursorX, 50); 
                    tft_setTextColor(ILI9340_GREEN); tft_setTextSize(3);
                    sprintf(DialBuffer,"%d", press);
                    tft_writeString(DialBuffer);//write updated array to TFT
                    CursorX += 18; //increment width between numbers
                }
             }
             else {               // key is * or #
                if (press = 10)    {  //key is *
                    memset(DialArray, 0, 12);  //clear DialBuffer
                    ArrayPtr = 0;               //reset ArrayPtr
                    CursorX = 0;                //reset cursor
                    tft_fillRoundRect(0, 50, 240, 40, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                }
                else if (press = 11) {  //key is #
                    //[start tone];       //start sound
                }
             }
          }
          else PushState=NoPush;
          break;

       case Pushed:  
          if (press > -1) PushState=Pushed; 
          else PushState=MaybeNoPush;    
          break;

       case MaybeNoPush:
          if (press > -1) PushState=Pushed; 
          else PushState=NoPush;    
          break;

    } // end case

  } // end while

   PT_END(pt);
} // end thread

// === Main  ======================================================
void main(void) {
 SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;

  // === config threads ==========
  // turns OFF UART support and debugger pin
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_timer);
  PT_INIT(&pt_key);
  PT_INIT(&pt_key_state);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240

  // seed random color
  srand(1);

  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_key(&pt_key));
      PT_SCHEDULE(protothread_key_state(&pt_key_state));
      }
  } // main

// === end  ======================================================

