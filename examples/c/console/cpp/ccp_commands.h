/*
 * ccp_commands.h
 */

#ifndef CCP_COMMANDS_H_
#define CCP_COMMANDS_H_

void ccp_leds_cmd (__uint16_t CommandParameter);
void ccp_coordinator_cmd (__uint16_t CommandParameter);
void ccp_node_cmd (__uint16_t CommandParameter);
void ccp_os_cmd (__uint16_t CommandParameter);

#endif /* CCP_COMMANDS_H_ */
