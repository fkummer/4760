/* 
 * File:   Capacitor_Charging.c
 * Author: Douglas
 *
 * Created on August 31, 2015, 9:44 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include<math.h>

#define VDD 3.3;
#define DISCHARGE_TIME 1000;  // 1msec
#define PT_THREAD pt;  // change this
#define RESISTOR 470;
#define VREF 1.2;

void capacitor(void);

int charged;
float charge_time;
float capacitance;

int main(int argc, char** argv) {

    return (EXIT_SUCCESS);
}

void capacitor(){
// Discharge Capacitor
    PT_BEGIN(PT_THREAD)
    // Set up discharge timer (timer3)
    T2CON = 0x0; // Disable and clear timer
    TMR2 = 0x0;
    PR2 = DISCHARGE TIME; // Set the period so the timer will have time to discharge
    T2CON = 0x30; // Set the timer prescalar to 1:8 (1MHz)
    
    // Enable timer interrupts
    IFS0CLR = 0x00000009; // Clear the timer interrupt status flag
    IEC0SET = 0x00000009; // Enable timer interrupts
    
    // Set pin as output and start timer
    TRISBSET = 0x3;    // Set PORTB3 as an output
    LATBCLR = 0x3;    // Set PORTB3 to zero
    T2CONSET = 0x8000; // Start the timer
    
    // Yield until discharge is complete
    charged = 0;
    PT_YIELD_UNTIL(PT_THREAD, charged);
    charged = 0;
    
// Charge Capacitor

    // Set up charge timer
    T2CON = 0x0;
    IEC0CLR = 0x00000009; // Disable timer interrupts
    T2CON = 0x30; // Set the timer prescalar to 1:8 (1MHz)
    
    // Set pin as input and start timer
    TRISBCLR = 0x3;    // Set PORTB3 as an input
    T2CONSET = 0x8000; // Start the timer
    
    // Yield while waiting for event from comparator
    PT_YIELD_UNTIL(PT_THREAD, charged);
    charged = 0;
    
    // Calculate capacitance
    capacitance = 1000000000 * ((log(1-(VREF / VDD)) * RESISTOR) / charge_time);
    
    if(capacitance < 1 || capacitance > 100){
        capacitance = 0;
    }  // Check if capacitance is out of range 1nF-100n
    
    PT_END(PT_THREAD)
}


// ***Interrupts are not correct right now***
void __ISR(_Timer_2_Vector,ip9)Timer2Handler(void)
{
IFS0CLR = 0x00000009; // Clear Timer2 interrupt flag
T2CON = 0x0;          // Disable and clear timer2
charged = 1;          // Cause the capacitor thread to stop yielding
}

// 
void __ISR(__EVENT_CAPTURE,ip2){
    charge_time = TMR2;  // Get charge time from timer2 after input capture
    IFSOCLR = 0x6;       // clear interrupt flag
    charged = 1;         // Cause capacitor thread to stop yielding
}
