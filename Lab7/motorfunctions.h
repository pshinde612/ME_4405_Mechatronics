/*
 * motorfunctions.h
 *
 *  Created on: Mar 31, 2023
 *      Author: parth
 */

#ifndef MOTORFUNCTIONS_H_
#define MOTORFUNCTIONS_H_

// "backwardStep" counts motor phases in reverse sequence
int backwardStep(int phasecount, uint8_t port){
    if(phasecount==0){
        GPIO_setOutputHighOnPin(port,GPIO_PIN4|GPIO_PIN5);
        GPIO_setOutputLowOnPin(port,GPIO_PIN6|GPIO_PIN7);
    }
    else if(phasecount==1){
        GPIO_setOutputHighOnPin(port,GPIO_PIN5);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN7|GPIO_PIN6);
    }
    else if(phasecount==2){
        GPIO_setOutputHighOnPin(port,GPIO_PIN5|GPIO_PIN6);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN7);
    }
    else if(phasecount==3){
        GPIO_setOutputHighOnPin(port,GPIO_PIN6);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN5|GPIO_PIN7);
    }
    else if(phasecount==4){
        GPIO_setOutputHighOnPin(port,GPIO_PIN7|GPIO_PIN6);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN5);
    }
    else if(phasecount==5){
        GPIO_setOutputHighOnPin(port,GPIO_PIN7);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN5|GPIO_PIN6);
    }
    else if(phasecount==6){
        GPIO_setOutputHighOnPin(port,GPIO_PIN4|GPIO_PIN7);
        GPIO_setOutputLowOnPin(port,GPIO_PIN6|GPIO_PIN5);
    }
    else if(phasecount==7){
        GPIO_setOutputHighOnPin(port,GPIO_PIN4);
        GPIO_setOutputLowOnPin(port,GPIO_PIN6|GPIO_PIN5|GPIO_PIN7);
    }
    if(phasecount==7){
        phasecount = 0;
    }
    else{
        phasecount++;
    }
    return phasecount;
}



// "forwardStep" counts motor phases in normal sequence
int forwardStep(int phasecount, uint8_t port){
    if(phasecount==0){
        GPIO_setOutputHighOnPin(port,GPIO_PIN4|GPIO_PIN5);
        GPIO_setOutputLowOnPin(port,GPIO_PIN6|GPIO_PIN7);
    }
    else if(phasecount==1){
        GPIO_setOutputHighOnPin(port,GPIO_PIN5);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN7|GPIO_PIN6);
    }
    else if(phasecount==2){
        GPIO_setOutputHighOnPin(port,GPIO_PIN5|GPIO_PIN6);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN7);
    }
    else if(phasecount==3){
        GPIO_setOutputHighOnPin(port,GPIO_PIN6);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN5|GPIO_PIN7);
    }
    else if(phasecount==4){
        GPIO_setOutputHighOnPin(port,GPIO_PIN7|GPIO_PIN6);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN5);
    }
    else if(phasecount==5){
        GPIO_setOutputHighOnPin(port,GPIO_PIN7);
        GPIO_setOutputLowOnPin(port,GPIO_PIN4|GPIO_PIN5|GPIO_PIN6);
    }
    else if(phasecount==6){
        GPIO_setOutputHighOnPin(port,GPIO_PIN4|GPIO_PIN7);
        GPIO_setOutputLowOnPin(port,GPIO_PIN6|GPIO_PIN5);
    }
    else if(phasecount==7){
        GPIO_setOutputHighOnPin(port,GPIO_PIN4);
        GPIO_setOutputLowOnPin(port,GPIO_PIN6|GPIO_PIN5|GPIO_PIN7);
    }

    if(phasecount==0){
        phasecount = 7;
    }
    else{
        phasecount--;
    }
    return phasecount;
}
#endif /* MOTORFUNCTIONS_H_ */
