
#ifndef COMMAND_PACKET_H
#define COMMAND_PACKET_H

#include <stdint.h>

static constexpr uint16_t COMMAND_MAGIC = 0xC35A;
static constexpr uint8_t COMMAND_VERSION = 1;

enum CommandType : uint8_t {
    COMMAND_LED_TEST = 1,
    COMMAND_ACK = 2
};

enum CommandStatus : uint8_t {
    STATUS_ACCEPTED = 1,
    STATUS_COMPLETED = 2,
    STATUS_REJECTED = 3
};

#pragma pack(push, 1)
struct CommandPacket {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint32_t commandId;
    uint8_t status;
};
#pragma pack(pop)

static_assert(
    sizeof(CommandPacket) == 9,
    "CommandPacket size mismatch"
);

inline bool isValidCommand(const CommandPacket& packet) {
    return packet.magic == COMMAND_MAGIC &&
           packet.version == COMMAND_VERSION;
}

#endif
