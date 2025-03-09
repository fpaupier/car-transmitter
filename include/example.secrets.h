#pragma once

#include <cstdint>

// Convert MAC format XX:XX:XX:XX:XX:XX to hex array
// Example: if MAC is 3C:61:05:12:34:56
// then use: {0x3C, 0x61, 0x05, 0x12, 0x34, 0x56}

const uint8_t RECEIVER_MAC_ADDRESS[] = {0x3C, 0x61, 0x05, 0x12, 0x34, 0x56};
