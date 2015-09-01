/* 
 * File:   Lab1Config.h
 * Author: Fred
 *
 * Created on August 31, 2015, 10:02 PM
 */

//Timer 2 setup for lab 1
//Use timer 2 
#define _SUPPRESS_PLIB_WARNING 1
#include "plib.h"

//Set up timer 2 to use with input capture
void capTimerSetup();

//Input capture setup
void IC1setup();

//Comparator setup
void Comp1Setup();

//Assign peripheral pins
void PerPinSetup();


//Do we need this? Probably not  
/*void __ISR(_TIMER_2_VECTOR, ipl2) CapTimer(void)
{
    // clear the interrupt flag
    mT2ClearIntFlag();
}*/

