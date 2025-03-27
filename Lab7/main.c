#include "msp.h"
#include <driverlib.h>
#include <stdio.h>
#include "motorfunctions.h"
//DEFINE VARIABLES
#define timerA_divider TIMER_A_CLOCKSOURCE_DIVIDER_64 //increment counter every 1 second
#define timerA_period 46875

//DECLARE VARIABLES
char prompt[30] = "enter a number from 0 to 60";
char promptStart[7] = "Start?";
char promptError[7]="Error!";
volatile char promptText[10]= "";//store characters from UART buffer.
volatile int timer=0;
volatile char receivedBuffer[3]="";//buffer to receive characters from UART
volatile int state=0;//state change variable
volatile int phasecount = 0; // counts motor phases 0-7
volatile int count=0;//keeps track of text read from buffer
volatile int counter=0;//step counter for cw
volatile int dir=0; //variable to determine change of direction
volatile int step= 0;
volatile int i=0;
volatile int pcount =0;// to check if start prompt has been called.
//SETUP TIMER A
const Timer_A_UpModeConfig upConfig_0= //configure timer in Up mode
{   TIMER_A_CLOCKSOURCE_SMCLK,   // Tie Timer A to SMCLK(clock signal)
    //TIMER_A_CLOCKSOURCE_DIVIDER_64, //Increment counter every 64 clock cycles, NB: Clock Signal Freq = Oscillator freq/ divider.
   timerA_divider,
    //46875, //Period of Timer A (this value is placed in TAxCCR0),calculated from eqxn 1 below
   timerA_period,
   TIMER_A_TAIE_INTERRUPT_DISABLE,   ///Disable Timer A rollover interrupt
   TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, //Enable Capture Compare Interrupt
   TIMER_A_DO_CLEAR                   //Clear counter upon initialization
};

//SETUP UART
const eUSCI_UART_Config UART_init=
{
     EUSCI_A_UART_CLOCKSOURCE_SMCLK, //SMCLK clock source
     3,
     4,
     2,
     ////DO NOT MESS WITH THIS CONFIGURATION///
     EUSCI_A_UART_NO_PARITY, // No parity
     EUSCI_A_UART_LSB_FIRST, // LSB first
     EUSCI_A_UART_ONE_STOP_BIT, //One stop bit
     EUSCI_A_UART_MODE,    //UART mode
     EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION, //Oversampling
};

void main (void)
{
      unsigned int dcoFrequency = 3E+6;
      CS_setDCOFrequency(dcoFrequency);
      CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
      WDT_A_holdTimer();//stop watchdog timer
      //CONFIGURE PORTS2.4 -2.7 FOR MOTOR DRIVING
      GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
      GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

       //// INITIALIZE UART MODULE 0  ///

       Interrupt_disableMaster();//Disable interrupts at NVIC level before configuration

       //Step1:   Configure pins 1.2 and 1.3 as TX and RX
       GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,GPIO_PIN2|GPIO_PIN3,GPIO_PRIMARY_MODULE_FUNCTION);

       // Step 2: initialize UART module 0
       UART_initModule(EUSCI_A0_BASE, &UART_init);
       UART_enableModule(EUSCI_A0_BASE); // Enable UART Module

       //Step3: Enable the UART receive interrupt in UART0 module itself
       UART_enableInterrupt(EUSCI_A0_BASE,EUSCI_A_UART_RECEIVE_INTERRUPT);
       Interrupt_enableInterrupt(INT_EUSCIA0);

       //Step4: enable interrupts at NVIC level
       Interrupt_enableMaster(); //No interrupt priority set prior.
       /// INITIALIZE TIMER A,
       //Set up timer A, used to print every sec
       Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig_0); //configure Timer A using above struct
       Interrupt_disableInterrupt(INT_TA0_0); //Enable Timer A interrupt,
       Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE); ///Start Timer_A
       Interrupt_disableMaster();
       //polling
       while(1){
        if (state == 0 && pcount==0)
        {
            Interrupt_disableMaster();
            for (i = 0; i < 27; i++)
            {
                while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
                UART_transmitData(EUSCI_A0_BASE, prompt[i]); // Send each character in the text string through UART
            }
            while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
            UART_transmitData(EUSCI_A0_BASE, 0x0D); //write carriage return '\r'
            while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
            UART_transmitData(EUSCI_A0_BASE, 0x0A); //write newline '\n'
            pcount =1;
            Interrupt_enableMaster();
       }
       if (state==1){
            Interrupt_disableMaster();
            if (timer <= 60)
            {
                for (i = 0; i < 7; i++)
                {
                    //display the start message
                    while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
                    UART_transmitData(EUSCI_A0_BASE, promptStart[i]); // Send each character in the text string through UART
                }
                //end the line in terminal window
                while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
                UART_transmitData(EUSCI_A0_BASE, 0x0D); //write carriage return '\r'

                while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
                UART_transmitData(EUSCI_A0_BASE, 0x0A); //write newline '\n' // turn stepper to start position
                state = 2;
                Interrupt_enableMaster(); // finished printing, allow MSP to receive data again
            }
            else
            {
                Interrupt_disableMaster();
                for (i = 0; i < 7; i++)
                {
                    while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
                    UART_transmitData(EUSCI_A0_BASE, promptError[i]); // Send each character in the text string through UART
                }
                //end the line in terminal window
                while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
                UART_transmitData(EUSCI_A0_BASE, 0x0D); //write carriage return '\r'

                while ((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){};
                UART_transmitData(EUSCI_A0_BASE, 0x0A); //write newline '\n' // turn stepper to start position
            }
     }

  }
}


 ///UART INTERRUPT: For receiving commands from Serial monitor
void EUSCIA0_IRQHandler(void)
{
//read data from rx buffer
    receivedBuffer[0] = EUSCI_A_UART_receiveData(EUSCI_A0_BASE); // NB: reading RX buffer resets the received interrupt flag

//if the start position has been set & return key is pressed*/
    if ((state == 2) && (receivedBuffer[0] == 0x0D))
    {
        receivedBuffer[0] == 0x00; //works as debouncing to not cause the same interrupt handler to repeat different parts when the buffer is the return key
        dir = 1;//go counter clockwise
        Interrupt_enableInterrupt(INT_TA0_0);
    }
//if we are not in the set position, wait for a number to be entered
    else if (state == 0)
    {
//if the buffer is not the carriage return
        if (receivedBuffer[0] != 0x0D)
        {
            promptText[count] = receivedBuffer[0]; //save consecutive keystrokes
            count = count + 1;
        }

        if (receivedBuffer[0] == 0x0D)
        {
            count = 0;
            timer = atoi(promptText); //once return key pressed, save string into a new variable as an integer
            //and convert the time to number of steps for the motor
            dir = 0;//go clockwise
            receivedBuffer[0] = 0x00;
            //works as debouncer to not cause the same interrupt handler
            printf("data acquired \n");
            state =1;//next state triggered
            Interrupt_enableInterrupt(INT_TA0_0);
        }
    }
}

 //TIMER A INTERRUPT : For stepper motor control/ timing motor phase transmission
void TA0_0_IRQHandler()
{
    //clear timer A0 interrupt
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);  //
    //the value has been entered and the stepper is ready to move CCW
    if (dir == 0)
    {   //set timer to move at right frequency
        phasecount = forwardStep(phasecount, GPIO_PORT_P2);     // move CCW
        step = timer * (4096 / 60);
        TA0CCR0 = 200;
        counter = counter + 1;
        //if the counter has reached the value of steps entered
        if (counter1 == step)
        {   //stop and prep for next state
            dir = 1;
            printf("Setting motor to start position \n");
            counter = 0;
            Interrupt_disableInterrupt(INT_TA0_0);
        }
    }
//repeat for moving in opposite direction
// if the value has been entered and the stepper is ready to move CW
    if (dir == 1)
    {
        //set timer to move at right frequency
        phasecount = backwardStep(phasecount, GPIO_PORT_P2); // move CW
        step = timer * (4096 / 60);
        TA0CCR0 = 46875 * 60 / 4096;
        counter = counter + 1;
        //if the counter has reached the value of steps entered
        if (counter == step)
        {   //stop and prep for next state
            dir = 0;
            state = 0;
            pcount=0;
            printf("Moving motor forward \n");
            counter = 0;
            //Interrupt_disableInterrupt(INT_EUSCIA0);
            Interrupt_disableInterrupt(INT_TA0_0);

        }
    }

}
