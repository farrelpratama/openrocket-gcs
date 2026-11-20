#pragma once

#include <stdint.h>

static constexpr uint16_t COMMAND_MAGIC = 0xA55A;
static constexpr uint8_t COMMAND_VERSION = 1;

enum CommandType : uint8_t {
    CMD_TEST_START = 1,
    CMD_TEST_STOP  = 2,
    CMD_STATUS     = 3,
    CMD_EJECTION_TRIGGER = 4
};

enum AckStatus : uint8_t {
    ACK_ACCEPTED = 1,
    ACK_STOPPED  = 2,
    ACK_STATUS   = 3,
    ACK_REJECTED = 4
};

#pragma pack(push, 1)

struct CommandPacket {
    uint16_t magic;
    uint8_t version;
    uint8_t command;
    uint32_t commandId;
};

struct AcknowledgementPacket {
    uint16_t magic;
    uint8_t version;
    uint8_t status;
    uint32_t commandId;
    uint8_t ledActive;
    uint8_t ejectionActive;
};

#pragma pack(pop)

static_assert(sizeof(CommandPacket) == 8,
              "Unexpected CommandPacket size");

static_assert(sizeof(AcknowledgementPacket) == 10,
              "Unexpected ACK packet size");
