# Wake-on-LAN Sender

[![Build & Test](https://github.com/YOUR-USERNAME/Wake-on-LAN/actions/workflows/build.yml/badge.svg)](https://github.com/YOUR-USERNAME/Wake-on-LAN/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A small, dependency-free C++ command-line tool that powers on a PC remotely by sending a
**Wake-on-LAN (WOL) magic packet** over UDP broadcast. It runs on Windows, Linux and macOS and
reports any error that occurs while sending the packet.

## How it works

A magic packet is a 102-byte UDP datagram:

```
FF FF FF FF FF FF   <- 6 bytes of 0xFF
[MAC] x 16          <- target MAC address repeated 16 times (96 bytes)
```

The tool opens a UDP socket, enables `SO_BROADCAST`, and sends the packet to a broadcast address
(default `255.255.255.255`, port `9`). The target's network card sees its own MAC in the packet and
tells the motherboard to power on.

## Features

- Accepts MACs as `AA:BB:CC:DD:EE:FF`, `AA-BB-CC-DD-EE-FF` or `AABBCCDDEEFF`
- Custom broadcast IP, UDP port and repeat count
- Detects and reports: invalid MAC / IP / port, socket creation failure, broadcast failure,
  send failure and partial sends (with the OS error text and code)
- Distinct exit codes, so it is scriptable
- Unit tests and CI on Windows, Linux and macOS

## Requirements

- A C++11 compiler (GCC, Clang or MSVC)
- CMake 3.10+ (or just `make` / `g++` on Linux and macOS)
- **Target PC**: Wake-on-LAN enabled (see below)

## Build

### CMake (all platforms)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure   # run unit tests
```

The program is at `build/wol` (Linux/macOS) or `build\Release\wol.exe` (Windows, MSVC).

### Make (Linux / macOS)

```bash
make          # builds ./wol
make test     # builds and runs the unit tests
```

### g++ directly

```bash
g++ -std=c++11 -o wol src/wol.cpp src/main.cpp                  # Linux / macOS
g++ -std=c++11 -o wol.exe src/wol.cpp src/main.cpp -lws2_32     # Windows (MinGW)
```

## Usage

```
wol <MAC> [broadcast_ip] [port] [repeat]
```

| Argument       | Default           | Description                                |
|----------------|-------------------|--------------------------------------------|
| `MAC`          | (required)        | MAC address of the target PC               |
| `broadcast_ip` | `255.255.255.255` | Broadcast address, e.g. `192.168.1.255`    |
| `port`         | `9`               | UDP port (`7` is also common)              |
| `repeat`       | `1`               | Number of packets to send (1-100)          |

### Examples

```bash
./wol AA:BB:CC:DD:EE:FF
./wol AA-BB-CC-DD-EE-FF 192.168.1.255 9 3
```

Sample output:

```
Sending magic packet to AA:BB:CC:DD:EE:FF via 255.255.255.255:9 (102 bytes)
[OK] 1 packet(s) sent.
Done. If WOL is enabled on the target PC, it should power on shortly.
```

Error example:

```
$ ./wol ZZ:11
[ERROR] Invalid character 'Z' in MAC address.
```

### Exit codes

| Code | Meaning                                   |
|------|-------------------------------------------|
| 0    | Success                                   |
| 1    | Missing arguments (usage shown)           |
| 2    | Invalid input (MAC, IP, port, repeat)     |
| 3    | Socket creation / configuration error     |
| 4    | Send error or partially sent packet       |

> **Note:** UDP has no delivery confirmation. "Sent" means the packet left your machine,
> not that the target woke up.

## Enabling Wake-on-LAN on the target PC

1. **BIOS/UEFI** - enable *Wake on LAN* / *Power on by PCI-E*.
2. **Windows** - Device Manager -> network adapter -> Properties:
   - *Power Management*: tick "Allow this device to wake the computer" and
     "Only allow a magic packet to wake the computer".
   - *Advanced*: set "Wake on Magic Packet" to Enabled.
   - Disable Fast Startup (Control Panel -> Power Options) if it doesn't wake from shutdown.
3. **Linux** - `sudo ethtool -s eth0 wol g` (replace `eth0` with your interface).
4. Use **wired Ethernet**; Wi-Fi rarely supports WOL.
5. Find the MAC: `ipconfig /all` (Windows) or `ip link` (Linux).

## Waking a PC over the internet

Broadcasts don't cross routers by default. Either forward UDP port 9 on your router to the LAN
broadcast address (many routers don't allow this), or connect to an always-on device on the target
network (VPN, Raspberry Pi) and run this tool from there.

## Project structure

```
Wake-on-LAN/
├── .github/workflows/build.yml   CI: build + test on Windows, Linux, macOS
├── src/
│   ├── wol.hpp                   library interface
│   ├── wol.cpp                   MAC parsing, packet building, socket code
│   └── main.cpp                  command-line front end
├── tests/test_wol.cpp            unit tests
├── CMakeLists.txt
├── Makefile
├── .gitignore
├── LICENSE
└── README.md
```

## License

MIT - see [LICENSE](LICENSE).