#include "msp.h"
#include "driverlib.h"
#include "stdio.h"

#define timerA_divider   TIMER_A_CLOCKSOURCE_DIVIDER_64   // Means counter is incremented at 1E+6/64 = 15625 Hz
#define timerA_period    15625
#define LED2 GPIO_PIN2|GPIO_PIN1|GPIO_PIN0//define pins for second LED

uint32_t SMCLK_divider = CS_CLOCK_DIVIDER_1 ;
volatile uint16_t ADCresult = 0 ;
volatile int phasecount = 0 ;
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
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, LED2);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
    // ****** ADC Configuration ***** //
    // Enable ADC module
    ADC14_enableModule();

    // Set functionality of pin P5.5
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5,GPIO_PIN5,GPIO_TERTIARY_MODULE_FUNCTION);

    // Set to 14 bit resolution
    ADC14_setResolution(ADC_10BIT);

    // Tie to SMCLK with no divider
    ADC14_initModule(ADC_CLOCKSOURCE_SMCLK,ADC_PREDIVIDER_1,ADC_DIVIDER_1, false);

    // Place result of conversion in ADC_MEM0 register. We will trigger each conversion manually
    ADC14_configureSingleSampleMode(ADC_MEM0, false);

    REF_A_setReferenceVoltage(REF_A_VREF2_5V);

    // Sample Channel 0 to register ADC_MEM0.  Use 0-3.3V range.
    ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_INTBUF_VREFNEG_VSS, ADC_INPUT_A0, false);


    // We will trigger each conversion manually
    ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

    ////////////////// Set up timer, used to print every ~ sec //////////////
    Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig_0);   // Configure Timer A using above struct
    Interrupt_enableInterrupt(INT_TA0_0);                  // Enable Timer A interrupt

    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);  // Start Timer A
    ///////////////////////////////////////////////////////////////////////////

    // Start conversion
    ADC14_enableConversion();        // Sets the ENC bit

    // Enable timer interrupt
    Interrupt_enableMaster();        // Enable all interrupts

    while(1){
        // No polling or for loop delays
    }
}

// This Timer A ISR is triggered every one second for a 1Hz FSR sample rate
void TA0_0_IRQHandler(){
    ADC14_toggleConversionTrigger();              // Sets the SC bit (trigger)
    while(ADC14_isBusy()){}                       // Wait for conversation to finish
    ADCresult = (uint16_t)ADC14_getResult(ADC_MEM0) ;     // Read ADC result from memory
    float tmp = (2.5*ADCresult/1024)*60;
    printf("%f\r\n", tmp) ;
    if(tmp>80){
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);//RED on
    }
    else {
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);//RED off
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);

    }

    // Clear the timer A0 interrupt
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
}


