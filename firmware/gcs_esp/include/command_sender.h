
#ifndef COMMAND_SENDER_H
#define COMMAND_SENDER_H

#include <stdint.h>

bool initCommandSender(const uint8_t* rocketMAC);
bool sendLedTestCommand(uint32_t commandId);

#endif
