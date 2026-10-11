// test_wol.cpp - dependency-free unit tests (no network access needed)
#include <iostream>
#include <string>

#include "../wol.hpp"

static int failures = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "FAIL: " #cond " (line " << __LINE__ << ")\n";    \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

int main() {
    wol::Mac mac;
    std::string err;

    // --- valid MAC formats ---
    CHECK(wol::parse_mac("AA:BB:CC:DD:EE:FF", mac, err));
    CHECK(mac[0] == 0xAA && mac[5] == 0xFF);
    CHECK(wol::parse_mac("aa-bb-cc-dd-ee-ff", mac, err));
    CHECK(wol::parse_mac("001122334455", mac, err));
    CHECK(mac[1] == 0x11 && mac[4] == 0x44);

    // --- invalid MACs ---
    CHECK(!wol::parse_mac("", mac, err));
    CHECK(!wol::parse_mac("AA:BB:CC:DD:EE", mac, err));
    CHECK(!wol::parse_mac("AA:BB:CC:DD:EE:FF:00", mac, err));
    CHECK(!wol::parse_mac("GG:BB:CC:DD:EE:FF", mac, err));
    CHECK(!err.empty());

    // --- packet layout ---
    wol::parse_mac("01:23:45:67:89:AB", mac, err);
    auto p = wol::build_magic_packet(mac);
    CHECK(p.size() == 102);
    for (int i = 0; i < 6; ++i) CHECK(p[i] == 0xFF);
    for (int rep = 0; rep < 16; ++rep)
        for (int b = 0; b < 6; ++b) CHECK(p[6 + rep * 6 + b] == mac[b]);

    // --- input validation in send (fails before any socket is opened) ---
    CHECK(wol::send_magic_packet(mac, "999.1.1.1", 9, 1).status == wol::Status::InvalidInput);
    CHECK(wol::send_magic_packet(mac, "255.255.255.255", 0, 1).status == wol::Status::InvalidInput);
    CHECK(wol::send_magic_packet(mac, "255.255.255.255", 9, 0).status == wol::Status::InvalidInput);
    CHECK(wol::send_magic_packet(mac, "255.255.255.255", 9, 101).status == wol::Status::InvalidInput);

    if (failures == 0) std::cout << "All tests passed.\n";
    return failures == 0 ? 0 : 1;
}