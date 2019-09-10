/*
 * ccp_os_cmd.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <assert.h>
#include "json.h"

#include "utility.h"
#include "Console.h"

// MCU type of TR module
#define MCU_UNKNOWN           0
#define PIC16LF819            1     // TR-xxx-11A not supported
#define PIC16LF88             2     // TR-xxx-21A
#define PIC16F886             3     // TR-31B, TR-52B, TR-53B
#define PIC16LF1938           4     // TR-52D, TR-54D

// TR module types
#define TR_52D              0
#define TR_58D_RJ           1
#define TR_72D              2
#define TR_53D              3
#define TR_54D              8
#define TR_55D              9
#define TR_56D              10
#define TR_76D              11

void print_os_info(__uint8_t *InfoBuffer);

/**
 * Print results of OS info command
 * @param InfoBuffer buffer with TR module and OS information data
 * @return none
 */
void print_os_info(__uint8_t *InfoBuffer)
{
    __uint8_t ModuleType = InfoBuffer[5] >> 4;
    __uint8_t McuType = InfoBuffer[5] & 0x07;
    __uint8_t OsVersionMajor = InfoBuffer[4] / 16;
    __uint8_t OsVersionMinor = InfoBuffer[4] % 16;
    __uint16_t OsBuild = ((__uint16_t)InfoBuffer[7] << 8) | InfoBuffer[6];
    __uint16_t SupplyVoltage;

    __uint8_t Ptr, I;
    char TempString[24];

    // decode and print module type
    printf("\nModule type    : ");

    Ptr=0;
    if(InfoBuffer[3] & 0x80) {
        TempString[Ptr++] = 'D';
        TempString[Ptr++] = 'C';
    }
    TempString[Ptr++] = 'T';
    TempString[Ptr++] = 'R';
    TempString[Ptr++] = '-';

    TempString[Ptr++] = '5';

    switch(ModuleType) {
    case TR_52D:
        TempString[Ptr++] = '2';
        break;
    case TR_58D_RJ:
        TempString[Ptr++] = '8';
        break;
    case TR_72D:
        TempString[Ptr-1] = '7';
        TempString[Ptr++] = '2';
        break;
    case TR_53D:
        TempString[Ptr++] = '3';
        break;
    case TR_54D:
        TempString[Ptr++] = '4';
        break;
    case TR_55D:
        TempString[Ptr++] = '5';
        break;
    case TR_56D:
        TempString[Ptr++] = '6';
        break;
    case TR_76D:
        TempString[Ptr-1] = '7';
        TempString[Ptr++] = '6';
        break;
    default :
        TempString[Ptr++] = 'x';
        break;
    }

    if (McuType == PIC16LF1938)
        TempString[Ptr++] = 'D';
    TempString[Ptr++] = 'x';
    TempString[Ptr++] = 0;
    printf("%s\n", TempString);

    // print module ID
    printf("Module ID      : ");
    printf("%02X%02X%02X%02X\n", InfoBuffer[3], InfoBuffer[2], InfoBuffer[1], InfoBuffer[0]);

    // print module IBK
    printf("Module IBK     : ");
    // if OS version is 4.03 and more, print IBK
    if ((OsVersionMajor > 4) || ((OsVersionMajor == 4) && (OsVersionMinor >= 3))) {
        for (I=0; I<16; I++) {
            printf("%02X ", InfoBuffer[I+12]);
            if (I == 15)
                printf("\n");
        }
    } else {
        printf("---\n");
    }

    // print OS version string
    printf("OS version     : ");
    Ptr = 0;
    // major version
    TempString[Ptr++] = OsVersionMajor + '0';
    TempString[Ptr++] = '.';

    // minor version
    if (OsVersionMinor < 10) {
        TempString[Ptr++] = '0';
        TempString[Ptr++] = OsVersionMinor + '0';
    } else {
        TempString[Ptr++] = '1';
        TempString[Ptr++] = OsVersionMinor - 10 + '0';
    }

    if(McuType == PIC16LF1938)
        TempString[Ptr++] = 'D';

    // OS build
    TempString[Ptr++] = ' ';
    TempString[Ptr++] = '(';
    I = (OsBuild >> 12) & 0x0F;
    TempString[Ptr++] = (I > 0x09) ? I-10+'A' : I+'0';
    I = (OsBuild >> 8) & 0x0F;
    TempString[Ptr++] = (I > 0x09) ? I-10+'A' : I+'0';
    I = (OsBuild >> 4) & 0x0F;
    TempString[Ptr++] = (I > 0x09) ? I-10+'A' : I+'0';
    I = (OsBuild >> 0) & 0x0F;
    TempString[Ptr++] = (I > 0x09) ? I-10+'A' : I+'0';
    TempString[Ptr++] = ')';
    TempString[Ptr] = 0;
    printf("%s\n", TempString);

    // supply voltage
    if (ModuleType == TR_72D || ModuleType == TR_76D)
        SupplyVoltage = (26112 / (127 - InfoBuffer[9]));
    else
        SupplyVoltage = (225 + InfoBuffer[9] * 10);

    Ptr = SupplyVoltage / 100;
    I = SupplyVoltage % 100;

    // print supply voltage
    printf("Supply voltage : ");
    printf("%d.%d V\n", Ptr, I);

    // print last RSSI
    printf("Last RSSI      : ");
    printf("%d\n", InfoBuffer[8]);
}

void ccp_os_cmd (__uint16_t CommandParameter)
{
    T_DPA_PACKET MyDpaPacket;
    int ParameterSize;
    char ParameterString[64];

    json_object *IqrfRqMsg = NULL;
    json_object *IqrfRsMsg = NULL;

    MyDpaPacket.NADR = 0;
    MyDpaPacket.PNUM = PNUM_OS;                             // peripheral OS
    MyDpaPacket.PCMD = CommandParameter;                    // set peripheral command
    MyDpaPacket.HWPID = HWPID_DoNotCheck;                   // do not check HWPID

    // read first command parameter
    ParameterSize = ccp_read_substring(&InLinePosition, ParameterString, InLine);
    if (ParameterSize)                                      // if exist
        MyDpaPacket.NADR = atoi(ParameterString);           // set destination address

    IqrfRqMsg = create_json_msg(&MyDpaPacket, 0, 2000);
    IqrfRsMsg = send_json_msg(IqrfRqMsg);
    if (IqrfRsMsg != NULL) {
        decode_json_msg(&MyDpaPacket, IqrfRsMsg);
        if (MyDpaPacket.PCMD == (RESPONSE_FLAG | CMD_OS_READ))                                  // response to command CMD_OS_READ
            print_os_info((uint8_t *)&MyDpaPacket.DpaMessage.PerOSRead_Response.ModuleId[0]);   // decode received data
    }

    if (IqrfRqMsg != NULL)                                  // destroy request JSON object, if exist
       json_object_put(IqrfRqMsg);
    if (IqrfRsMsg != NULL)                                  // destroy response JSON object, if exist
        json_object_put(IqrfRsMsg);
}
