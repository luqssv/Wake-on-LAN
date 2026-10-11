// wol.hpp - Wake-on-LAN library interface
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wol {

constexpr std::size_t MAC_LEN = 6;
constexpr std::size_t PACKET_LEN = 6 + 16 * MAC_LEN;  // 102 bytes

using Mac = std::array<std::uint8_t, MAC_LEN>;

enum class Status {
    Ok = 0,
    InvalidInput = 2,  // bad IP / port / repeat count
    SocketError = 3,   // socket creation or configuration failed
    SendError = 4      // sendto() failed or sent a partial packet
};

struct SendResult {
    Status status = Status::Ok;
    std::string message;   // human-readable error (empty on success)
    int packets_sent = 0;  // how many packets were fully sent
};

// Parses "AA:BB:CC:DD:EE:FF", "AA-BB-CC-DD-EE-FF" or "AABBCCDDEEFF".
// On failure returns false and fills `error`.
bool parse_mac(const std::string &text, Mac &out, std::string &error);

// 6 x 0xFF followed by the MAC repeated 16 times.
std::vector<std::uint8_t> build_magic_packet(const Mac &mac);

// Sends the magic packet `repeat` times as a UDP broadcast.
SendResult send_magic_packet(const Mac &mac, const std::string &broadcast_ip,
                             std::uint16_t port, int repeat);

}  // namespace wol