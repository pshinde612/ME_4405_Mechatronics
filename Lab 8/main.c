#include "msp.h"
#include <driverlib.h>
#include <stdio.h>

volatile unsigned int result = 0;
volatile int PWM = 0;
volatile int states1 = 0;
volatile int states2 = 0;
volatile int i=0;
volatile int dir =0;
volatile int s1_on =0;
volatile int s2_on =0;
volatile uint16_t ADC0 = 0;
volatile uint16_t ADC1 = 0;
volatile uint16_t ADC3 = 0;
volatile uint16_t FSR1 = 0;
volatile uint16_t FSR2 = 0;
volatile uint16_t FSR_avg = 0;

// Configure Timer A for UpMode operation
const Timer_A_UpModeConfig upConfig_0 =
{    TIMER_A_CLOCKSOURCE_SMCLK,
     TIMER_A_CLOCKSOURCE_DIVIDER_1,
     60000,
     TIMER_A_TAIE_INTERRUPT_DISABLE,
     TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
     TIMER_A_DO_CLEAR
};

// Main Loop
void main(){
    unsigned short usibutton1 = 0;
    unsigned short usibutton2 = 0;

    // Disable the Watchdog Timer
    WDT_A_holdTimer() ;

    // Set DCO Frequency to 3 Mhz
    CS_setDCOFrequency(3E+6);

    // Set up Sub-Master Clock
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    //set push button input as a pull up resistor
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN4);
    ///// *** Configure Timer A and its interrupt *** /////
    Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig_0);
    Interrupt_enableInterrupt(INT_TA0_0);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    TA0CCTL1 = OUTMOD_7 ;    // Macro which is equal to 0x00e0, defined in msp432p401r.h
    TA0CCTL2 = OUTMOD_7;
    TA0CCR1 = 0 ;    // Set duty cycle  to 0 on pin 2.4 and 2.5 on start.
    TA0CCR2 = 0;
    ///// *** Setup Pin 2.4 as PWM output signal *** /////
    P2->SEL0 |= 0x30 ;    // Set bits 4-5 of P2SEL0 to enable TA0.1|TA0.2 functionality on P2.4|P2.5
    P2->SEL1 &= ~0x30 ;   // Clear bits 4-5 of P2SEL0 to enable TA0.1|TA0.2 functionality on P2.4|P2.5
    P2->DIR |= 0x30 ;     // Set pins 2.4-2.5 as an output pin



    ///// *** ANALOG to DIGITAL CONVERSION SETUP *** /////
    Interrupt_disableMaster();

    // Enable and initialize ADC14
    ADC14_enableModule();
    ADC14_initModule(ADC_CLOCKSOURCE_SMCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1, false);

    // Set up ADC14 GPIO pin
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN5, GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN2, GPIO_TERTIARY_MODULE_FUNCTION);


    // Configure ADC output
    ADC14_setResolution(ADC_14BIT);

    // Configure ADC sampling
    ADC14_configureMultiSequenceMode(ADC_MEM0,ADC_MEM2,false);

    // Configure ADC conversion
    ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, false);//pin5.4
    ADC14_configureConversionMemory(ADC_MEM1, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A1, false);//pin5.5
    ADC14_configureConversionMemory(ADC_MEM2, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A3, false);//pin 5.2
    ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

    // Enable ADC operation and conversion
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();

    // Enable NVIC
    Interrupt_enableMaster();

    // Infinite loop
    while(1){
        //read the input values from the pushbuttons
       usibutton1 = MAP_GPIO_getInputPinValue(GPIO_PORT_P1,GPIO_PIN1);
       usibutton2 = MAP_GPIO_getInputPinValue(GPIO_PORT_P1,GPIO_PIN4);
       if(usibutton1 == GPIO_INPUT_PIN_LOW){
           for(i=0;i<=2000;i++){}//for loop to prevent bouncing
           states1=1;
           states2 = 0;
       } else if(usibutton1 == GPIO_INPUT_PIN_HIGH){
           if(states1 ==1){
               if(s1_on ==1){
                   dir =0;
                   s1_on =0;
               }else{
                   dir =1;
                   s1_on =1;
               }
           }
           states1 = 0;
       }
       if(usibutton2 == GPIO_INPUT_PIN_LOW){
          for(i=0;i<=2000;i++){}//for loop to prevent bouncing
          states2=1;
          states1 = 0;
      }else if(usibutton2 == GPIO_INPUT_PIN_HIGH){
          if(states2 ==1){
              if(s2_on ==1){
                  dir =0;
                  s2_on =0;
              }else{
                  dir =2;
                  s2_on =1;
              }
          }
          states2 = 0;
      }
    }
}

//////////////// *** Timer A Interrupt Service Routine *** ///////////////////
void TA0_0_IRQHandler(void){

    ADC14_toggleConversionTrigger();     // Start ADC conversion
    while(ADC14_isBusy()){}              // Use polling to wait for the ADC14 to finish sampling

    result = ADC14_getResult(ADC_MEM0);       // Retrive and convert results stored in ADC_MEM0
    //ADC1 = 2.5*(ADC14_getResult(ADC_MEM1))/16384;
    //ADC3 = 2.5*(ADC14_getResult(ADC_MEM2))/16384;
    //FSR1 = 719.7*pow(ADC1,4) -2785*pow(ADC1,3)+3691*pow(ADC1,2)-1804*ADC1 +316.8;
    //FSR2 = 719.7*pow(ADC3,4) -2785*pow(ADC3,3)+3691*pow(ADC3,2)-1804*ADC3 +316.8;
    //FSR_avg = (FSR1+FSR2)/2;
    printf("ADC Inputs: %i \r\n", result) ;   // Display results
    PWM = 1000 + 12000*(int)result/10700;     // Update PWM Value
    //PWM = 10000;
    // Save new PWM value to register
    if(dir ==1){
        TA0CCR1 = PWM;
        TA0CCR2 = 0;
        printf("Motor running in dir 1\r\n");    // Display results
    }
    else if(dir == 2){
        TA0CCR2 = PWM;
        TA0CCR1 = 0;
        printf("Motor running in dir 2\r\n");
    }
    else{
        TA0CCR1 = 0;
        TA0CCR2 = 0;
        printf("Motor stopped\r\n");
    }

    // Verify PWM values
    //printf("PWM Value: %i \r\n\n", PWM) ;
    //printf("FSR1 value is: %f \r\n\n",ADC3);
    //printf("FSR2 value is: %f \r\n\n",ADC1);
    //printf("Average force value is: %f \r\n\n",FSR_avg);


    // Clear Timer A Interrupt Flag
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
}
