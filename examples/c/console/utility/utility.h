/*
 * utility
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include "json.h"
#include "DPA.h"

typedef struct {
    uint16_t NADR;
    uint8_t PNUM;
    uint8_t PCMD;
    uint16_t HWPID;
    uint8_t ResponseCode;
    uint8_t DpaValue;
    TDpaMessage DpaMessage;
} T_DPA_PACKET;

extern char HostIp[32];
extern char PTopic[64];
extern char STopic[64];

json_object * create_json_msg(T_DPA_PACKET *DpaRequest, unsigned int pDataLen, unsigned int timeout);
json_object * send_json_msg(json_object *IqrfDpaRequest);
void decode_json_msg(T_DPA_PACKET *DpaPacket, json_object *IqrfDpaResponse);

#endif /* UTILITY_H_ */
