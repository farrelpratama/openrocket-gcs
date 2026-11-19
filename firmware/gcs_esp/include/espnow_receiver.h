
#pragma once

#include <Arduino.h>

bool initReceiver();
bool sendRocketCommand(uint8_t command);
void processGCSInput();
