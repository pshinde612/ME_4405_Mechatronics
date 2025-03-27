#include "C:\Users\parth\OneDrive\Desktop\ME 4405\source\ti\devices\msp432p4xx\driverlib\driverlib.h"
#include <stdio.h>
#include <msp.h>
#include <stdbool.h>

#define FLASH_ADDR 0x1FE000   // Address of Main Memory, Bank 1, Sector 25
#define timerA_divider   TIMER_A_CLOCKSOURCE_DIVIDER_64   // Means counter is incremented at 1E+6/64 = 15625 Hz

volatile char buffer[60];
volatile float data[30];
volatile uint16_t ADCresult = 0 ;
volatile int count=0;
int i =0;
int j =0;
int flag =0;


// ===== Configure UART Communication at 9600 bps ===== //
const Timer_A_UpModeConfig upConfig_0 = // Configure counter in Up mode
{   TIMER_A_CLOCKSOURCE_SMCLK,              // Tie Timer A to SMCLK
    timerA_divider,                         // Increment counter every 64 clock cycles
    46875,                         // Period of Timer A (this value placed in TAxCCR0)
    TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer A rollover interrupt
    TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,     // Enable Capture Compare interrupt
    TIMER_A_DO_CLEAR                        // Clear counter upon initialization
};
const eUSCI_UART_ConfigV1 UART_config = {
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,
        19,
        8,
        85,
        EUSCI_A_UART_NO_PARITY,
        EUSCI_A_UART_LSB_FIRST,
        EUSCI_A_UART_ONE_STOP_BIT,
        EUSCI_A_UART_MODE,
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,
        EUSCI_A_UART_8_BIT_LEN
};

// ===== MAIN PROGRAM LOOP ===== //
void main(){

    // Disable the Watchdog Timer
    WDT_A_holdTimer() ;
    FPU_enableModule();

    // Set DCO Frequency to 3 Mhz
    CS_setDCOFrequency(3E+6);

    // Set up Sub-Master Clock
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // ***** Setup GPIO interrupt for flash read *****
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1|GPIO_PIN4);
    GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN0);

    // Configure P1.2 and P1.3 as UART TX/RX
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,GPIO_PIN2|GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    // Initialize UART MODULE 0
    UART_initModule(EUSCI_A0_BASE, &UART_config);
    UART_enableModule(EUSCI_A0_BASE);

    // 1. Disable All Interrrupts
    Interrupt_disableMaster();

    // 2. Select falling edge for push button
    GPIO_interruptEdgeSelect(GPIO_PORT_P1,GPIO_PIN1|GPIO_PIN4,GPIO_HIGH_TO_LOW_TRANSITION);

    // 3. Clear interrupt flags
    GPIO_clearInterruptFlag(GPIO_PORT_P1,GPIO_PIN1|GPIO_PIN4);

    // 4. Arm interrupts on those pins
    GPIO_enableInterrupt(GPIO_PORT_P1,GPIO_PIN1|GPIO_PIN4);

    // 5. Set priority
    Interrupt_setPriority(INT_PORT1,1);

    // 6. Enable interrupts on those ports
    //Interrupt_enableInterrupt(INT_PORT1);
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
   ///////////////////////////////////////////////////////////////////////////// 7. Enable NVIC
   ADC14_enableConversion();
   Interrupt_disableMaster();

    // Infinite loop
    while(1){
        if(GPIO_getInputPinValue(GPIO_PORT_P1,GPIO_PIN1)==0){
            for(i=0;i<=2000;i++){}//for loop to prevent bouncing
            Interrupt_enableMaster();
        }
        else if (GPIO_getInputPinValue(GPIO_PORT_P1,GPIO_PIN4)==0){
            for(i=0;i<=2000;i++){}//for loop to prevent bouncing
            GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN0);

            float *data_from_flash = (float*) FLASH_ADDR;
            for (j=0; j<30;j++){
                sprintf(buffer,"The temperature is %0.1f", data_from_flash[j]);
                for(i = 0; i <50; i++){
                   while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;                  // (UCA0IFG & 0x02) == 0 will evaluate to true when TXIFG bit is set, false when TXIFG bit is clear
                   UART_transmitData(EUSCI_A0_BASE, buffer[i]) ;  // Send each character in the text string through UART
                }
                while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;
                UART_transmitData(EUSCI_A0_BASE, 0x0D) ;

                while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;
                UART_transmitData(EUSCI_A0_BASE, 0x0A) ;
            }
      }
   }
}

// This Timer A ISR is triggered every one second for a 1Hz FSR sample rate
void TA0_0_IRQHandler(){
    // Clear the timer A0 interrupt

    ADC14_toggleConversionTrigger();              // Sets the SC bit (trigger)
    while(ADC14_isBusy()){}                      // Wait for conversation to finish
    ADCresult = (uint16_t)ADC14_getResult(ADC_MEM0) ;     // Read ADC result from memory
    data[count] = (2.5*ADCresult/1024)*60;
    printf("%f\r\n", data[count]);
    count++;
    GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN0);
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
    if(count>30){
     count =0;
    // Step 1. Unprotect Main Bank 1, Sector 31
    FlashCtl_A_unprotectMemory(FLASH_ADDR,FLASH_ADDR);
    // Step 2. Erase the sector. Within this function, the API will automatically try to
    // erase the maximum number of tries. If it fails, notify user.
    if(!FlashCtl_A_eraseSector(FLASH_ADDR))
        printf("Erase failed\r\n");
    // Step 3. Write the data to memory. If the write attempt fails, display an error message
    if(!FlashCtl_A_programMemory(&data, (void*) FLASH_ADDR, 145))
        printf("Write attempt failed\r\n");
    // Step 4. Set the sector back to protected
    FlashCtl_A_protectMemory(FLASH_ADDR,FLASH_ADDR);
    printf("Write to memory completed\r\n") ;
    Interrupt_disableMaster();
  }

}
