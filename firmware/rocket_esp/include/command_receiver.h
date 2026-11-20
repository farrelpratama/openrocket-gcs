
#pragma once

#include <Arduino.h>

bool initCommandReceiver(const uint8_t* gcsMAC);
void processCommandReceiver();
