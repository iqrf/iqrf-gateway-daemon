/*
 * ccp_leds_cmd.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <assert.h>
#include "json.h"

#include "utility.h"
#include "Console.h"
#include "DPA.h"

void ccp_leds_cmd (__uint16_t CommandParameter)
{
    T_DPA_PACKET MyDpaPacket;
    int Message = 0;
    int ParameterSize;
    char ParameterString[64];

    json_object *IqrfRqMsg = NULL;
    json_object *IqrfRsMsg = NULL;

    // processing of command input parameters
    // read required operation
    ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);

    if (ParameterSize) {
        if (strcmp("on", ParameterString) == 0)
            MyDpaPacket.PCMD = CMD_LED_SET_ON;
        else if (strcmp("off", ParameterString) == 0)
            MyDpaPacket.PCMD = CMD_LED_SET_OFF;
        else if (strcmp("flash", ParameterString) == 0)
        	MyDpaPacket.PCMD = CMD_LED_PULSE; // PCMD = CMD_LED_FLASHING;
        else if (strcmp("pulse", ParameterString) == 0)
            MyDpaPacket.PCMD = CMD_LED_PULSE;
        else Message = CCP_BAD_PARAMETER;
    } else {
        Message = CCP_BAD_PARAMETER;
    }

    // read destination address
    ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);
    if (ParameterSize)
        MyDpaPacket.NADR = atoi(ParameterString);           // set destination address
    else
        Message = CCP_BAD_PARAMETER;                        // if address not exist, print error MSG

    // sending DPA request and processing response
    if (Message) {
        sys_msg_printer(Message);                           // command parameters ERROR
    } else {
        MyDpaPacket.PNUM = CommandParameter;                // select RED or GREEN led
        MyDpaPacket.HWPID = HWPID_DoNotCheck;               // do not check HWPID
        IqrfRqMsg = create_json_msg(&MyDpaPacket, 0, 2000);
        IqrfRsMsg = send_json_msg(IqrfRqMsg);

        if (IqrfRqMsg != NULL)                              // destroy request JSON object, if exist
            json_object_put(IqrfRqMsg);
        if (IqrfRsMsg != NULL)                              // destroy response JSON object, if exist
            json_object_put(IqrfRsMsg);
    }
}
