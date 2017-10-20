# cantools
A collection of CAN bus tools

Description
---
These tools can be used - and were developed - using a Raspberry Pi 3 with a PiCAN2 HAT.
* __cantx__: one-time or cyclic transmission of a single frame
* __canprint__: printing frames into the console
* __cangw__: routing frames between CAN and UDP

Build
---
Use `make` to build all tools or `make cangw` etc.

Status
---
WIP

Usage
---
| Tool | Parameter | Short | Required | Default | Description |
| ---- | --------- | :---: | :------: | ------- | ----------- |
| cantx | device<br>id<br>data<br>cycle | <br><br><br> | <br>✓<br><br><br> | can0<br><br>00<br>-1 (send once) | CAN device<br>Frame ID<br>Hex data string<br>Repetition time in ms |
| canprint | device | | | can0 | CAN device |
| cangw | listen<br>send<br>realtime<br>timestamp<br>device<br>ip<br>port | -l<br>-s<br>-r<br>-t<br>-d<br>-i<br>-p |-l ∨ -s<br>-l ∨ -s<br><br><br><br>✓<br>✓ | <br><br>false<br>false<br>can0<br><br><br> | Route frames from CAN to UDP<br>Route frames from UDP to CAN<br>Enable realtime scheduling policy<br>Prefix UDP payload with a timestamp<br>CAN device<br>IP of remote device<br>UDP port |



Examples:
```bash
# Send frame each 100 ms
$ ./cantx --device=can0 --id=42 --data=0102030405060708 --cycle=100

# Route frames from CAN to UDP
$ ./cangw --listen --ip=192.168.1.5 --port=30001

# Same but using short options
$ ./cangw -li 192.168.1.5 -p=30001

# Route frames between interfaces and add timestamps to UDP payload
$ ./cangw -lsti 192.168.1.5 -p 30001
```

Acknowledgements
---
[cxxopts](https://github.com/jarro2783/cxxopts) for parsing command line options
