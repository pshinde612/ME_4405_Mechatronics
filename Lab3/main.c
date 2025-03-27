#include "driverlib.h"//driver library
#include "header3.h"//lab 3 header file
#include "msp.h"//msp header file

//Global variables
int i =0;//variable for for loop delay to counter switch debouncing
int LED_STATE =0;//Variable that changes the color state of the second LED
int count = 0;//Variable to store state of second LED

//helper function to change the LED color state
int getState(LED_STATE){
    if(LED_STATE == 0){
        return 1;
    } else if(LED_STATE ==1){
        return 2;
    } else{
        return 0;
    }
}
//main function
int main(void)
{
    unsigned short usibutton1 = 0;
    unsigned short usibutton2 = 0;
    //stop watchdog timer
    WDT_A_holdTimer();
    //set LED's as output pins and set initial state to off
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, LED_RED);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, LED_RED);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, LED2);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
    //set push button input as a pull up resistor
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN4);

    //infinite loop to constantly run the program or polling
    while(1)
    {
        //read the input values from the pushbuttons
        usibutton1 = MAP_GPIO_getInputPinValue(GPIO_PORT_P1,GPIO_PIN1);
        usibutton2 = MAP_GPIO_getInputPinValue(GPIO_PORT_P1,GPIO_PIN4);
        //LED 1 control
        if(usibutton1 == GPIO_INPUT_PIN_LOW){
            for(i=0;i<=2000;i++){}//for loop to prevent bouncing
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);//set LED high on button press
        }
        else{
            for(i=0;i<=2000;i++){}
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);//set LED high on button release
        }
        //LED 2 control(RGB switching)
        if(usibutton2 == GPIO_INPUT_PIN_LOW){
            count =1;//store state of button press to change color mode
            for(i=0;i<=2000;i++){}
            //turn on specific LED color based on state
            switch(LED_STATE){
                case 0:
                    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
                    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
                    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);//RED on
                    break;
                case 1:
                    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
                    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
                    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);//GREEN on
                    break;
                default:
                    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
                    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
                    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);//BLUE on
            }

        }
        else{
            for(i=0;i<=1000;i++){}
            //change state depending on how many times the button has benn pressed
            if(count ==1){
                count =0;
                LED_STATE = getState(LED_STATE);
            }
            //turn off LED 2 all pins on button release
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
        }
    }
}
