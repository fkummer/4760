#include "Lab1Config.h"

void capTimerSetup(){
    
  //This setup code is adapted from code found in pt_cornell.h, by Bruce Land
  // ===Set up timer2 ======================
  // timer 2: off,  no interrupts, internal clock, prescalar 32,
  // ticks once/microsec!
  // set up to count millsec 32 MHz/32 = 1 MHz
  OpenTimer2(T2_ON  | T2_SOURCE_INT | T2_PS_1_32 , 0);
  // set up the timer interrupt with a priority of 2
  ConfigIntTimer2(T2_INT_OFF | T2_INT_PRIOR_2);
  mT2ClearIntFlag(); // and clear the interrupt flag
}

//Input capture setup
void IC1setup(){
    //Setup Input Capture 1
    //Turn IC1 On, Set it to 16 bits, set to user timer 2, set it to 1 capture per interrupt,
    //and set it to capture on every rising edge
    OpenCapture1(IC_ON | IC_FEDGE_RISE | IC_CAP_16BIT | IC_TIMER2_SRC | IC_INT_1CAPTURE | IC_EVERY_RISE_EDGE); 
    
    //Configure the input capture interrupt
    ConfigIntCapture1(IC_INT_ON | IC_INT_PRIOR_2);
    mIC1ClearIntFlag();
    //Enable the interrupt on capture event
    EnableIntIC1;
    
}

//Comparator setup
void Comp1Setup(){
    //Setup Comparator 1
    //Enable the comparator and its output, set it non-inverting, set its positive input to C1IN, set negative input to IVREF
    CMP1Open(CMP_ENABLE | CMP_OUTPUT_ENABLE | CMP_OUTPUT_NONINVERT | CMP_POS_INPUT_C1IN_POS | CMP1_NEG_INPUT_IVREF);
}

//Assign peripheral pins
void PerPinSetup(){
    //Set the Input Capture input to RPB13
    IC1R = 0x0003;
    //Comparator input C1INA is already mapped to pin 7
    
    //Set C1Out to RPB9 which is pin 18
    RPB9R = 0x0007;
}

////Input Capture Event Interrupt
//void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl2) IC1Handler(void)
//{
//    //captureTime = mIC1ReadCapture();
//    mIC1ClearIntFlag();
//}