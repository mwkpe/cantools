# cantx
A small program for transmitting a CAN message

Description
---
A command line program for the single or cyclic transmission of a CAN message.

Build
---
g++ -std=c++14 -pthread -o cantx cantx.cpp

Usage
---
| Option   | Description  |            |
| -------- | ------------ | ---------- |
| --id     | Hex frame ID (0-7ff) | required |
| --data   | Hex data string (size must be even and <= 16) | optional (default "00") |
| --cycle  | Cycle time (ms), -1 for sending frame once | optional (default -1) |
| --device | CAN device name | optional (default "can0") |

Example:<br>
./cantx --id=2a --data=0102030405FF0708 --cycle=100 --device=can0

Acknowledgements
---
Adapted from [cansend.c](https://github.com/linux-can/can-utils/blob/master/cansend.c)<br>
[cxxopts](https://github.com/jarro2783/cxxopts) for parsing command line options
