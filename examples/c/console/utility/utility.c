/*
 * utility.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <assert.h>
#include "json.h"

#include "MQTTClient.h"

#include "utility.h"

#define PCLIENTID    "ExampleClientPub"
#define SCLIENTID    "ExampleClientSub"
#define QOS          1
#define PTIMEOUT     3000L

char HostIp[32];
char PTopic[64];
char STopic[64];

/**
 * create JSON object from DPA Request parameters
 * @param *DpaRequest - pointer to DPA request
 * @param pDataLen - number of bytes in buffer with DPA request additional data
 * @param timeout - timeout of created DPA request
 * @return pointer to created JSON object
 */
json_object * create_json_msg(T_DPA_PACKET *DpaRequest, unsigned int pDataLen, unsigned int timeout)
{
    int cnt;
    static unsigned int msgCnt = 0;
    char msgId[32];

    json_object *iqrfMsg;
    json_object *iqrfData;
    json_object *iqrfRq;
    json_object *iqrfPData;

    iqrfMsg = json_object_new_object();
    iqrfData = json_object_new_object();
    iqrfRq = json_object_new_object();
    strcpy(msgId, "dpaRq");

    json_object_object_add(iqrfRq, "nAdr", json_object_new_int(DpaRequest->NADR));
    json_object_object_add(iqrfRq, "pNum", json_object_new_int(DpaRequest->PNUM));
    json_object_object_add(iqrfRq, "pCmd", json_object_new_int(DpaRequest->PCMD));
    json_object_object_add(iqrfRq, "hwpId", json_object_new_int(DpaRequest->HWPID));
    if (pDataLen != 0) {
        iqrfPData = json_object_new_array();
        for (cnt=0; cnt<pDataLen; cnt++) {
            json_object_array_add(iqrfPData, json_object_new_int(DpaRequest->DpaMessage.Request.PData[cnt]));
        }
        json_object_object_add(iqrfRq, "pData", iqrfPData);
    }

    sprintf(&msgId[5], "%d", msgCnt++);
    json_object_object_add(iqrfData, "msgId", json_object_new_string(msgId));
    if (timeout != 0) {
        json_object_object_add(iqrfData, "timeout", json_object_new_int(timeout));
    }
    json_object_object_add(iqrfData, "req", iqrfRq);
    json_object_object_add(iqrfData, "returnVerbose", json_object_new_boolean(1));

    json_object_object_add(iqrfMsg, "mType", json_object_new_string("iqrfRawHdp"));
    json_object_object_add(iqrfMsg, "data", iqrfData);

    return(iqrfMsg);
}

/**
 * Convert to string and publish DPA Request to MQTT broker. Then wait
 * for DPA response and parse it to JSON object
 * @param *IqrfDpaRequest - pointer to DPA request JSON object
 * @return pointer to DPA response JSON object, or NULL, in case of wrong or undelivered response
 */
json_object * send_json_msg(json_object *IqrfDpaRequest)
{
    MQTTClient Pclient, Sclient;
    MQTTClient_connectOptions Pconn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_connectOptions Sconn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message Pubmsg = MQTTClient_message_initializer;
    MQTTClient_message *Submsg;
    MQTTClient_deliveryToken Token;
    char *ReceivedTopic;
    int	ReceivedTopicLen;
    int rc;
    unsigned long STimeout = 2000L;				// default timeout for MQTT response

    json_object *IqrfDpaResponse = NULL;
    json_object *IqrfFind;
    json_object *IqrfDpaResponseStatus;

    // print DPA request in JSON format
    printf("MQTT - DPA Request =\n%s\n\n", json_object_to_json_string(IqrfDpaRequest));

    // create MQTT subscribe client
    MQTTClient_create(&Sclient, HostIp, SCLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    Sconn_opts.keepAliveInterval = 20;
    Sconn_opts.cleansession = 1;

    // connect MQTT subscribe client to MQTT broker
    if ((rc = MQTTClient_connect(Sclient, &Sconn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n\n", rc);
        MQTTClient_destroy(&Sclient);
        return(NULL);
    }
    MQTTClient_subscribe(Sclient, STopic, QOS);

    // create MQTT publish client
    MQTTClient_create(&Pclient, HostIp, PCLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    Pconn_opts.keepAliveInterval = 20;
    Pconn_opts.cleansession = 1;

    // connect MQTT publish client to MQTT broker
    if ((rc = MQTTClient_connect(Pclient, &Pconn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        MQTTClient_unsubscribe(Sclient, STopic);
        MQTTClient_disconnect(Sclient, 10000);
        MQTTClient_destroy(&Sclient);
        MQTTClient_destroy(&Pclient);
        return(NULL);
    }

    // create MQTT publish message
    Pubmsg.payload = json_object_to_json_string(IqrfDpaRequest);
    Pubmsg.payloadlen = (int)strlen(json_object_to_json_string(IqrfDpaRequest));
    Pubmsg.qos = QOS;
    Pubmsg.retained = 0;
    MQTTClient_publishMessage(Pclient, PTopic, &Pubmsg, &Token);
    rc = MQTTClient_waitForCompletion(Pclient, Token, PTIMEOUT);
    printf("Message with delivery token %d delivered\n\n", Token);

    // add to default subscribe timeout, DPA timeout (if defined in JSON DPA Request)
    IqrfFind = IqrfDpaRequest;
    if (json_object_object_get_ex(IqrfFind, "data", &IqrfFind) == 1) {
        if (json_object_object_get_ex(IqrfFind, "timeout", &IqrfFind) == 1) {
            STimeout += json_object_get_int(IqrfFind);
        }
    }

    // wait for DPA response
    rc = MQTTClient_receive(Sclient, &ReceivedTopic, &ReceivedTopicLen, &Submsg, STimeout);

    // if response not received, or timeout elapsed
    if ((rc!=MQTTCLIENT_SUCCESS && rc!=MQTTCLIENT_TOPICNAME_TRUNCATED) || Submsg == NULL) {
        printf("MQTT - DPA Response not received\n");
        goto mqtt_disconnect;
    }

    // print and parse DPA Response in JSON format (received from MQTT broker)
    printf("MQTT - DPA Response =\n%s\n\n", Submsg->payload);
    IqrfDpaResponse = json_tokener_parse(Submsg->payload);

    // destroy unnecessary MQTT message (free memory)
    MQTTClient_freeMessage(&Submsg);
    MQTTClient_free(ReceivedTopic);

mqtt_disconnect:
    MQTTClient_disconnect(Pclient, 10000);
    MQTTClient_destroy(&Pclient);

    MQTTClient_unsubscribe(Sclient, STopic);
    MQTTClient_disconnect(Sclient, 10000);
    MQTTClient_destroy(&Sclient);

    // check if the parse of response message was successful
    if (IqrfDpaResponse != NULL) {
        printf("Parse OK\n");
    } else {
        printf("Parse error\n\n");
        return (NULL);
    }

    // find status of DPA Response
    // if status is not OK, print identification of error
    IqrfFind = IqrfDpaResponse;
    if (json_object_object_get_ex(IqrfFind, "data", &IqrfFind) == 1) {
        if (json_object_object_get_ex(IqrfFind, "status", &IqrfDpaResponseStatus) == 1) {
            if (json_object_get_int(IqrfDpaResponseStatus) != 0) {
                if (json_object_object_get_ex(IqrfFind, "statusStr", &IqrfDpaResponseStatus) == 1) {
                    printf("DPA Response error = %s\n\n", json_object_get_string(IqrfDpaResponseStatus));
                    return(NULL);
                }
            } else {
                printf("DPA Response OK\n\n");
                return(IqrfDpaResponse);
            }
        }
    }

    printf("DPA Response error = Key not found\n\n");
    return(NULL);
}

/**
 * Decode DPA Response in JSON object to T_DPA_PACKET structure
 * @param *DpaPacket - pointer to structure T_DPA_PACKET (destination of decode function)
 * @param *IqrfDpaResponse - pointer to DPA response JSON object (source for decode function)
 * @return none
 */
void decode_json_msg(T_DPA_PACKET *DpaPacket, json_object *IqrfDpaResponse)
{
    json_object *IqrfFind;
    json_object *IqrfDpaObject;
    int pDataLen, idx;

    memset(DpaPacket, 0, sizeof(T_DPA_PACKET));

    IqrfFind = IqrfDpaResponse;
    if (json_object_object_get_ex(IqrfFind, "data", &IqrfFind) == 1) {
        if (json_object_object_get_ex(IqrfFind, "rsp", &IqrfFind) == 1) {
            if (json_object_object_get_ex(IqrfFind, "nAdr", &IqrfDpaObject) == 1) {
                DpaPacket->NADR = json_object_get_int(IqrfDpaObject);
            }
            if (json_object_object_get_ex(IqrfFind, "pNum", &IqrfDpaObject) == 1) {
                DpaPacket->PNUM = json_object_get_int(IqrfDpaObject);
            }
            if (json_object_object_get_ex(IqrfFind, "pCmd", &IqrfDpaObject) == 1) {
                DpaPacket->PCMD = json_object_get_int(IqrfDpaObject);
            }
            if (json_object_object_get_ex(IqrfFind, "hwpId", &IqrfDpaObject) == 1) {
                DpaPacket->HWPID = json_object_get_int(IqrfDpaObject);
            }
            if (json_object_object_get_ex(IqrfFind, "rCode", &IqrfDpaObject) == 1) {
                DpaPacket->ResponseCode = json_object_get_int(IqrfDpaObject);
            }
            if (json_object_object_get_ex(IqrfFind, "dpaVal", &IqrfDpaObject) == 1) {
                DpaPacket->DpaValue = json_object_get_int(IqrfDpaObject);
            }
            if (json_object_object_get_ex(IqrfFind, "pData", &IqrfFind) == 1) {
                pDataLen = json_object_array_length(IqrfFind);
                if (pDataLen) {
                    for (idx=0; idx<pDataLen; idx++) {
                        IqrfDpaObject = json_object_array_get_idx(IqrfFind, idx);
                        DpaPacket->DpaMessage.Response.PData[idx] = json_object_get_int(IqrfDpaObject);
                    }
                }
            }
        }
    }
}
