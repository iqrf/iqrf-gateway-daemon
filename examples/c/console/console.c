/*
 * Console.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "ccp_commands.h"
#include "utility.h"
#include "Console.h"

#include "DPA.h"

/* data types */
typedef struct {                        // command decode table item structure
    char  Com[16];
    void  (*Func)(__uint16_t);
    __uint16_t  Param;
} COM;

/* function prototypes */
void ccp_find_command(char *CommandString);
int ccp_read_substring(int *StartPos, char *DestinationString, char *SourceString);
void ccp_exit_cmd(__uint16_t CommandParameter);
void sys_msg_printer(__uint16_t CommandParameter);
int chkin_select(void);

/* global variables */
const COM Commands[] = {        // command decode table
    {"bond", ccp_coordinator_cmd, 0},
    {"unbondc", ccp_coordinator_cmd, CMD_COORDINATOR_REMOVE_BOND},
    {"discovery", ccp_coordinator_cmd, CMD_COORDINATOR_DISCOVERY},
    {"ledr", ccp_leds_cmd, PNUM_LEDR},
    {"ledg", ccp_leds_cmd, PNUM_LEDG},
    {"unbondn", ccp_node_cmd, 0},
    {"osreset", ccp_os_cmd, CMD_OS_RESET},
    {"osrestart", ccp_os_cmd, CMD_OS_RESTART},
    {"osinfo", ccp_os_cmd, CMD_OS_READ},
    {"exit", ccp_exit_cmd, 0},
};

#define NUM_OF_SYSTEM_MSG   17
#define SIZE_OF_SYSTEM_MSG  26

const char SystemMsg[NUM_OF_SYSTEM_MSG][SIZE_OF_SYSTEM_MSG] = {       // error messages
    "Command not found        ",   /* 0 */
    "Done !!!                 ",   /* 1 */
    "SD card operation ERROR  ",   /* 2 */
    "Uploading ....           ",   /* 3 */
    "Bad command parameter    ",   /* 4 */
    "File not found           ",   /* 5 */
    "Directory not found      ",   /* 6 */
    "Sending request          ",   /* 7 */
    "Confirmation OK          ",   /* 8 */
    "Confirmation ERROR       ",   /* 9 */
    "Response OK              ",   /* 10 */
    "Response ERROR           ",   /* 11 */
    "Command timeout          ",   /* 12 */
    "Code stored successfully ",   /* 13 */
    "Code store ERROR         ",   /* 14 */
    "TR module not ready      ",   /* 15 */
    "Configuration file ERROR ",   /* 16 */
};

char InLine[BUFF];
int InLinePosition;
void (*run_func)(__uint16_t);                   // pointer to command service function
__uint16_t Parameter;                           // command service function parameter
int ExitCmd;

int main(int argc, char *argv[])
{
    char MyString[BUFF];
    int SubstringSize;
    int ErrFlag = 0;

    if ( argc != 7 ) {/* argc should be 7 for correct execution */
         printf( "\nWrong parameters\n");
         printf( "Use: ./Console -h MqttHostIp:port -pt MqttPublishTopic -st MqttSubscribeTopic\n\n");
         return(-1);
    }

    if (strcmp(argv[1], "-h") == 0 && strlen(argv[2]) < sizeof(HostIp)) {
         strcpy(HostIp, argv[2]);
    } else {
        ErrFlag = 1;
    }

    if (strcmp(argv[3], "-pt") == 0 && strlen(argv[4]) < sizeof(PTopic)) {
         strcpy(PTopic, argv[4]);
    } else {
        ErrFlag = 1;
    }

    if (strcmp(argv[5], "-st") == 0 && strlen(argv[6]) < sizeof(STopic)) {
         strcpy(STopic, argv[6]);
    } else {
        ErrFlag = 1;
    }

    if (ErrFlag == 1) {
         printf( "Wrong parameter.\n\r");
         return(-2);
    }

    ExitCmd = 0;

    while(1)
    {
        printf("cmd> ");
        fflush(stdout);

        while(!chkin_select()) {
            usleep(100000);
        }
        fgets(InLine, BUFF, stdin);

        InLinePosition = 0;
        SubstringSize = ccp_read_substring(&InLinePosition, MyString, InLine);
        if (SubstringSize != 0) {
            ccp_find_command(MyString);
            run_func(Parameter);
            if (ExitCmd)
                break;
        } else {
            continue;
        }
    }

    return EXIT_SUCCESS;
}


/**
 * find command in command table
 *   - set pointer to command service function
 *   - initialize parameter of command service function
 */

void ccp_find_command(char *CommandString)
{
    __uint8_t CSel;

    for (CSel=0; CSel<(sizeof(Commands)/sizeof(COM)); CSel++) {   // compare input buffer with existing command patterns
        if (strcmp(Commands[CSel].Com, CommandString) == 0) {
            run_func = Commands[CSel].Func;                      // initialize pointer to function to run
            Parameter = Commands[CSel].Param;                    // initialize function parameter
            return;
        }
    }

    run_func = sys_msg_printer;                                     // in case of error, run error service function
    Parameter = CCP_COMMAND_NOT_FOUND;
}

/**
 * find string in SourceSting buffer from StartPos position
 * @param StartPos pointer to variable defining starting position to search substring in SourceString buffer
 * @param DestinationString pointer to string where substring going to be placed
 * @param SourceString pointer to string where substring is looking for
 * @return size of DestinationString string / 0 = no substring found
 */
int ccp_read_substring(int *StartPos, char *DestinationString, char *SourceString)
{
    int  TempCnt = 0;
    char  TempChar;

    InLine[BUFF-1] = 0;

    do {
        TempChar = SourceString[*StartPos];
        if (TempChar == ' ') (*StartPos)++;
    } while (TempChar == ' ');

    if (TempChar == 0) {
        DestinationString[0] = 0;
        return(0);
    }

    do {
        TempChar = SourceString[(*StartPos)++];
        DestinationString[TempCnt++] = TempChar;
    } while(TempChar!=' ' && TempChar!=0x0A && TempChar!=0 && *StartPos<BUFF && TempCnt<BUFF);

    DestinationString[--TempCnt] = 0;
    (*StartPos)--;
    return(TempCnt);
}

/**
 * exit console
 * @param CommandParameter
 * @return none
 */
void ccp_exit_cmd(__uint16_t CommandParameter)
{
    puts("Bye ...");
    ExitCmd = 1;
}

/**
 * print selected system MSG
 * @param CommandParameter - number of system MSG
 * @return none
 */
void sys_msg_printer(__uint16_t CommandParameter)
{
    if (CommandParameter < NUM_OF_SYSTEM_MSG) {
        puts(&SystemMsg[CommandParameter][0]);
    }
}

/*
   select() and pselect() allow a program to monitor multiple file
   descriptors, waiting until one or more of the file descriptors become
   "ready" for some class of I/O operation (e.g., input possible).  A
   file descriptor is considered ready if it is possible to perform a
   corresponding I/O operation (e.g., read(2) without blocking, or a
   sufficiently small write(2)).
 */
int chkin_select(void)
{
    fd_set rd;
    struct timeval tv={0};
    int ret;

    FD_ZERO(&rd);
    FD_SET(STDIN_FILENO, &rd);
    ret=select(1, &rd, NULL, NULL, &tv);

    return (ret>0);
}
