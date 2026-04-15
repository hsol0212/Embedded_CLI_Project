#ifndef __HW_DRIVER_CLI_H_    
#define __HW_DRIVER_CLI_H_

#include "hw_def.h"

void cliInit();

void cliMain();
void cliPrintf(const char *fmt, ...);

void cliParseArgs(char* line_buf);
bool cliAdd(const char* cmd_str, void (*cmd_func)(uint8_t argc, char** argv));
void cliRunCommand();

#endif // __HW_DRIVER_UART_H_