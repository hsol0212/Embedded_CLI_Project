#include "cli.h"
#include "ap.h"

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

static uint8_t cli_hist_count=0;
static uint8_t cli_hist_write=0;
static uint8_t cli_hist_depth=0;

typedef enum{
    CLI_STATE_NORMAL=0,
    CLI_STATE_ESC_RCVD,
    CLI_STATE_BRACKET_RCVD
}cli_input_state_t;

static cli_input_state_t input_state=CLI_STATE_NORMAL;
static cli_callback_t ctrl_c_handler=NULL;

//Refactoring CLI function
// static void cliRedrawTail(void){

// }

static void handleEnterKey(void){
    cliPrintf("\r\n");

    cli_line_buf[cli_line_idx] = '\0';
    
    // 히스토리 버퍼에 복사
    strncpy(cli_history_buf[cli_hist_write], cli_line_buf, CLI_LINE_BUF_MAX - 1);
    cli_hist_write = (cli_hist_write + 1) % CLI_HIST_MAX;
    cli_hist_depth = 0;
    
    if (cli_hist_count < CLI_HIST_MAX) {
        cli_hist_count++;
    }

    // 명령어 실행
    cliParseArgs(cli_line_buf);
    cliRunCommand();

    cliPrintf("CLI> ");
    cli_line_idx = 0;
}

static void handleBackspace(void){
    if (cli_line_idx > 0) {
        cli_line_idx--;
        cliPrintf("\b \b"); // 커서 한 칸 뒤로 -> 공백으로 지우기 -> 다시 한 칸 뒤로
    }
}

static void handleCharInsert(uint8_t rx_data){
    if (cli_line_idx < CLI_LINE_BUF_MAX - 1) {
        cliPrintf("%c", rx_data);
        cli_line_buf[cli_line_idx++] = rx_data;
    } else {
        // 버퍼 꽉 찼을 때 처리 (여기서는 0으로 초기화하게 짜여있었네요)
        cli_line_idx = 0;
    }
}

static void handleArrowKeys(uint8_t rx_data){
    if (rx_data == 'A') { // UP 화살표
        if (cli_hist_depth < cli_hist_count) {
            cli_hist_depth++;
            // 1. 현재 화면의 글자 모두 지우기
            for (uint16_t i = 0; i < cli_line_idx; i++) cliPrintf("\b \b");

            // 2. 히스토리에서 가져오기
            int idx = (cli_hist_write + CLI_HIST_MAX - cli_hist_depth) % CLI_HIST_MAX;
            strncpy(cli_line_buf, cli_history_buf[idx], CLI_LINE_BUF_MAX - 1);
            
            cli_line_idx = strlen(cli_line_buf);
            cliPrintf("%s", cli_line_buf);

        }
    } 
    else if (rx_data == 'B') { // DOWN 화살표
        if (cli_hist_depth > 0) {
            cli_hist_depth--;
            for (uint16_t i = 0; i < cli_line_idx; i++) cliPrintf("\b \b");

            if (cli_hist_depth == 0) {
                cli_line_buf[0] = '\0';
                cli_line_idx = 0;
            } else {
                int idx = (cli_hist_write + CLI_HIST_MAX - cli_hist_depth) % CLI_HIST_MAX;
                strncpy(cli_line_buf, cli_history_buf[idx], CLI_LINE_BUF_MAX - 1);
                cliPrintf("%s", cli_line_buf);
                cli_line_idx = strlen(cli_line_buf);
            }
        }
    }
    input_state = CLI_STATE_NORMAL; // 상태 초기화
}

static void processAnsiEscape(uint8_t rx_data){
    if (input_state == CLI_STATE_ESC_RCVD) {
        if (rx_data == '[') input_state = CLI_STATE_BRACKET_RCVD;
        else input_state = CLI_STATE_NORMAL;
    } 
    else if (input_state == CLI_STATE_BRACKET_RCVD) {
        handleArrowKeys(rx_data);
        // handleArrowKeys 내부에서 input_state를 NORMAL로 바꿉니다.
    }
}

void cliMain(void)
{
    uint8_t rx_data;
    if(uartReadBlock(0, &rx_data, osWaitForever) == true)
    {
        if(input_state != CLI_STATE_NORMAL)
        {
        processAnsiEscape(rx_data);
        return;
        }
    }
    switch (rx_data) 
    {   
        case 0x03: 
            if(ctrl_c_handler!=NULL) ctrl_c_handler();
            cliPrintf("^C \r\nCLI> ");
            cli_line_idx = 0;
            break;    
        case 0x1B:  // ESC 키 입력 시
            input_state=CLI_STATE_ESC_RCVD;
            break;
        case '\r':  // 엔터 키  :   
        case '\n':  // 엔터 키  :  
            handleEnterKey();
                break;
        case '\b':  // 백스페이스 키
        case 127:   // 백스페이스 키
                handleBackspace();
                break;
        default:
        if(32 <= rx_data && rx_data <= 126)
            {
                handleCharInsert(rx_data);
                break;
            }
    }
    
}

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
    ctrl_c_handler = NULL;

    cli_cmd_count = 0;
    cli_line_idx = 0;

    cliAdd("help", cliHelp);
    cliAdd("cls", cliClear);
    cliAdd("log", cliLog);
}

void cliSetCtrlHandler(cli_callback_t handler){
    ctrl_c_handler=handler;
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

    if(is_found==false)
    {
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