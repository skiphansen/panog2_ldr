# Pano_ldr

[https://github.com/skiphansen/panog2_ldr](https://github.com/skiphansen/panog2_ldr)

The purpose of this project is to allow a user to flash and run their own
projects on [Pano G2](https://github.com/tomverbeure/panologic-g2/wiki/Identifying-different-Pano-generations-and-revisions#second-generation-rev-b-g2_b)
and [Fujitsu DZ22-2](https://github.com/tomverbeure/panologic-g2/wiki/Identifying-different-Pano-generations-and-revisions#fujitsu-zero-client-dz22-2)
thin clients without a JTAG programer. Unlike my [pano_progfpga](https://github.com/skiphansen/pano_progfpga) project
pano_ldr is a bitstream for the Pano which can be co-resident with the user's
bitstream.

It is possible to install pano_ldr on a factory stock Pano G2 using [pano_progfpga](https://github.com/skiphansen/pano_progfpga)
without opening the case.  This is particularly important for owners of the 
DZ22-2 since it's almost impossible to open the case without damage to the 
case, the user or both.

Once pano_ldr has been installed the user can telnet into the Pano and issue 
commands to update the flash and run applications.

Unlike Pano's progfpga pano_ldr is fast, limited only by the speed of the flash 
chip itself.  Flashing with pano_ldr over the network should  is just as fast 
or faster than as using a JTAG programmer.

## Usage

Pano_ldr provides the user with a command line interface (CLI) via a 
[telnet](https://en.wikipedia.org/wiki/Telnet) connection.  Commands that are 
provided for reading and writing flash as well as configuring Pano_ldr to
automatically boot the user's application on power on or reset.

```
skip@Dell-7040:~/pano/working/panog2_ldr$ telnet pano_ldr
Trying 192.168.123.196...
Connected to pano_ldr.lan.
Escape character is '^]'.
Pano LDR v0.01 compiled May 14 2022 10:48:14
ldr> help
Commands:
  auto    - [on | off]
  bootadr - <flash adr>
  dump    - <flash adr> <length>
  erase   - <start adr> <end adr>
  flash   - <filename> <flash adr>
  map     - Display blank regions in flash
  reboot  - <flash adr>
  reflash - <filename> <flash adr> flash (w/o auto erase)
  tftp    - <IP adr of tftp server>
  verify  - <filename> <flash adr>
ldr>
```

## HW Requirements

* A Pano Logic G2 (the one with a DVI port) or a DZ22-2
* A suitable 5 volt power supply

## Software Requirements

* GNU make
* Host (or VM) capable of running 32 bit Linux programs
* DHCP server somewhere on the LAN
* TFTP server somewhere on the LAN

## Installation

There are two way to install the loader, using a JTAG programmer or over the 
network.

Using a JTAG programmer is the fastest way for a developer who already has
a JTAG progammer connected. 

## Installation without a JTAG programmer

A script that uses Pano update utilities which have been patched by the 
[pano_progfpga](https://github.com/skiphansen/pano_progfpga) project is
provide to install pano_ldr of the network.  The Pano update utilities are 32 
bit Linux x86 programs that can be run on  any OS that can run 32 bit Linux binaries.  

You should be able to run these programs on a modern X86 Linux by installing
[32 bit libraries](https://linuxconfig.org/unable-to-find-a-suitable-destination-to-install-32-bit-compatibility-libraries-on-ubuntu-18-04-bionic-beaver-linux).
Alternately they can be run in a VM running a 32 bit version of Linux. 
If you want to go the VM route start [here](https://www.youtube.com/watch?v=DPkF5EisGDQ), or create a 32 bit Linux VM of your choice.

The installation procedure is:

* Plug the Pano into your local LAN.
* Turn it on.
* Wait for it to obtain an IP address via DHCP.
* Run install_pano_ldr.sh
* Verify that you wnat to proceed
* Cross your fingers and wait as the Pano is updated.  (Go get coffee, 
this will take a good amount of time).  
* Power cycle the Pano 

For example:

```
```

When progfpga is used to install pano_ldr it will be flashed to the 
"multiboot bitstream" partition. 

## Installation using a JTAG programmer

Install xc3sprog for your system.  If a binary install isn't available for your
system the original project can be found here: https://sourceforge.net/projects/xc3sprog/.
Sources can be checked out using subversion from https://svn.code.sf.net/p/xc3sprog/code/trunk.

As an alternate if you don't have subversion a fork of the original project
can be found here: https://github.com/Ole2mail/xc3sprog.git .

If your JTAG cable is not a Digilent JTAG-HS2 then you will need to set the
"CABLE" environment variable to your cable type before loading the bit file.

Refer to the supported hardware [web page](http://xc3sprog.sourceforge.net/hardware.php) page or run  xc3sprog -c 
to find the correct cable option for your device.

**IMPORTANT: There are 2 versions of the Pano Logic G2: some use a Spartan 6 
LX150 while others use an LX100. You should be able to distinguish between the 
two by the revision code: LX150 is rev B and LX100 is rev C.  

The bit file and the embedded firmware must be generated for the correct device, 
the binary images are NOT compatible.  The build system uses the PLATFORM 
environment variable to determine the target device.  If the PLATFORM environment 
variable is not set a revision A or B device is assumed.

Set the PLATFORM environment variable to "pano-g2-c" if your device is a 
revision C before running make or specify it on the make command line.

Once xc3sprog has been in installed the bit file can be programmed into the 
Pano's SPI flash by running "make prog_fpga".

## Loader Commands

All arguments can be specifed in decimal or hex.  For hex prefix the hex value
with '0x'.  To save typing commands may be abbriviated.

The "help" command lists available commands for a quick memory refresh.

## Dump \<flash adr> \<length>

The dump command displays the specified range of flash in hex.

## Erase  \<start adr> \<end adr>

The erase ccommand erases the specified range of flash.  This command does not
need be use before using the flash command.  

The start address must be on an erase boundary and the end address must be 
one byte less than the next erase boundary.  

For a rev B the erase size is 
256k or 0x40000, for a rev C it is 64K or 0x10000.

For example on a rev b:
```
ldr> map
M25P128: 16 MB, 256 KB sectors
0x000000 -> 0x8fffff 9216 K not empty
0x900000 -> 0xffffff 7168 K empty
ldr> erase 0x480000 0x4bffff
.
Erased 256K
ldr> map
M25P128: 16 MB, 256 KB sectors
0x000000 -> 0x47ffff 4608 K not empty
0x480000 -> 0x4bffff  256 K empty
0x4c0000 -> 0x8fffff 4352 K not empty
0x900000 -> 0xffffff 7168 K empty
ldr>
```

## Reboot \<flash adr>

The reboot command initiates a FPGA reconfiguration using the bitstream at the 
specified address.


## Flash \<filename> \<flash adr>

The flash command programs flash with the specified file from the TFTP server.
Flash is automatically erased as needed.

## Map

The map command scans flash displaying the regions of flash which are
empty and not empty.

For example on a Rev C Pano with stock partitioning:

```
ldr> map
M25P64: 8 MB, 64 KB sectors
0x000000 -> 0x00ffff   64 K not empty
0x010000 -> 0x03ffff  192 K empty
0x040000 -> 0x36ffff 3264 K not empty
0x370000 -> 0x37ffff   64 K empty
0x380000 -> 0x6affff 3264 K not empty
0x6b0000 -> 0x6bffff   64 K empty
0x6c0000 -> 0x6cffff   64 K not empty
0x6d0000 -> 0x7fffff 1216 K empty
```

## reflash \<filename> \<flash adr>

The reflash command programs flash without erasing it first.  This allows
multiple files to be programmed into the same erase block. 

## tftp [\<IP adr>]

The tftp command is used display or set the IP address of the TFTP server.  
The TFTP server MUST be on the same subnet as the Pano.  The TFTP server address 
is saved in flash 

Note:  By default the TFTP server address is set to the IP address of the first
telnet connection.  It is only necessary to set the TFTP sever address if the
TFTP server is not on the same machine which is used to telnet in the loader.

## verify \<filename> \<flash adr>

The verify command compares flash with the specified file from the TFTP server.

## Status LED

The Pano button's LED is used to show pano_ldr's network state.

| State | LED |
| - | - |
| Ethernet link down | Flashing red |
| Ethernet link up, IP address not assigned | Flashing blue |
| IP address assigned by DHCP, not connected | Flashing green |
| User connected | Solid green |

## Erase Boundaries

Flash chips can be read and written in single byte increments, however they
can only be erased in blocks.  

As a consequence of this the flash commands and erase commands must start
on an erase boundrary

| Pano | flash size | erase block size | number of blocks |
| - | - | - | - |
| rev B | 16 MBytes | 256k | 64 |
| rev C | 8 MBytes | 64k | 128 |

## Rev B erase blocks

| Block number | Adress range |
| - | - |
| 0 | 0x000000 -> 0x03ffff |
| 1 | 0x040000 -> 0x07ffff |
| 2 | 0x080000 -> 0x0bffff |
| ... |
| 62 | 0xf80000 -> 0xfbffff |
| 63 | 0xfc0000 -> 0xffffff |

## Rev C erase blocks

| Block number | Adress range |
| - | - |
| 0 | 0x000000 -> 0x00ffff |
| 1 | 0x010000 -> 0x01ffff |
| 2 | 0x020000 -> 0x02ffff |
| ... |
| 126 | 0x7e0000 -> 0x7effff |
| 127 | 0x7f0000 -> 0x7fffff |



|     Byte address     |        usage        | size     |
|----------------------|---------------------|----------|
| 0x000000 -> 0x03ffff | multiboot header    | 256k     |
| 0x040000 -> 0x446534 | golden bitstream    | 4122k    |
| 0x480000 -> 0x886534 | multiboot bitstream | 4122k    |
| 0x8c0000 -> 0x8fffff | MAC adr and unknown | 256k     |
| 0x900000 -> 0x446534 | golden bitstream    | 4122k    |
| 0x900000 -> 0xd3ffff | User bitstream      | 4122k    |
| 0xd40000 -> 0xffffff | Unused              | 2816k    |


When progfpga is used to install pano_ldr it will be flashed to the 
"multiboot bitstream" partition. 


## Flash Partitioning Schemes

Once pano_ldr has been installed it is possible to repartition the flash 
to make it more efficient and convenient.  

Using bit stream compression can save a **CONSIDERABLE* amount of flash space,
for example a compressed bit stream of pano_ldr could occupy ?? flash sectors
rather than 17.


## Flash Partition: Safe

A very conser

|     Byte address     |                   usage        | size  |
|----------------------|--------------------------------|-------|
| 0x000000 -> 0x280000 | multiboot header + <br>compressed pano_ldr bitstream  | 2560k|
| 0x280000 -> 0x2bffff | pano_ldr data                  |  256k |
| 0x2c0000 -> 0x6fffff | user bitstream (worse case)    | 4122k |
| 0x700000 -> 0xffffff | spiffs                         | 9216k |

As another example multiple applications could be stored.

|     Byte address     |                   usage        | size  |
|----------------------|--------------------------------|-------|
| 0x000000 -> 0x280000 | multiboot header + <br>compressed pano_ldr bitstream  | 2560k|
| 0x280000 -> 0x2bffff | pano_ldr data                  |  256k |
| 0x2c0000 -> 0x?????  | user bitstream #1              | ????k |
| 0x?????? -> 0x?????  | user bitstream #2              | ????k |
| 0x?????? -> 0x?????  | user bitstream #3              | ????k |
| 0x?????? -> 0x?????  |            ...                 | ????k |
| 0x?????? -> 0xffffff | spiffs                         | ????k |


## Second Generation rev B (G2) SPI memory map

This device's flash is a 16 megabyte Numonyx M25P128 with a minimum erase size of 256k bytes.

This device's FPGA is a xc6ls150.  The uncompressed bitstream size is 4220212 (0x406534) 
bytes which requires 17 erase sectors.  


|     Byte address     |        usage        | size     | notes |
|----------------------|---------------------|----------|-------|
| 0x000000 -> 0x000034 | multiboot header    | 52 bytes |       |
| 0x000035 -> 0x03ffff | unused              | 255k     |   1   |
| 0x040000 -> 0x446534 | golden bitstream    | 4122k    |       |
| 0x446535 -> 0x47ffff | unused              | 230k     |   2   |
| 0x480000 -> 0x886534 | multiboot bitstream | 4122k    |       |
| 0x886535 -> 0x8bffff | unused              | 230k     |   3   |
| 0x8c0000 -> 0x8fffff | MAC adr and unknown | 256k     |   4   |
| 0x900000 -> 0xffffff | unused              | 7167k    |       |

## Second Generation rev C (G2_C) memory map

This device's flash is a 8 megabyte Numonyx M25P64 with a minimum erase size of 64k bytes.

This device's FPGA is a xc6ls100.  The uncompressed bitstream size is 3317908 (0x32A094)
bytes which requires 51 erase sectors.  

|     Byte address     |        usage        | size     | notes |
|----------------------|---------------------|----------|-------|
| 0x000000 -> 0x000034 | multiboot header    | 52 bytes |       |
| 0x000035 -> 0x00ffff | unused              | 63k      |   1   |
| 0x020000 -> 0x03ffff | unused              | 128k     |       |
| 0x040000 -> 0x36a094 | golden bitstream    | 3240k    |       |
| 0x36a095 -> 0x36ffff | unused              | 23k      |   2   |
| 0x370000 -> 0x37ffff | unused              | 64k      |       |
| 0x380000 -> 0x6aa094 | multiboot bitstream | 3240k    |       |
| 0x6aa094 -> 0x6affff | unused              | 23k      |   3   |
| 0x6b0000 -> 0x6bffff | MAC adr and unknown | 64k      |   4   |
| 0x6c0000 -> 0x7fffff | unused/unknown      | 1280k    |       |

## SPI Flash Memory Map Notes:
1. These unused bytes are located within the multiboot header erase sector.
2. These unused bytes are located within a golden bitstream erase sector.
3. These unused bytes are located within a multiboot bitstream erase sector.
4. This region contains 232 bytes of mostly unknown data.  The last 3 bytes of 
the devices MAC address appear to be at offset 0xd in the block.   


To program the SPI flash in the Pano and/or to run this project you DO NOT 
need Xilinx's ISE.

If you would like to modify the firmware you'll also need gcc built for 
RISC-V RV32IM.

If you would like to modify the RTL you'll also need Xilinx ISE 14.7.

The free Webpack version of Xilinx [ISE 14.7](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/archive-ise.html) is used for development.
Download the latest Windows 10 version which supports the Spartan 6 family of 
chips used in the second generation Panologic device.

## Serial port 

A serial port is required for debugging.

Please see the [fpga_test_soc](https://github.com/skiphansen/fpga_test_soc/tree/master/fpga/panologic_g2#serial-port) for connection information.

If the serial port you use to interface to the Pano is not /dev/ttyUSB1 then
you will need to set the "TARGET_PORT" environment variable to point
to your serial device.

Ultraembedded's SOC platform includes the ability to load firmware over a 
serial port which is VERY HANDY for code development.


### Running

1. If necessary set the "PLATFORM", "CABLE" and "TARGET_PORT" environment 
   variables as needed for your environment (see above).

2. Run "make load" to load the bitstream into the Pano.

tftp_loader should obtain an IP address from an DHCP on your local LAN.
It will also respond to pings.

```
```

### Ethernet Status lights

Note:  The RTL handles Ethernet the link configuration negotiation and it only
allow full duplex connections at 1000BaseT, 100BaseT and 10BaseT.  MDI/MDIX
configuration is automatic.  Links WILL NOT be established with half duplex
hubs/switches (this really shouldn't be an issue).

Looking at the Pano's Ethernet port with the tab up the right LED shows the
link state

Off - No Link
Green - 10BaseT connection
Green/Amber - 100BaseT connection
Amber - 1000BaseT connection


### Building everything from sources

**NB:** While it may be possible to use Windows for development I haven't 
tried it and don't recommend it.

1. Clone the https://github.com/skiphansen/panog2_ldr repository
2. cd into .../panog2_ldr
3. Make sure the RISC-V GCC (built for RV32IM) is in the PATH.
4. Run "make build_all" or "make PLATFORM=pano-g2-c build_all".


## Acknowledgement and Thanks
This project uses code from several other projects including:
 - (ultraembedded's fpga_test_soc)[https://github.com/ultraembedded/fpga_test_soc.git]
 - (Yol's Ethernet MAC)[https://github.com/yol/ethernet_mac.git]
 - (The Light Weight IP project)[git://git.savannah.gnu.org/lwip.git]

## Pano Links                          

Links to other Pano logic information can be found on the 
[Pano Logic Hackers wiki](https://github.com/tomverbeure/panologic-g2/wiki)

## LEGAL 

My original work (the Pano ethernet_mac glue code) is released under the 
GNU General Public License, version 2.

## Auto boot

Once pano_ldr has been installed it is the default bistream that is loaded 
following power on or reset.  With the default configuration pano_ldr will 
initialize the network and then wait for a telnet connection.

Once an applcation has been installed pano_ldr can be configured to start
the application automatcally (auto boot) if desired.  

When pano_ldr is configured for auto boot instead of initializing the 
network and waiting for a telnet connection it checks the Pano button and if
it is **NOT** pressed it immediately boots the usr application.

The _auto_ and _bootadr_ commands are used to configure auto boot mode.

## Auto [on | off]

The auto command turns the auto boot mode on an off.  This command requires
that the last sector of flash 
can be used to configure Pano_ldr to automatically boot another
bitstream without command allows 

## Bootadr [\<flash adr>]

The bootadr command displays or sets the address of the auto boot bitstream.

If the Pano button is NOT pressed then pano_ldr simply causes the "Golden"
bistream to be loaded and started.  This prevents the need to telnet into
the loader to start the user's application every time the Pano is turned on.

If the Pano button IS pressed then pano_ldr initializes the Ethernet port, 
obtains an IP address via DHCP and then waits for a telnet connection.

## Configuration 
