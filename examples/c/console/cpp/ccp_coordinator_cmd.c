/*
 * ccp_coordinator_cmd.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <assert.h>
#include "json.h"

#include "utility.h"
#include "Console.h"

void print_disc_bond_info(T_DPA_PACKET *DpaPacket);

/**
 * print result of discovery get or bond get commands
 */
void print_disc_bond_info(T_DPA_PACKET *DpaPacket)
{
    __uint16_t Mask;
    __uint16_t Data;
    int X, Y;

    printf("    0 1 2 3 4 5 6 7 8 9 A B C D E F\n");

    for (X=0; X<16; X++) {
        Mask = 0x0001;
        Data = ((uint16_t)DpaPacket->DpaMessage.Response.PData[2*X+1]<<8) | DpaPacket->DpaMessage.Response.PData[2*X];
        printf("%X - ", X);
        for (Y=0; Y<16; Y++) {
            if (Data & Mask)
                printf("x ");
            else
                printf(". ");
            Mask <<= 1;
        }
        printf("\n");
    }
    printf("\n");
}

void ccp_coordinator_cmd (__uint16_t CommandParameter)
{
    T_DPA_PACKET MyDpaPacket;
    int ParameterSize;
    char ParameterString[64];

    json_object *IqrfRqMsg = NULL;
    json_object *IqrfRsMsg = NULL;

    int ReqAddr = 0;
    int TxPower = 7;
    int MaxAddr = 32;
    int BondingMask = 0xFF;
    int ErrFlag = 0;

    // DPA header
    MyDpaPacket.NADR = COORDINATOR_ADDRESS;
    MyDpaPacket.PNUM = PNUM_COORDINATOR;
    MyDpaPacket.HWPID = HWPID_DoNotCheck;
    MyDpaPacket.PCMD = 0xFF;                                       // dummy command

    switch(CommandParameter) {

    case CMD_COORDINATOR_REMOVE_BOND:
        MyDpaPacket.PCMD = CommandParameter;                       // set peripheral cmd
        ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);
        if (ParameterSize)                                         // is any command parameter ?
            ReqAddr = atoi(ParameterString);                       // read command parameter

        if (ReqAddr == 0 || ReqAddr > MAX_ADDRESS) {               // bad parameter
            sys_msg_printer(CCP_BAD_PARAMETER);                    // if bad parameter, print error message
        } else {
            MyDpaPacket.DpaMessage.PerCoordinatorRemoveRebondBond_Request.BondAddr = ReqAddr;
            IqrfRqMsg = create_json_msg(&MyDpaPacket, sizeof(TPerCoordinatorRemoveRebondBond_Request), 2000);
            IqrfRsMsg = send_json_msg(IqrfRqMsg);
            if (IqrfRsMsg != NULL) {
                decode_json_msg(&MyDpaPacket, IqrfRsMsg);
                printf("Number of bonded nodes = %d\n", MyDpaPacket.DpaMessage.PerCoordinatorRemoveRebondBond_Response.DevNr);
            }
        }
        break;

    case CMD_COORDINATOR_DISCOVERY:
        // read first command parameter
        ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);
        if (ParameterSize) {                                      // if exist
            if (strcmp("get", ParameterString) == 0) {            // GET subcommand ?
                MyDpaPacket.PCMD = CMD_COORDINATOR_DISCOVERED_DEVICES;
                IqrfRqMsg = create_json_msg(&MyDpaPacket, 0, 5000);
                IqrfRsMsg = send_json_msg(IqrfRqMsg);
                if (IqrfRsMsg != NULL) {
                    decode_json_msg(&MyDpaPacket, IqrfRsMsg);
                    print_disc_bond_info(&MyDpaPacket);
                }
            } else {                                              // TxPower & MaxAddr parameters
                TxPower = atoi(ParameterString);                  // set TxPower
                if (TxPower > 7)
                    ErrFlag = 1;                                  // check max TxPower
                // read MaxAddr
                ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);
                if (ParameterSize) {                              // if exist
                    MaxAddr = atoi(ParameterString);              // set MaxAddr
                    if (MaxAddr > 239)
                        ErrFlag = 1;                              // check MaxAddr
                }
            }
        }

        if (ErrFlag == 1) {
            sys_msg_printer(CCP_BAD_PARAMETER);                    // if bad parameter, print error message
        } else {
            if (MyDpaPacket.PCMD == 0xFF) {                        // if command not defined
                MyDpaPacket.PCMD = CMD_COORDINATOR_DISCOVERY;      // discovery command
                MyDpaPacket.DpaMessage.PerCoordinatorDiscovery_Request.TxPower = TxPower;  // Command parameters
                MyDpaPacket.DpaMessage.PerCoordinatorDiscovery_Request.MaxAddr = MaxAddr;

                IqrfRqMsg = create_json_msg(&MyDpaPacket, sizeof(TPerCoordinatorDiscovery_Request), 30000);
                IqrfRsMsg = send_json_msg(IqrfRqMsg);
                if (IqrfRsMsg != NULL) {
                    decode_json_msg(&MyDpaPacket, IqrfRsMsg);
                    printf("Number of discovered nodes = %d\n", MyDpaPacket.DpaMessage.PerCoordinatorDiscovery_Response.DiscNr);
                }
            }
        }
        break;

    default:
        // read first command parameter
        ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);
        if (ParameterSize) {                                            // if exist
            if (strcmp("clear", ParameterString) == 0) {
                MyDpaPacket.PCMD = CMD_COORDINATOR_CLEAR_ALL_BONDS;
                IqrfRqMsg = create_json_msg(&MyDpaPacket, 0, 2000);
                IqrfRsMsg = send_json_msg(IqrfRqMsg);
                if (IqrfRsMsg != NULL) {
                    printf("All nodes cleared successfully\n");
                }
            } else {
                if (strcmp("get", ParameterString) == 0) {
                    MyDpaPacket.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
                    IqrfRqMsg = create_json_msg(&MyDpaPacket, 0, 5000);
                    IqrfRsMsg = send_json_msg(IqrfRqMsg);
                    if (IqrfRsMsg != NULL) {
                        decode_json_msg(&MyDpaPacket, IqrfRsMsg);
                        print_disc_bond_info(&MyDpaPacket);
                    }
                } else {
                    ReqAddr = atoi(ParameterString);
                    // read bonding mask
                    ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);
                    if (ParameterSize)                              // if exist
                        BondingMask = atoi(ParameterString);
                }
            }
        }

        if (MyDpaPacket.PCMD == 0xFF) {                             // if command not defined
            MyDpaPacket.PCMD = CMD_COORDINATOR_BOND_NODE;
            MyDpaPacket.DpaMessage.PerCoordinatorBondNode_Request.ReqAddr = ReqAddr;   // Command parameters
            MyDpaPacket.DpaMessage.PerCoordinatorBondNode_Request.BondingMask = BondingMask;

            IqrfRqMsg = create_json_msg(&MyDpaPacket, sizeof(TPerCoordinatorBondNode_Request), 5000);
            IqrfRsMsg = send_json_msg(IqrfRqMsg);
            if (IqrfRsMsg != NULL) {
                decode_json_msg(&MyDpaPacket, IqrfRsMsg);
                printf("Bonded node address = %d\n", MyDpaPacket.DpaMessage.PerCoordinatorBondNode_Response.BondAddr);
                printf("Number of bonded nodes = %d\n", MyDpaPacket.DpaMessage.PerCoordinatorBondNode_Response.DevNr);
            }
        }
    }

    if (IqrfRqMsg != NULL)                              // destroy request JSON object, if exist
        json_object_put(IqrfRqMsg);
    if (IqrfRsMsg != NULL)							   // destroy response JSON object, if exist
        json_object_put(IqrfRsMsg);
}
