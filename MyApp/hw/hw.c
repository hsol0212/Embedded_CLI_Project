#include "driver/button.h"
#include "driver/cli.h"
#include "led.h"
#include "uart.h"

void hwInit(void){
    ledInit();
    uartInit();
    cliInit();
    buttonInit();
}

