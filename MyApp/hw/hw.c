#include "driver/button.h"
#include "driver/cli.h"
#include "driver/temp.h"
#include "led.h"
#include "uart.h"

void hwInit(void){
    ledInit();
    uartInit();
    cliInit();
    buttonInit();
    tempInit();
}

