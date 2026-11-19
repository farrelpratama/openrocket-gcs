#ifndef COMMAND_PACKET_H
#define COMMAND_PACKET_H
#include <stdint.h>

// Protocol shared by GCS and rocket firmware.
static constexpr uint16_t COMMAND_MAGIC = 0xA55A;
static constexpr uint8_t COMMAND_PROTOCOL_VERSION = 1;

enum CommandType : uint8_t {
    COMMAND_LED_TEST_START = 1,
    COMMAND_LED_TEST_ABORT = 2
};

enum AckStatus : uint8_t {
    ACK_ACCEPTED = 1,
    ACK_COMPLETED = 2,
    ACK_ABORTED = 3,
    ACK_REJECTED = 4,
    ACK_BUSY = 5
};

#pragma pack(push, 1)
struct CommandPacket {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint32_t commandId;
};

struct AcknowledgementPacket {
    uint16_t magic;
    uint8_t version;
    uint8_t status;
    uint32_t commandId;
};
#pragma pack(pop)

static_assert(sizeof(CommandPacket) == 8, "Unexpected command packet size");
static_assert(sizeof(AcknowledgementPacket) == 8, "Unexpected ACK packet size");
#endif
