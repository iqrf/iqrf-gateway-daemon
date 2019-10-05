/*
 * Console.h
 *
 *  Created on: Jul 21, 2019
 *      Author: dudo
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

#define CCP_COMMAND_NOT_FOUND     0
#define CCP_DONE                  1
#define CCP_SD_CARD_ERR           2
#define CCP_UPLOADING             3
#define CCP_BAD_PARAMETER         4
#define CCP_FILE_NOT_FOUND        5
#define CCP_DIR_NOT_FOUND         6
#define CCP_SENDING_REQUEST       7
#define CCP_CONFIRMATION_OK       8
#define CCP_CONFIRMATION_ERR      9
#define CCP_RESPONSE_OK           10
#define CCP_RESPONSE_ERR          11
#define CCP_COMMAND_TIMEOUT       12
#define CCP_CODE_STORED           13
#define CCP_CODE_STORE_ERR        14
#define CCP_TR_NOT_READY          15
#define CCP_CONFIG_FILE_ERR       16

#define BUFF 256

extern char InLine[BUFF];
extern int InLinePosition;

int ccp_read_substring(int *StartPos, char *DestinationString, char *SourceString);
void sys_msg_printer(__uint16_t CommandParameter);

#endif /* CONSOLE_H_ */
