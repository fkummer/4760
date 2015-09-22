
#include "pt_cornell_TFT.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>



#define _SUPPRESS_PLIB_WARNING 1
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include "plib.h"

#define SYS_FREQ 64000000
#define uint8_t unsigned char
#define uint16_t unsigned short

#define int16_t short

#define SYS_FREQ 64000000
//4 MHz
#define PBCLK	32000000UL

#define NoPush 0
#define MaybePush 1
#define Pushed 2
#define MaybeNoPush 3

// string buffer
char buffer[60];
char DialBuffer[60];

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_key, pt_key_state ;

// system 1 second interval tick
int sys_time_seconds ;

volatile signed char sine_table[256];

volatile unsigned int DDS_accumulator_1; 
volatile unsigned int DDS_accumulator_2; 
volatile unsigned int DDS_increment_1; 
volatile unsigned int DDS_increment_2; 
volatile unsigned int DDS_amp;

unsigned char sine_index_1;
unsigned char sine_index_2;
int frequency1 = 697;
int frequency2 = 1209;
float multiplier = 32.0;
int ramping = 1;
int sys_time_seconds;
int cntr = 0;
short sampled_value_1 = 0x0000;
short sampled_value_2 = 0x0000;
short added_value = 0x0000;

#define SS      LATBbits.LATB4
#define dirSS   TRISBbits.TRISB4


void initDAC(void){
/* Steps:
 *	1. Setup SS as digital output.
 *	2. Map SDO to physical pin.
 *	3. Configure SPI control and clock with either of a or b:
 *		a. OpenSPIx(config1, config2) and SpiChnSetBrg(spi_channel, spi_brg)
 *		b. SpiChnOpen(spi_channel, config, spi_divider)
 */

	dirSS = 0;					// make SS an output
    SS = 1;						// set SS = 1 to deselect slave
	PPSOutput(2, RPB5, SDO2);	// map SDO2 to RB5

/////////
//#define config1 SPI_MODE16_ON | SPI_CKE_ON | MASTER_ENABLE_ON
//	/*	FRAME_ENABLE_OFF
//	 *	ENABLE_SDO_PIN		-> SPI Output pin enabled
//	 *	SPI_MODE16_ON		-> 16-bit SPI mode
//	 *	SPI_SMP_OFF			-> Sample at middle of data output time
//	 *	SPI_CKE_ON			-> Output data changes on transition from active clock
//	 *							to idle clock state
//	 *	SLAVE_ENABLE_OFF	-> Manual SW control of SS
//	 *	MASTER_ENABLE_ON	-> Master mode enable
//	 */
//#define config2 SPI_ENABLE
//	/*	SPI_ENABLE	-> Enable SPI module
//	 */
////	OpenSPI2(config1, config2);
//	// see pg 193 in plib reference

#define spi_channel	2
	// Use channel 2 since channel 1 is used by TFT display

#define spi_brg	0
	// Divider = 2 * (spi_brg + 1)
	// Divide by 2 to get SPI clock of FPBDIV/2 -> max SPI clock

//	SpiChnSetBrg(spi_channel, spi_brg);
	// see pg 203 in plib reference

//////

//////

#define spi_divider 2
/* Unlike OpenSPIx(), config for SpiChnOpen describes the non-default
 * settings. eg for OpenSPI2(), use SPI_SMP_OFF (default) to sample
 * at the middle of the data output, use SPI_SMP_ON to sample at end. For
 * SpiChnOpen, using SPICON_SMP as a parameter will use the non-default
 * SPI_SMP_ON setting.
 */
#define config SPI_OPEN_MSTEN | SPI_OPEN_MODE16 | SPI_OPEN_DISSDI | SPI_OPEN_CKE_REV
	/*	SPI_OPEN_MSTEN		-> Master mode enable
	 *	SPI_OPEN_MODE16		-> 16-bit SPI mode
	 *	SPI_OPEN_DISSDI		-> Disable SDI pin since PIC32 to DAC is a
	 *							master-to-slave	only communication
	 *	SPI_OPEN_CKE_REV	-> Output data changes on transition from active
	 *							clock to idle clock state
	 */

SpiChnOpen(spi_channel, config, spi_divider);

}

//Unchanged from Tahmid, shouldn't need any alteration..
inline void writeDAC(uint16_t data){
    SS = 0;	// select slave device: MCP4822 DAC
    while (TxBufFullSPI1());	// ensure buffer is free before writing
    WriteSPI2(data);			// send the data through SPI
    while (SPI2STATbits.SPIBUSY); // blocking wait for end of transaction
    SS = 1;	// deselect slave device, transmission complete
}



void ramper(){
    cntr += 1;
    if(cntr > 0 && cntr <= 186){
        if(cntr%6==0){
            multiplier -= 1;
        }
    }else{
        if(cntr>186 && cntr <= 3386){
            multiplier = 1;
        }else{
            if(cntr>3386 && cntr <= 3572){
                if(cntr%6==0){
                    multiplier += 1;
                }
            }
        }
    }
    
    if(cntr > 3572 && cntr <= 6772){
        
    }
    
    if(cntr > 6772){
        cntr = 0;
        multiplier = 32.0;
    }
          
}

// DDS
void __ISR(_TIMER_2_VECTOR, ipl2) Time2Handler(void)
{
    // generate a trigger strobe for timing other events
    mPORTBSetBits(BIT_0);
    // increment DDS accumulator to get next value in sine wave
    DDS_accumulator_1 += DDS_increment_1;
    DDS_accumulator_2 += DDS_increment_2;
	// shift value in accumulator to get the most significant bits
	// need to truncate value because sine table has 8bit addresses
    sine_index_1 = DDS_accumulator_1>>24 ;
    sine_index_2 = DDS_accumulator_2>>24;
	//  !!not sure what DDS amp is
    sampled_value_1 = (sine_table[sine_index_1]<<4)/multiplier;
    sampled_value_2 = (sine_table[sine_index_2]<<4)/multiplier;
    
    
    added_value = (sampled_value_1 + sampled_value_2)+1024;
    ramper();

    writeDAC(0x3000 | added_value);
    // would need to pass sampled_value to DAC I think
    // clear the timer interrupt flag
    mT2ClearIntFlag();
    mPORTBClearBits(BIT_0);
}


// === Command Thread ======================================================
// The serial interface
static char cmd[16]; 
static int value;

static PT_THREAD (protothread_cmd(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
          
			// M = (fout*2^N)/fc
			// 4294967296 = 2^32
            
			
      } // END WHILE(1)
  PT_END(pt);
} // thread 3

// update a 1 second tick counter, could be useful for real time if we need it
static PT_THREAD (protothread_time(struct pt *pt))
{
    PT_BEGIN(pt);

      while(1) {
            // yield time 1 second
            PT_YIELD_TIME_msec(1000);
            sys_time_seconds++;
            // NEVER exit while
      } // END WHILE(1)

  PT_END(pt);
} // thread 

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

int main(void)
{
    SYSTEMConfigPerformance(PBCLK);
    ANSELA = 0; 
    ANSELB = 0; 
    CM1CON = 0; 
    CM2CON = 0;
    // timer two uses an internal clock source and runs at 32MHz
    DDS_increment_1 = (frequency1*4294967296)/32000;
    DDS_increment_2 = (frequency2*4294967296)/32000;
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_32, 31);
    // Enable timer 2 interrupts with priority 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    // Clear interupt flag
    mT2ClearIntFlag();
    initDAC();
    // MAP PINS/SET I/O PINS

    PT_setup();

    // create sinetable
    // 6.283 = 2pi
    // 32 = amplitude
    // !!unsure why signed char, and amplitude = 32
    // 256 samples
    int i;
        for (i=0; i <256; i++){
            sine_table[i] = (signed char) (32.0 * sin((float)i*6.283/(float)256));  
        }

    // Setup Multivector interrupts
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

    // round-robin scheduler for threads
    while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_key(&pt_key));
      PT_SCHEDULE(protothread_key_state(&pt_key_state));
    }
}