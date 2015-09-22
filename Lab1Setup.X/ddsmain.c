
#include "pt_cornell_TFT.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>



#define _SUPPRESS_PLIB_WARNING 1
#include "config.h"
//#include "tft_master.h"
//include "tft_gfx.h"
#include "plib.h"

#define SYS_FREQ 64000000
#define uint8_t unsigned char
#define uint16_t unsigned short

#define int16_t short

//4 MHz
#define PBCLK	32000000UL

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

//I UNCOMMENTED THIS SO WE COULD ACTUALLY OPEN THE SPI
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

int main(void)
{
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
//    cntr = cntr+1;
//    
//    if(cntr%500 == 0){
//        cntr = 1;
//        multiplier += 1.0;
//    }
//    
//    if(multiplier >= 32.0){
//        cntr = 1;
//        multiplier = 1.0;
//    }
//    
//    sampled_value += 0x0001;
//    if(sampled_value >= 0x0FFE){
//        sampled_value = 0x0000;
//    }
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

