#include "ap.h"

//button enable/disable = on/off
void cliButton(uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        if (strcmp(argv[1], "on") == 0)
        {
            buttonEnable(true); // 여기서 기능을 켭니다!
            cliPrintf("Button Interrupt Report: ON\r\n");
        }
        else if (strcmp(argv[1], "off") == 0)
        {
            buttonEnable(false); // 여기서 기능을 끕니다!
            cliPrintf("Button Interrupt Report: OFF\r\n");
        }
        else
        {
            cliPrintf("Usage: button [on|off]\r\n");
        }
    }
    else
    {
        // 현재 상태(ON/OFF)를 보여줍니다.
        cliPrintf("Button Enable : %s\r\n", buttonGetEnable() ? "ON" : "OFF");
        cliPrintf("Usage: button [on|off]\r\n");
    }
}


static bool isSafeAddress(uint32_t addr){
    // 1. f411 flash
    if (0x08000000 <= addr && addr <= 0x0807FFFF) return true;

    // 2. f411 ram
    if (0x20000000 <= addr && addr <= 0x2000FFFF) return true;

    // 3. system memory
    if(0x1FFF0000 <= addr && addr <= 0x1FFF7A1F) return true;

    // 4. Peripheral register
    if(0x40000000 <= addr && addr <= 0x5FFFFFFF) return true;

    return false;

}

// md 0x8000 - 0000 32
void cliMd(uint8_t argc, char **argv){
    if(argc >= 2){
        uint32_t addr = strtoul (argv[1], NULL, 16);
        uint32_t length = 16;

        if(argc >= 3){
            length = strtoul (argv[2], NULL, 0);
        }
        for (uint32_t i = 0; i < length; i+=16){

            cliPrintf("0x%08x: ", addr + i);
            for (uint32_t j = 0; j < 16; j++){

                if(i+j < length){

                    uint32_t target_addr = addr + i + j;
                    if (isSafeAddress(target_addr)) {

                        uint8_t val = *((volatile uint8_t *)target_addr);
                        cliPrintf("%02X ", val);
                    }
                    else {
                        cliPrintf("Not Valid Adress!!\r\n");
                        break;
                    }
                }
                else{
                    cliPrintf("   ");
                }
            }
            cliPrintf(" | ");
            for (uint32_t j = 0; j < 16; j++){
                if (i + j < length){
                    uint32_t target_addr = addr + i + j;
                    if (isSafeAddress(target_addr)){
                        uint8_t val = *((volatile uint8_t *)target_addr);
                        if (val >= 0x20 && val <= 0x7E){
                            cliPrintf("%c", val);
                        }
                        else{
                            cliPrintf(".");
                        }
                    }
                    else{
                        cliPrintf("Not Valid Adress!!\r\n");
                        break;
                    }
                }
                else{
                    cliPrintf(" ");
                }
            }

            cliPrintf("\r\n");
        }
    }
    else{
        cliPrintf("Usage: md [addr(hex)] [length]\r\n");
        cliPrintf("       md 08000000 32\r\n");
    }
}

// argv[1] : "read "write"
// argv[2] : pin A5, B12
void cliGpio(uint8_t argc, char **argv){
    if(argc >= 3){
        char port_char = tolower(argv[2][0]);
        int pin_num = atoi(&argv[2][1]);

        uint8_t port_idx = port_char - 'a';

        if(strcmp(argv[1], "read") == 0){
            int8_t state = gpioExtRead(port_idx, pin_num);
            if(state < 0){
                cliPrintf("Invalid Port or Pin(ex: a5, b12)\r\n");
            }
            else{
                cliPrintf("GPIO %c%d = %d\r\n", toupper(port_char), pin_num, state);
            }
        }
        else if(strcmp(argv[1], "write") == 0 && argc == 4){
            int val = atoi(argv[3]);
            if(gpioExtWrite(port_idx, pin_num, val) == true){
                cliPrintf("GPIO %c%d = %d\r\n", toupper(port_char), pin_num, val);
            }
            else{
                cliPrintf("Invaild Port or Pin(ex: a5, b12)\r\n");
            }
        }
        else {
            cliPrintf("Usage: gpio read[a~h][0~15]\r\n");
            cliPrintf("       gpio write[a~h][0~15] [0|1]\r\n");
        }
    }

    else{
        cliPrintf("Usage: gpio read[a~h][0~15]\r\n");
        cliPrintf("       gpio write[a~h][0~15] [0|1]\r\n");
    }
}


static uint32_t led_toggle_period = 0;

void cliled(uint8_t argc, char **argv)
{
    if(argc >= 2){
        if(strcmp(argv[1], "on") == 0){
            led_toggle_period=0;
            ledOn();
            cliPrintf("LED ON\r\n");
        }
        else if(strcmp(argv[1], "off") == 0){
            led_toggle_period=0;
            ledOff();
            cliPrintf("LED OFF\r\n");
        }
        else if(strcmp(argv[1], "toggle") == 0){
            if(argc==3){
                led_toggle_period=atoi(argv[2]);
                if(led_toggle_period>0){
                    cliPrintf("LED Auto-Toggle!!\r\n");

                }else {
                    cliPrintf("Invaild Period\r\n");
                }
            }
            else{
                led_toggle_period=0;
                ledToggle();
                cliPrintf("LED TOGGLE\r\n");
            }
        }
        else{
            cliPrintf("Invaild Command\r\n");
        }
    }
    else{
        cliPrintf("Usage: led[on|off]\r\n");
        cliPrintf("     : led toggle\r\n");
        cliPrintf("     : led toggle [period]\r\n");
    }
}


void cliInfo(uint8_t argc, char **argv) {

    if (argc == 1){
        cliPrintf("==============================\r\n");
        cliPrintf("  HW Model   :  STM32F411\r\n");
        cliPrintf("  FW Version :  V1.0.0\r\n");
        cliPrintf("  Build Date :  %s %s\r\n", __DATE__, __TIME__);
        
        uint32_t uid0 = HAL_GetUIDw0();
        uint32_t uid1 = HAL_GetUIDw1();
        uint32_t uid2 = HAL_GetUIDw2();
        uint32_t dev = HAL_GetDEVID();

        // uint32_t hal = HAL_GetHalVersion();
        // uint32_t rev = HAL_GetREVID();

        cliPrintf("  Serial Num :  %08x-%08x-%08x\r\n", uid0, uid1, uid2);
        cliPrintf("  DevicID    :  %08x\r\n", dev);
        // cliPrintf("  Serial Num :  %08x-%08x-%08x\r\n");
        cliPrintf("==============================\r\n");
    }

    if ((argc == 2) || strcmp(argv[1], "uptime") == 0) {
        cliPrintf("System Uptime: %d ms \r\n", millis());
    }
    else {
        cliPrintf("Usage: info[uptime]\r\n");
        cliPrintf("info[uptime]\r\n");
    }
}

void cliSys(uint8_t argc, char **argv)
{
    if(argc == 2 && strcmp(argv[1], "reset") == 0){
        NVIC_SystemReset();
    }
    else {
        cliPrintf("Usage: sys[reset]\r\n");
    }
}

static uint32_t temp_read_period = 0;
void cliTemp(uint8_t argc, char **argv)
{
    if(argc == 1){
        temp_read_period=0;
        if(temp_read_period > 0){
            tempStopAuto();
        }
        temp_read_period=0;
        float t=tempReadAuto();
        cliPrintf("Current Temp: %.2f *C\r\n", t);

    }else if (argc == 2){
        
        int period = atoi(argv[1]);
        if(period > 0){
            tempStartAuto();
            temp_read_period=period;
            cliPrintf("Temperature Auto-Read Started %d ms\r\n", period);
        }else{
            tempStopAuto();
            cliPrintf("Invaild Period");
            }
    }else{
        tempStopAuto();
        cliPrintf("Usage: temp\r\n");
        cliPrintf("       temp [period]\r\n");
    }
}


void ledSystemTask(void *argument)
{
    while(1)
    {
        if (led_toggle_period > 0)
        {
            ledToggle();                // LED 상태 반전
            osDelay(led_toggle_period); // 설정된 주기만큼 대기
        }
        else
        {
            osDelay(50); 
        }
    }
}

void StartDefaultTask(void *argument)
{
    apInit();
    for(;;)
    {
        apMain();
    }
}   

void tempSystemTask(void *argument)
{
    while(1){
        if(temp_read_period > 0){
            float t=tempReadAuto();
            cliPrintf("Current Temp: %.2f *C\r\n", t);
            osDelay(temp_read_period);
        
        }else{
            osDelay(50);
        }
    }
}

void apInit(void){
    hwInit();
    cliAdd("led", cliled);
    cliAdd("info", cliInfo);
    cliAdd("sys", cliSys);
    cliAdd("gpio", cliGpio);
    cliAdd("md", cliMd);
    cliAdd("button", cliButton);
    cliAdd("temp", cliTemp);
}

void apMain(void)
{
    uartPrintf(0,"LED Task Started!\r\n");

    while(1){

        cliMain();
        osDelay(1);
    }
}