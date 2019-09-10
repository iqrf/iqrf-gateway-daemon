/*
 * ccp_node_cmd.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <assert.h>
#include "json.h"

#include "utility.h"
#include "Console.h"

void ccp_node_cmd (__uint16_t CommandParameter)
{
    T_DPA_PACKET MyDpaPacket;
    int ParameterSize;
    char ParameterString[64];

    json_object *IqrfRqMsg = NULL;
    json_object *IqrfRsMsg = NULL;

    // Send Batch command
    MyDpaPacket.NADR = 0;
    MyDpaPacket.PNUM = PNUM_OS;
    MyDpaPacket.PCMD = CMD_OS_BATCH;
    MyDpaPacket.HWPID = HWPID_DoNotCheck;

    // Remove bond
    MyDpaPacket.DpaMessage.Request.PData[0] = 5;
    MyDpaPacket.DpaMessage.Request.PData[1] = PNUM_NODE;
    MyDpaPacket.DpaMessage.Request.PData[2] = CMD_NODE_REMOVE_BOND;
    MyDpaPacket.DpaMessage.Request.PData[3] = HWPID_DoNotCheck & 0xff;
    MyDpaPacket.DpaMessage.Request.PData[4] = HWPID_DoNotCheck >> 0x08;
    // Restart
    MyDpaPacket.DpaMessage.Request.PData[5] = 5;
    MyDpaPacket.DpaMessage.Request.PData[6] = PNUM_OS;
    MyDpaPacket.DpaMessage.Request.PData[7] = CMD_OS_RESTART;
    MyDpaPacket.DpaMessage.Request.PData[8] = HWPID_DoNotCheck & 0xff;
    MyDpaPacket.DpaMessage.Request.PData[9] = HWPID_DoNotCheck >> 0x08;
    // EndBatch
    MyDpaPacket.DpaMessage.Request.PData[10] = 0;

    // read destination address
    ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);
    if (ParameterSize)                                       // if exist
        MyDpaPacket.NADR = atoi(ParameterString);            // set destination address

    if (MyDpaPacket.NADR != 0 && MyDpaPacket.NADR <= MAX_ADDRESS) { // parameter OK
        IqrfRqMsg = create_json_msg(&MyDpaPacket, 11, 50000);
        IqrfRsMsg = send_json_msg(IqrfRqMsg);
    } else {
        sys_msg_printer(CCP_BAD_PARAMETER);                  // bad parameter
    }

    if (IqrfRqMsg != NULL)                                  // destroy request JSON object, if exist
        json_object_put(IqrfRqMsg);
    if (IqrfRsMsg != NULL)                                  // destroy response JSON object, if exist
        json_object_put(IqrfRsMsg);
}
