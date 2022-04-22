#!/bin/bash

set -x
loadbits.sh ../../fpga/build.good/fpga_routed.bit
/home/skip/pano/working/panog2_usb_sniffer/pano/tools/dbg_bridge/poke.py -t uart -d /dev/ttyUSB.Pano -b 1000000 -a 0xF0000000 -v 0x1
if [ $? -ne 0 ]; then echo "poke failed"; exit; fi
/home/skip/pano/working/panog2_usb_sniffer/pano/tools/dbg_bridge/poke.py -t uart -d /dev/ttyUSB.Pano -b 1000000 -a 0xF0000000 -v 0x0
if [ $? -ne 0 ]; then echo "poke failed"; exit; fi
/home/skip/pano/working/panog2_usb_sniffer/pano/tools/dbg_bridge/load.py -t uart -d /dev/ttyUSB.Pano -b 1000000 -f build/lwip_test -p ''
if [ $? -ne 0 ]; then echo "load failed"; exit; fi
/home/skip/pano/working/panog2_usb_sniffer/pano/tools/dbg_bridge/console-uart.py -t uart -d /dev/ttyUSB.Pano -b 1000000

