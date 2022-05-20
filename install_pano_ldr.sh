#!/bin/sh

# set -x 
pano_ip=`./prebuilt/install_rev_b --discover pano | head -n3 | tail -n1 | cut -d' ' -f1`
if [ "x$pano_ip" = "xTest" ]; then
    echo "Error: no Panos were found"
    exit 1
fi
echo "Pano IP: $pano_ip"
boardid=`./prebuilt/install_rev_b $pano_ip chipIdRd | grep CFG |  cut -d' ' -f 5`

if [ "x$boardid" = "x0x00050000" ]; then
    echo "The detected Pano is a first generation device which is not supported"
    exit 1
elif [ "x$boardid" = "x0x10010001" ]; then
    device="Pano G1+"
    rev=c
elif [ "x$boardid" = "x0x08010000" ]; then
    device="Rev B Pano G2"
    rev=b
elif [ "x$boardid" = "x0x08010002" ]; then
    device="Rev C Pano G2"
    rev=c
elif [ "x$boardid" = "x0x08011000" ]; then
    device="Fujitsu DZ22-2"
    rev=b
else
    echo
    echo "Unknown Pano boardId '$boardid'"
    echo "Please open an issue and report this to https://github.com/skiphansen/panog2_ldr/issues with the following"
    echo "with the following:"
    echo "----"
    ./prebuilt/install_rev_b $pano_ip chipIdRd
    echo "----"
    echo
    echo "You might also post to https://gitter.im/panologic/community for help"
    exit 1
fi

echo ""
echo -n "Install pano_ldr on $device (y/n) ?"
read answer
if [ x$answer = "xy" -o x$answer = "xyes" ]; then
    rm BurninLowLevel${pano_ip}.log > /dev/null
    ./prebuilt/install_rev_${rev} $pano_ip flashFPGA_Series2_Multiboot &
    installer_pid=$!
    sleep 1
    tail -f BurninLowLevel${pano_ip}.log &
    wait $installer_pid
    rm BurninLowLevel${pano_ip}.log
    echo "Pano_ldr installed, power cycle the Pano to start it"
fi

