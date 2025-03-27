#include "msp.h"
#include "driverlib.h"
#include "stdio.h"
#include "math.h"
#include "header.h"

#define timerA_divider   TIMER_A_CLOCKSOURCE_DIVIDER_64   // Means counter is incremented at 1E+6/64 = 15625 Hz
#define timerA_period    15625

uint32_t SMCLK_divider = CS_CLOCK_DIVIDER_1 ;
unsigned short bit1 = 0;
unsigned short bit2 =0;
volatile uint16_t ADC1result = 0 ;
volatile uint16_t ADC2result = 0 ;
volatile int phasecount = 0 ;
volatile float ADC1 = 0;
volatile float ADC2 = 0;
volatile float ADC_avg = 0;
volatile float FSR = 0;
volatile float PWM =0;
volatile int count =0;
volatile float force_avg = 0;
int state=0;
int prev_conterr =0;
volatile int dir = 0;

//PID gains and errors
float des_volt = 0;
float dt = 0.25;
float prev_error = 0;
float pidOutput = 0;
float kp = 1 ;
// Integral gain.
float ki = 0.7;
// Derivative gain.
float kd = 0.002;
float control = 0;


const Timer_A_UpModeConfig upConfig_0 = // Configure counter in Up mode
{   TIMER_A_CLOCKSOURCE_SMCLK,              // Tie Timer A to SMCLK
    timerA_divider,                         // Increment counter every 64 clock cycles
    timerA_period,                          // Period of Timer A (this value placed in TAxCCR0)
    TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer A rollover interrupt
    TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,     // Enable Capture Compare interrupt
    TIMER_A_DO_CLEAR                        // Clear counter upon initialization
};

// Main program loop
void main(void)
{
    WDT_A_holdTimer() ;         // Halt the watchdog timer
    CS_setDCOFrequency(1E+6);           // Set DCO to 3 MHz
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, SMCLK_divider);
    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN5);
    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN6);

    // ****** ADC Configuration ***** //
    // Enable ADC module
    ADC14_enableModule();

    // Set functionality of pin P5.5
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN5, GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN2, GPIO_TERTIARY_MODULE_FUNCTION);

    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN5);
    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN6);

    // Set to 14 bit resolution
    ADC14_setResolution(ADC_14BIT);

    // Tie to SMCLK with no divider
    ADC14_initModule(ADC_CLOCKSOURCE_SMCLK,ADC_PREDIVIDER_1,ADC_DIVIDER_1, false);

    // Place result of conversion in ADC_MEM0 register. We will trigger each conversion manually
    ADC14_configureMultiSequenceMode(ADC_MEM0,ADC_MEM2, false);

    // Sample Channel 0 to register ADC_MEM0.  Use 0-3.3V range.
    ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, false);
    ADC14_configureConversionMemory(ADC_MEM2, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A3, false);//pin 5.2

    // We will trigger each conversion manually
    ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

    ////////////////// Set up timer, used to print every ~ sec //////////////
    Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig_0);   // Configure Timer A using above struct
    Interrupt_enableInterrupt(INT_TA0_0);                  // Enable Timer A interrupt

    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);  // Start Timer A
    ///////////////////////////////////////////////////////////////////////////
   TA0CCTL1 = OUTMOD_7 ;    // Macro which is equal to 0x00e0, defined in msp432p401r.h
   TA0CCTL2 = OUTMOD_7;
   TA0CCR1 = 0 ;    // Set duty cycle  to 0 on pin 2.4 and 2.5 on start.
   TA0CCR2 = 0;
   ///// *** Setup Pin 2.4 as PWM output signal *** /////
   P2->SEL0 |= 0x30 ;    // Set bits 4-5 of P2SEL0 to enable TA0.1|TA0.2 functionality on P2.4|P2.5
   P2->SEL1 &= ~0x30 ;   // Clear bits 4-5 of P2SEL0 to enable TA0.1|TA0.2 functionality on P2.4|P2.5
   P2->DIR |= 0x30 ;     // Set pins 2.4-2.5 as an output pin


    // Start conversion
    ADC14_enableConversion();        // Sets the ENC bit

    // Enable timer interrupt
    Interrupt_enableMaster();        // Enable all interrupts

    while(1){
        ADC14_toggleConversionTrigger();              // Sets the SC bit (trigger)
        while(ADC14_isBusy()){}                       // Wait for conversation to finish
        ADC1result = (uint16_t)ADC14_getResult(ADC_MEM0) ;
        ADC2result = (uint16_t)ADC14_getResult(ADC_MEM2) ;
        ADC1 = 3.3*ADC1result/16384;// Read ADC result from memory
        ADC2 = 3.3*ADC2result/16384;// Read ADC result from memory
        ADC_avg = (ADC1+ADC2)/2;

        force_avg = ((force_avg*count) + ADC_avg)/(count+1);
        count = count +1;

        bit1 = GPIO_getInputPinValue(GPIO_PORT_P3,GPIO_PIN5);
        bit2 = GPIO_getInputPinValue(GPIO_PORT_P3,GPIO_PIN6);
        if (bit1){
            if(bit2){
                des_volt = 0.6;
                state =3;
                printf("level3\n\r");
            } else{
                des_volt = 0.4;
                state =2;
                printf("level2\n\r");
            }
        }else {
            if(bit2){
                des_volt = 0.25;
                state =1;
                printf("level1\n\r");
            }else {
                des_volt = 0;
                state =0;
                printf("stationary\n\r");
            }
        }
        control = getPID();
        printf("error control is %f\n\r",control);
      if (control > 0.02*des_volt && control < 1) {
        TA0CCR2 = control * 7000;
        TA0CCR1=0;
      } else if (control>1) {
         TA0CCR2 =  7000;
         TA0CCR1=0;
      } else if (control <=0.02*des_volt && control >=-(0.02)*des_volt) {
          TA0CCR1=0;
          TA0CCR2=0;
      }else{
          TA0CCR1 = fabs(control) * 7000;
          TA0CCR2=0;
      }

    }
}
void TA0_0_IRQHandler(void){
    force_avg =0;
    count =0;

    // Clear Timer A Interrupt Flag
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

float getPID(){
    float err = 0;
    float err_dot =0;
    float windup_const =0.5;
    float int_err = 0;
    err = (des_volt - force_avg)/0.8;
    int_err = int_err + dt*err;
    //Anti-windup routine
    if(abs(int_err) >windup_const){
        if(int_err >0){
            int_err =windup_const;
        }
        else{
            int_err =-windup_const;

        }
    }
    err_dot = (err - prev_error)/dt;
    // Compute PID control.
    pidOutput = (kp*err + ki*int_err + kd*err_dot);
    // Save last error value for next derivative computation.
    prev_error = err;
    return pidOutput;
}
