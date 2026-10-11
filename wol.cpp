// wol.cpp - Wake-on-LAN library implementation (Windows / Linux / macOS)
#include "wol.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET socket_t;
  #define INVALID_SOCK INVALID_SOCKET
  #define CLOSE_SOCK closesocket
  static int last_error() { return WSAGetLastError(); }
  static std::string error_text(int code) {
      char *buf = nullptr;
      FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, code, 0, reinterpret_cast<LPSTR>(&buf), 0, nullptr);
      std::string s = buf ? buf : "Unknown error";
      if (buf) LocalFree(buf);
      while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
      return s;
  }
#else
  #include <arpa/inet.h>
  #include <cerrno>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  typedef int socket_t;
  #define INVALID_SOCK (-1)
  #define CLOSE_SOCK close
  static int last_error() { return errno; }
  static std::string error_text(int code) { return std::strerror(code); }
#endif

namespace wol {

bool parse_mac(const std::string &text, Mac &out, std::string &error) {
    std::string hex;
    for (char c : text) {
        if (c == ':' || c == '-') continue;
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            error = std::string("Invalid character '") + c + "' in MAC address.";
            return false;
        }
        hex += c;
    }
    if (hex.size() != MAC_LEN * 2) {
        error = "MAC address must have exactly 12 hex digits (got " +
                std::to_string(hex.size()) + ").";
        return false;
    }
    for (std::size_t i = 0; i < MAC_LEN; ++i)
        out[i] = static_cast<std::uint8_t>(
            std::strtoul(hex.substr(i * 2, 2).c_str(), nullptr, 16));
    return true;
}

std::vector<std::uint8_t> build_magic_packet(const Mac &mac) {
    std::vector<std::uint8_t> packet(PACKET_LEN);
    std::memset(packet.data(), 0xFF, 6);
    for (int i = 0; i < 16; ++i)
        std::memcpy(packet.data() + 6 + i * MAC_LEN, mac.data(), MAC_LEN);
    return packet;
}

namespace {
SendResult fail(Status s, const std::string &msg, int sent = 0) {
    SendResult r;
    r.status = s;
    r.message = msg;
    r.packets_sent = sent;
    return r;
}
std::string with_code(const std::string &what, int e) {
    return what + ": " + error_text(e) + " (code " + std::to_string(e) + ")";
}
}  // namespace

SendResult send_magic_packet(const Mac &mac, const std::string &broadcast_ip,
                             std::uint16_t port, int repeat) {
    if (port == 0) return fail(Status::InvalidInput, "Port must be between 1 and 65535.");
    if (repeat < 1 || repeat > 100)
        return fail(Status::InvalidInput, "Repeat count must be between 1 and 100.");

    sockaddr_in dest;
    std::memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    if (inet_pton(AF_INET, broadcast_ip.c_str(), &dest.sin_addr) != 1)
        return fail(Status::InvalidInput, "Invalid broadcast IP address: " + broadcast_ip);

#ifdef _WIN32
    WSADATA wsa;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (rc != 0) return fail(Status::SocketError, with_code("WSAStartup failed", rc));
#endif

    SendResult result;
    socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCK) {
        result = fail(Status::SocketError, with_code("Could not create socket", last_error()));
    } else {
        int enable = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                       reinterpret_cast<const char *>(&enable), sizeof(enable)) != 0) {
            result = fail(Status::SocketError,
                          with_code("Could not enable broadcast", last_error()));
        } else {
            const std::vector<std::uint8_t> packet = build_magic_packet(mac);
            for (int i = 1; i <= repeat; ++i) {
                int sent = static_cast<int>(
                    sendto(sock, reinterpret_cast<const char *>(packet.data()),
                           static_cast<int>(packet.size()), 0,
                           reinterpret_cast<const sockaddr *>(&dest), sizeof(dest)));
                if (sent < 0) {
                    result = fail(Status::SendError,
                                  with_code("Packet " + std::to_string(i) + " failed",
                                            last_error()),
                                  result.packets_sent);
                    break;
                }
                if (static_cast<std::size_t>(sent) != packet.size()) {
                    result = fail(Status::SendError,
                                  "Packet " + std::to_string(i) + " partially sent (" +
                                      std::to_string(sent) + "/" +
                                      std::to_string(packet.size()) + " bytes).",
                                  result.packets_sent);
                    break;
                }
                ++result.packets_sent;
            }
        }
        CLOSE_SOCK(sock);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return result;
}

}  // namespace wol