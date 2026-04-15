#include "cli.h"
#include "uart.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/_intsup.h>

#define CLI_LINE_BUF_MAX 32
#define CLI_CMD_LIST_MAX 32
#define CLI_CMD_ARG_MAX  4

typedef struct _cli_cmd_t{
    char cmd_str[16];
    void (*cmd_func)(uint8_t argc, char **argv);
}cli_cmd_t;


static cli_cmd_t cli_cmd_list[CLI_CMD_LIST_MAX];
static uint8_t cli_cmd_count=0;

static uint8_t cli_argc=0;
static char *cli_argv[CLI_CMD_ARG_MAX];
static char cli_line_buf[CLI_LINE_BUF_MAX];
static uint16_t cli_line_idx=0;

#define CLI_HIST_MAX 10
static char cli_history_buf[CLI_HIST_MAX][CLI_LINE_BUF_MAX];
static uint8_t esc_state=0;

static uint8_t cli_hist_count=0;
static uint8_t cli_hist_write=0;
static uint8_t cli_hist_depth=0;




static void cliHelp (uint8_t argc, char* argv[]){
    cliPrintf("-------------CLI Commands--------------");
    for(uint8_t i=0; i<cli_cmd_count; i++){
        cliPrintf("%s \r\n", cli_cmd_list[i].cmd_str);

    }
    cliPrintf("---------------------------------------\r\n");
}

static void cliClear (uint8_t argc, char* argv[]){

    cliPrintf("\x1B[2J\x1B[H");
}

void cliInit() 
{
  cli_cmd_count = 0;
  cli_line_idx = 0;

  cliAdd("help", cliHelp);
  cliAdd("cls", cliClear);
}

void cliPrintf(const char *fmt, ...)
{
    char buf[128];
    uint32_t len;
    va_list args;

    va_start(args,fmt);

    len=vsnprintf(buf, 128, fmt, args);

    va_end(args);
    uartWrite(0,(uint8_t*)buf,len);
}
void cliParseArgs(char* line_buf){
    char *tok;
    cli_argc=0;
    tok=strtok(line_buf, " ");
    while(tok!=NULL && cli_argc<CLI_CMD_ARG_MAX){
        cli_argv[cli_argc++]=tok;
        tok=strtok(NULL, " ");
    }
}

void cliRunCommand(){
    if(cli_argc==0)
        return;

    bool is_found=false;
    for(uint8_t i=0; i<cli_cmd_count; i++){
        if(strcmp(cli_argv[0], cli_cmd_list[i].cmd_str)==0){
            is_found=true;
            cli_cmd_list[i].cmd_func(cli_argc, cli_argv);
            break;
        }
    }

    if(is_found==false){
        cliPrintf("Command Not Found \r\n");
    }
}
bool cliAdd(const char* cmd_str, void (*cmd_func)(uint8_t argc, char** argv)){
    if(cli_cmd_count>=CLI_CMD_LIST_MAX)
        return false;

    strncpy(cli_cmd_list[cli_cmd_count].cmd_str, cmd_str, strlen(cmd_str));

    cli_cmd_list[cli_cmd_count].cmd_func=cmd_func;
    cli_cmd_count++;

    return true;

}


void cliMain() {
    if (uartAvailable(0) > 0) {
        uint8_t rx_data = uartRead(0);

        if (esc_state == 0) {
            if (rx_data == 0x1B) { // ESC 키 입력 시
                esc_state = 1;
            } 
            else {
                if ((rx_data == '\r') || (rx_data == '\n')) {
                    cliPrintf("\r\n");

                    cli_line_buf[cli_line_idx] = '\0';
                    //  실행전 히스토리 버퍼에 복사
                    strncpy(cli_history_buf[cli_hist_write], cli_line_buf, CLI_LINE_BUF_MAX-1);
                    cli_hist_write=(cli_hist_write + 1) % CLI_HIST_MAX;
                    cli_hist_depth=0;
                    if(cli_hist_count<CLI_HIST_MAX) 
                        cli_hist_count++;

                    cliParseArgs(cli_line_buf);
                    cliRunCommand();

                    cliPrintf("CLI> ");
                    cli_line_idx = 0;
                } 
                else if (rx_data == '\b' || rx_data == 127) {
                    if (cli_line_idx > 0) {
                      cli_line_idx--;
                      cliPrintf("\b \b");
                    }
                } else {
                  cliPrintf("%c", rx_data);
                  cli_line_buf[cli_line_idx++] = rx_data;
                  if (cli_line_idx >= CLI_LINE_BUF_MAX) {
                    cli_line_idx = 0;
                  }
                }
            } // end of else (esc_state == 0 내의 일반 문자 처리)
        } 
        else if (esc_state==1)
        { 
            if(rx_data=='[') esc_state=2;
            else esc_state=0;
        }
        else if (esc_state==2)
        {
            if(rx_data=='A')
            {   
                if(cli_hist_depth<cli_hist_count)
                {   
                    cli_hist_depth++;
                    for (uint16_t i = 0; i < cli_line_idx; i++) {
                     cliPrintf("\b \b");
                    }
                    int idx=(cli_hist_write-CLI_HIST_MAX-cli_hist_depth)%CLI_HIST_MAX;
                    strncpy(cli_line_buf, cli_history_buf[idx], CLI_LINE_BUF_MAX-1);
                    
                    cli_line_idx=strlen(cli_line_buf);
                    cliPrintf("%s", cli_line_buf);
                }  



            }else if(rx_data=='B')
            {
                if(cli_hist_depth>0)
                {
                    cli_hist_depth--;
                    for (uint16_t i=0; i<cli_line_idx; i++) {
                        cliPrintf("\b \b");
                    }
                   if(cli_hist_depth==0)
                    {
                        cli_line_buf[0]='\0';
                        cli_line_idx=0;
                    }
                    else { //중간 깊이일 경우
                        int idx=(cli_hist_write + CLI_HIST_MAX - cli_hist_depth)%
                                CLI_HIST_MAX;

                        strncpy(cli_line_buf, cli_history_buf[idx], CLI_LINE_BUF_MAX-1);
                        cliPrintf("%s", cli_line_buf);
                        cli_line_idx = strlen(cli_line_buf);
                    }
                }
            }
        esc_state=0;
        }
    }
}