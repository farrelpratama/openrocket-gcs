
#ifndef COMMAND_SENDER_H
#define COMMAND_SENDER_H

#include <Arduino.h>

bool initCommandSender(const uint8_t* rocketMAC);

bool sendLedTestCommand(uint32_t commandId);

#endif
