/* 
 * File:   config.h
 * Author: Douglas, XiaoXing, Fred
 * Adapted from code written by Syed Tahmid
 * Created on August 31, 2015, 9:44 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#define _SUPPRESS_PLIB_WARNING 1

#include "plib.h"
// serial stuff
#include <stdio.h>

//Oscillator selection. This is the Fast RC(8 MHz) with PLL
#pragma config FNOSC = FRCPLL
//Primary oscillator disable
#pragma config POSCMOD = OFF
//PLL Input divider, 2x divider, bringing it to 4 MHz
#pragma config FPLLIDIV = DIV_2
//PLL Multiplier, 16x multiplication, bringing it to 64 MHz
#pragma config FPLLMUL = MUL_16
//System PLL Output divider, divided by 1, bring the sys clock to 64 MHz
#pragma config FPLLODIV = DIV_1
//Peripheral Clock Divider, divided by 2, mean pbclk is 32 MHz
#pragma config FPBDIV = DIV_2
//Watchdog timer disabled
#pragma config FWDTEN = OFF
//JTAG Disabled
#pragma config JTAGEN = OFF
//Secondary oscillator disabled
#pragma config FSOSCEN = OFF

#endif	/* CONFIG_H */

