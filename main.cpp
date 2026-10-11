// main.cpp - command-line front end for the Wake-on-LAN sender
#include <cstdlib>
#include <iostream>
#include <string>

#include "wol.hpp"

static void print_usage(const char *prog) {
    std::cout << "Wake-on-LAN magic packet sender\n\n"
              << "Usage: " << prog << " <MAC> [broadcast_ip] [port] [repeat]\n\n"
              << "  MAC           Target MAC (AA:BB:CC:DD:EE:FF, AA-BB-..., or AABBCC...)\n"
              << "  broadcast_ip  Default 255.255.255.255 (or e.g. 192.168.1.255)\n"
              << "  port          UDP port, default 9 (7 is also common)\n"
              << "  repeat        Times to send the packet, default 1 (max 100)\n\n"
              << "Example: " << prog << " AA:BB:CC:DD:EE:FF 192.168.1.255 9 3\n\n"
              << "Exit codes: 0 ok | 1 usage | 2 invalid input | 3 socket error | 4 send error\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    const std::string first = argv[1];
    if (first == "-h" || first == "--help") {
        print_usage(argv[0]);
        return 0;
    }

    const std::string mac_text = first;
    const std::string ip = (argc > 2) ? argv[2] : "255.255.255.255";
    const long port = (argc > 3) ? std::strtol(argv[3], nullptr, 10) : 9;
    const long repeat = (argc > 4) ? std::strtol(argv[4], nullptr, 10) : 1;

    wol::Mac mac;
    std::string err;
    if (!wol::parse_mac(mac_text, mac, err)) {
        std::cerr << "[ERROR] " << err << "\n";
        return 2;
    }
    if (port < 1 || port > 65535) {
        std::cerr << "[ERROR] Port must be between 1 and 65535.\n";
        return 2;
    }

    std::cout << "Sending magic packet to " << mac_text << " via " << ip << ":" << port
              << " (" << wol::PACKET_LEN << " bytes)\n";

    wol::SendResult r = wol::send_magic_packet(mac, ip, static_cast<std::uint16_t>(port),
                                               static_cast<int>(repeat));
    if (r.status != wol::Status::Ok) {
        if (r.packets_sent > 0)
            std::cout << "[OK] " << r.packets_sent << " packet(s) sent before the error.\n";
        std::cerr << "[ERROR] " << r.message << "\n";
        return static_cast<int>(r.status);
    }

    std::cout << "[OK] " << r.packets_sent << " packet(s) sent.\n"
              << "Done. If WOL is enabled on the target PC, it should power on shortly.\n";
    return 0;
}