#include <stdio.h>
#include <msp.h>
#include <driverlib.h>

// Serial Communication variables
volatile char receivedBuffer[200]; // String used to store ASCII characters sent over UART to other MSP
volatile char receivedBuffer1[200];// String used to store ASCII characters received over UART to Putty
volatile uint8_t printPrompt = 0;     // Flag for printing text over UART
volatile uint8_t printPrompt1 = 0;     // Flag for printing text over UART


// Set Baud Rate = 57600
/* Calculations:
 * N = 3,000,000/57600 = 52.0833
 * DIV = 52.0833/16 = 3.255
 * INT(DIV) = 3
 * FirstModReg = 0.255*16 = 4.08 INT(FirstModReg) = 4
 * SecondModReg = 2 from Datasheet
 * Oversampling mode used */

// Create structure for UART communication
const eUSCI_UART_Config UART_init =
    {
     EUSCI_A_UART_CLOCKSOURCE_SMCLK,
     3,
     4,
     2,
     EUSCI_A_UART_NO_PARITY,
     EUSCI_A_UART_LSB_FIRST,
     EUSCI_A_UART_ONE_STOP_BIT,
     EUSCI_A_UART_MODE,
     EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
    };

int i = 0; // Global variable for ASCII character counting
int index = 0;//indexing receivedBuffer
int index1 = 0;//indexing receivedBuffer1

// Start program
void main(void)
{
    WDT_A_holdTimer();              // Stop watchdog timer
    CS_setDCOFrequency(3E+6);       // Set clock frequency to 3MHz
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Disable Interrupts at the NVIC level before configuration
    Interrupt_disableMaster();

    // Configure P1.2 and P1.3 as UART TX/RX
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    // Configure P2.2 and P2.3 as UART TX/RX
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    // Initialize UART Module 0
    UART_initModule(EUSCI_A0_BASE, &UART_init);
    UART_enableModule(EUSCI_A0_BASE);
    UART_initModule(EUSCI_A1_BASE, &UART_init);
    UART_enableModule(EUSCI_A1_BASE);

    // Enable the UART receive interrupt at the peripheral level
    UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    Interrupt_enableInterrupt(INT_EUSCIA0);
    Interrupt_enableInterrupt(INT_EUSCIA1);

    // Enable interrupts at the NVIC level
    Interrupt_enableMaster();  // No priority was set because this is the only interrupt in this program

    while(1){
        // If the Return Key was pressed
        if(printPrompt){
            Interrupt_disableMaster() ;     // Turn off interrupts so we can print - i.e., stop receiving data
            // Cycle through each character in the print string
            for(i = 0; i < index; i++){
                while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;                  // (UCA0IFG & 0x02) == 0 will evaluate to true when TXIFG bit is set, false when TXIFG bit is clear
                UART_transmitData(EUSCI_A1_BASE, receivedBuffer[i]) ;  // Send each character in the text string through UART
            }
            // End the line in terminal window
            while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;
            UART_transmitData(EUSCI_A1_BASE, 0x0D) ;      // write carriage return '\r'

            while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;
            UART_transmitData(EUSCI_A1_BASE, 0x0A) ;      // write newline '\n'

            memset(receivedBuffer,0,200); //clear data in buffer
            index=0;
            printPrompt = 0;                // Set print flag back to 0
            Interrupt_enableMaster() ;      // Finished printing - allow the MSP to receive data again
        }
        if(printPrompt1){
            Interrupt_disableMaster() ;     // Turn off interrupts so we can print - i.e., stop receiving data
            for(i = 0; i < index1; i++){
                   while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;                  // (UCA0IFG & 0x02) == 0 will evaluate to true when TXIFG bit is set, false when TXIFG bit is clear
                   UART_transmitData(EUSCI_A0_BASE, receivedBuffer1[i]) ;  // Send each character in the text string through UART
            }
            while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;
            UART_transmitData(EUSCI_A0_BASE, 0x0D) ;

            while((EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG & 0x02) == 0){} ;
            UART_transmitData(EUSCI_A0_BASE, 0x0A) ;

            memset(receivedBuffer1,0,200);//clear data in buffer
            index1=0;
            printPrompt1= 0;// Set print flag back to 0
            Interrupt_enableMaster() ;// Finished printing - allow the MSP to receive data again
        }
    }
}

// ISR for UART Module 0
void EUSCIA0_IRQHandler(void){
    // Read the data from the RX buffer
    char tmp = EUSCI_A_UART_receiveData(EUSCI_A0_BASE); // reading the RX buffer and storing the data
    receivedBuffer[index] = tmp;
    index = index +1;

    if(tmp== 0x0D){
        printPrompt=1;  // Set the text print flag high if carriage return pressed
    }
}
// ISR for UART Module 1
void EUSCIA1_IRQHandler(void){
    // Read the data from the RX buffer
    char tmp = EUSCI_A_UART_receiveData(EUSCI_A1_BASE); // reading the RX buffer and storing the data
    receivedBuffer1[index1] = tmp;
    index1 = index1 +1;
    printf("%c\n\r",tmp);

    if(tmp== 0x0D){
        printPrompt1=1; // Set the text print flag high if carriage return pressed
    }
}
