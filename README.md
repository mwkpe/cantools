# cantx
A small program for transmitting a CAN message

Description
---
A command line program for the cyclic transmission of a single CAN message.<br>
Developed using a Raspberry Pi 3 with a PiCAN2 CAN-Bus board.

Build
---
g++ -std=c++14 -pthread -o cantx cantx.cpp

Usage
---
| Option   | Description  |            |
| -------- | ------------ | ---------- |
| --id     | Hex message ID (0-7ff) | required |
| --data   | Hex data string (size must be even and <= 16) | optional (default "00") |
| --cycle  | Cycle time (ms) | optional (default 100) |
| --device | CAN device name | optional (default "can0") |

Example:<br>
./cantx --id=2a --data=0102030405FF0708 --cycle=100 --device=can0

Acknowledgements
---
cantx is adapted from [can-utils](https://github.com/linux-can/can-utils) (see [cansend.c](https://github.com/linux-can/can-utils/blob/master/cansend.c)) and uses [cxxopts](https://github.com/jarro2783/cxxopts) for parsing command line options.
