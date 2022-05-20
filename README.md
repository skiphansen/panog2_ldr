# Pano_ldr

[https://github.com/skiphansen/panog2_ldr](https://github.com/skiphansen/panog2_ldr)

The purpose of this project is to allow a user to flash and run their own
projects on [Pano G2](https://github.com/tomverbeure/panologic-g2/wiki/Identifying-different-Pano-generations-and-revisions#second-generation-rev-b-g2_b)
and [Fujitsu DZ22-2](https://github.com/tomverbeure/panologic-g2/wiki/Identifying-different-Pano-generations-and-revisions#fujitsu-zero-client-dz22-2)
thin clients without a JTAG programer. 

Unlike my [pano_progfpga](https://github.com/skiphansen/pano_progfpga) project
pano_ldr is a bitstream for the Pano which can be co-resident with the user's
bitstream. This means that pano_ldr may be used to update the Pano over and
over as many times as desired.

It is possible to install pano_ldr on a factory stock Pano G2 using [pano_progfpga](https://github.com/skiphansen/pano_progfpga)
without opening the case.  This is particularly important for owners of the 
DZ22-2 since it's almost impossible to open the case without damage to the 
case, the user or both.

Once pano_ldr has been installed the user can telnet into the Pano and issue 
commands to update the flash and run applications.

Unlike Pano's original progfpga utility pano_ldr is fast, limited only by the 
speed of the flash chip itself.  Flashing with pano_ldr over the network is just 
as fast or faster than as using a JTAG programmer.

# Usage

Pano_ldr provides the user with a command line interface (CLI) via a 
[telnet](https://en.wikipedia.org/wiki/Telnet) connection.  Commands are 
provided for reading and writing flash as well as configuring Pano_ldr to
automatically boot the user's application on power on or reset.

Files are transferred to and from a specified TFTP server on your network.

```
skip@Dell-7040:~/pano/working/panog2_ldr$ telnet pano_ldr
Trying 192.168.123.196...
Connected to pano_ldr.lan.
Escape character is '^]'.
Pano LDR v0.01 compiled May 19 2022 16:22:37
ldr> help
Commands:
  autoboot  - [ on | off]
  autoerase - [ on | off]
  bootadr   - <flash adr>
  dump      - <flash adr> <length>
  erase     - <start adr> <end adr>
  flash     - <filename> <flash adr>
  map       - Display blank regions in flash
  reboot    - <flash adr>
  save      - <filename> <flash adr> <length>
  tftp      - <IP adr of tftp server>
  verify    - <filename> <flash adr>
ldr> flash my_project.bin 0x40000
..............................
flashed 1974219 bytes
ldr>

```
# TFTP server

There are MANY choices for a TFTP servers, personally I use tftpd-hpa on 
Ubuntu 20.04.  

Please refer to the Internet for instructions on how to install and configure 
a TFTP server of your choice.

# Auto boot

Once pano_ldr has been installed it is the default bitstream that is loaded 
following power on or reset.  With the default configuration pano_ldr will 
initialize the network and then wait for a telnet connection.

Once an application has been installed pano_ldr can be configured to start
the application automatically (autoboot) if desired.  

When pano_ldr is configured for autoboot instead of initializing the 
network and waiting for a telnet connection it checks if the Pano button is 
pressed.  If the button is **NOT** pressed pano_ldr will immediately reload
the application bitstream, otherwise it will enter normal operation.

The autoboot and bootadr commands are used to configure the auto bootmode.

# Pano_ldr Commands

All arguments can be specified in either decimal or hex.  For hex prefix the hex value
with '0x'.  To save typing commands may be abbreviated.

The "help" command lists available commands for reference.

## Autoboot [on | off]

The autoboot command displays or turns the autoboot feature ON and OFF.

For example:
```
ldr> autoboot
Autoboot off
ldr> autoboot on
Autoboot on
ldr> autoboot off
Autoboot off
ldr>
```

DZ22-2 note:  Do not turn ON autoboot for on the DZ22-2, it is unknown
how the "Pano button" is handled on the DZ22.

## Autoerase [on | off]

The autoerase command displays or turns the autoerase feature ON and OFF.
By default autoerase is ON allowing the flash command to erase flash as needed.

By turning autoerase OFF it is possible to flash multiple data blocks into the
one erase block.  For example firmware for a project might be stored past 
the end of a bitimage but before the end of the erase block to save some space.

Another usage would be to flash a multiboot header along with pano_ldr at
the beginning of flash to save almost 256K of flash on a Rev B or almost
64k on a Rev C.

**NB:** It is strongly advised that the verify command be used after 
flashing with autoerase turned OFF to ensure that flash is in the desired state.
Flash write operations can only turn '1' bits into '0' so programming the
same byte with conflicting value **WILL** result in errors.

**NB:** Even if flash is completely corrupted it is always possible to fix it
as long as you have backups of the data and you have not power cycled the Pano.
If you power cycle the Pano while flash is corrupted enough to prevent pano_ldr
or the "Golden" from starting you will need to use a JTAG programmer to recover.

For example:
```
ldr> autoerase
AutoErase on
ldr> autoerase off
AutoErase off
ldr>
```

## Bootadr [\<flash adr>]

The bootadr command displays or sets the address of the auto boot bitstream.
By default bootadr is set to 0x40000 which is the location of the "golden"
image on a stock Pano.

For example:
```
ldr> bootadr
Autoboot @0x40000
ldr> bootadr 0x380000
ldr> bootadr
Autoboot @0x380000
ldr>
```

## Dump \<flash adr> \<length>

The dump command displays the specified range of flash in hex.

For example:
```
ldr> dump 0x40000 128
00040000  ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
00040010  aa 99 55 66 30 a1 00 07 20 00 31 a1 09 60 31 41
00040020  3d 00 31 61 09 ee 31 c2 04 01 d0 93 30 e1 00 cf
00040030  30 c1 00 81 20 00 20 00 20 00 20 00 20 00 20 00
00040040  20 00 20 00 20 00 20 00 20 00 20 00 20 00 20 00
00040050  20 00 20 00 20 00 33 81 3c 18 31 81 08 81 34 21
00040060  00 00 32 01 00 1f 31 e1 ff ff 33 21 00 05 33 41
00040070  00 04 33 01 01 00 32 61 00 00 32 81 00 00 32 a1
ldr>
```

## Erase  \<start adr> \<end adr>

The erase command erases the specified range of flash.  This command does not
need be used before using the flash command unless autoerase is turned OFF.

The start address must be on an [erase boundary](https://github.com/skiphansen/panog2_ldr/edit/master/README.md#erase-boundaries)
and the end address must be one byte less than the next erase boundary.  

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

## Flash \<filename> \<flash adr>

The flash command programs flash with the specified file from the TFTP server.
The start address must be on an [erase boundary](https://github.com/skiphansen/panog2_ldr/edit/master/README.md#erase-boundaries)
unless autoerase is turned OFF.

If autoerase is on flash is erased automatically as needed.

For compatibility with the stock Pano flash partitioning user applications
can be flashed as the "golden" bitstream at address 0x40000.

Please see [SPI-Flash-memory-maps](https://github.com/tomverbeure/panologic-g2/wiki/SPI-Flash-memory-maps)
for details.

For example:
```
ldr> flash pano-g2-c.bin 0x40000
..............................

flashed 1974219 bytes
ldr>
```

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

## Reboot \<flash adr>

The reboot command initiates a FPGA reconfiguration using the bitstream at the 
specified address.

For example:
```
ldr> reboot 0x380000
Booting bitstream @ 0x380000
```

## save \<filename> \<flash adr> \<length>

The save command saves a region of flash as a file on the TFTP sever.

For example:
```
ldr> save pano-g2-c.bin 0x40000 1974219
..............................
Saved 1974219 bytes
ldr>
```

## tftp [\<IP adr>]

The tftp command is used display or set the IP address of the TFTP server.  
The TFTP server MUST be on the same subnet as the Pano.  The TFTP server address 
is saved in flash.

Note:  By default the TFTP server address is set to the IP address of the first
telnet connection.  It is only necessary to set the TFTP server address if the
TFTP server is not on the same machine which is used to telnet in the loader.

For example:
```
ldr> tftp
192.168.123.170
ldr> tftp 1.2.3.4
ldr> tftp
1.2.3.4
ldr>
```

## verify \<filename> \<flash adr>

The verify command compares flash with the specified file from the TFTP server.

For example:
```
ldr> verify pano-g2-c.bit 0x40000
..............................
1974219 bytes verified
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
a JTAG programmer connected. 

## Installation without a JTAG programmer

A script that uses Pano update utilities which have been patched by the 
[pano_progfpga](https://github.com/skiphansen/pano_progfpga) project is
provide to install pano_ldr of the network.  The Pano update utilities are 32 
bit Linux x86 programs that can be run on  any OS that can run 32 bit Linux binaries.  

You should be able to run these programs on a modern X86 Linux by installing
[32 bit libraries](https://linuxconfig.org/unable-to-find-a-suitable-destination-to-install-32-bit-compatibility-libraries-on-ubuntu-18-04-bionic-beaver-linux).
Alternately they can be run in a VM running a 32 bit version of Linux. 
If you want to go the VM route start [here](https://www.youtube.com/watch?v=DPkF5EisGDQ), or create a 32 bit Linux VM of your choice.

The pano_ldr is installed as the "multiboot bitstream" so unexpected power
failures during the update should **NOT** brick the Pano.

The installation procedure is:

* Plug the Pano into your local LAN.
* Turn it on.
* Wait for it to obtain an IP address via DHCP
* Run install_pano_ldr.sh
* Verify that you want to proceed
* Cross your fingers and wait as the Pano is updated.  (Go get coffee, you have time)
* Power cycle the Pano 

Connect the Pano to your LAN then power it on.  Wait for the Pano button to
turn amber and stop flashing before running the install script.

It takes about 6 minutes to install pano_ldr on a G2 Rev B and about 4 minutes
on a Rev C.

```
skip@Dell-7040:~/pano/working/panog2_ldr$ ./install_pano_ldr.sh 
Pano IP: 192.168.123.118

Install pano_ldr on Rev B Pano G2 (y/n) ?y
answer: 'y'
Running Test = flashFPGA_Series2_Multiboot
Client connected : IP Addr = 192.168.123.118:8321
READ CFG reg 0: 0x08010000
TESTING with board Type = SERIES_II
FPGA Major Rev = 0801, Minor Rev = 0014 
SPI ERASE SECTOR 00120000 
Flash type : LX150look for lx150/multiboot.9nimgSPI ERASE SECTOR 00124000 
SPI ERASE SECTOR 00128000 
SPI ERASE SECTOR 0012c000 
...
SPI ERASE SECTOR 0022c000 
Erase took 204.816864 seconds
finish writing addr=0x00123fc0, 0x00004000 words and 1 sectors
finish writing addr=0x00127fc0, 0x00008000 words and 2 sectors
...
finish writing addr=0x0021ffc0, 0x00100000 words and 64 sectors
Writing the Start writing multiboot image! consumed 89.843155 seconds
reading flash & verifing 
Test has PASSED 
Disconnecting audio...
Start writing multiboot image! Image Verified, validation consumed 154.612976 seconds
Disconnected
Pano_ldr installed, power cycle the Pano to start it
skip@Dell-7040:~/pano/working/panog2_ldr$ 
```


After power cycling the Pano, pano_ldr should run and initialize the network.

The Pano button's LEDs are used to show pano_ldr's network state.

| State | LED |
| - | - |
| Ethernet link down | Flashing red |
| Ethernet link up, IP address not assigned | Flashing blue |
| IP address assigned by DHCP, not connected | Flashing green |
| User connected | Solid green |

Once the Pano button begins blinking green you should be able to telnet into
pano_ldr.

DZ22-2 note: The LED on the DZ22-2 is not directly controlled by the Pano so no useful
status will be displayed.

## Determine the Pano's IP address

Pano_ldr's MAC address is 00:1c:02:70:1d:5d. Since this MAC address is 
different that the MAC address used by the stock bitstream a new IP address 
should be assigned to the Pano after it reboots.

The DHCP client provides a host name of "pano_ldr" to the DHCP server
when an IP address is requested.  IF your router provides DNS service for 
local clients you should be able to telnet into your pano by host name. 
I use an OpenWRT based router and it provides this service.

If your router doesn't provide DNS for local clients you will need to use 
determine the Pano's IP address manually.  All routers should have some way
of viewing active DHCP leases.  

## Installation using a JTAG programmer

If already know how to flash .bit files you can find them in the prebuilt 
subdirectory, otherwise read on.

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

## Erase Boundaries

Flash chips can be read and written in single byte increments, however they
can only be erased in blocks.  

As a consequence of this the flash commands and erase commands must start
on an erase boundary

| Pano | flash size | erase block size | number of blocks |
| - | - | - | - |
| rev B | 16 MBytes | 256k | 64 |
| rev C | 8 MBytes | 64k | 128 |

## Rev B erase blocks

| Block number | Address range |
| - | - |
| 0 | 0x000000 -> 0x03ffff |
| 1 | 0x040000 -> 0x07ffff |
| 2 | 0x080000 -> 0x0bffff |
| ... |
| 62 | 0xf80000 -> 0xfbffff |
| 63 | 0xfc0000 -> 0xffffff |

## Rev C erase blocks

| Block number | Address range |
| - | - |
| 0 | 0x000000 -> 0x00ffff |
| 1 | 0x010000 -> 0x01ffff |
| 2 | 0x020000 -> 0x02ffff |
| ... |
| 126 | 0x7e0000 -> 0x7effff |
| 127 | 0x7f0000 -> 0x7fffff |

# Building everything from sources

**NB:** While it may be possible to use Windows for development I haven't 
tried it and don't recommend it.

1. Clone the https://github.com/skiphansen/panog2_ldr repository
2. cd into .../panog2_ldr
3. Make sure the RISC-V GCC (built for RV32IM) is in the PATH.
4. Run "make build_all" or "make PLATFORM=pano-g2-c build_all".

The output from the build will be in the prebuilt subdirectory name
pano-g2.bit or pano-g2-c.bit.

## Serial port (Optional)

Pano_ldr is based on Ultraembedded's SOC platform which includes the ability to 
load firmware over a serial port which is VERY HANDY for code development.  I 
strongly suggest building a serial cable to allow the capability to be used if
you are interested in modifying the firmware.

Please see the [fpga_test_soc](https://github.com/skiphansen/fpga_test_soc/tree/master/fpga/panologic_g2#serial-port) for more information.

# Acknowledgement and Thanks
This project uses code from several other projects including:
 - [ultraembedded's fpga_test_soc](https://github.com/ultraembedded/fpga_test_soc.git)
 - [Yol's Ethernet MAC](https://github.com/yol/ethernet_mac.git)
 - [The Light Weight IP project](https://savannah.nongnu.org/projects/lwip)

# Pano Links                          

Links to other Pano logic information can be found on the 
[Pano Logic Hackers wiki](https://github.com/tomverbeure/panologic-g2/wiki)

# LEGAL 

My original work (the Pano ethernet_mac glue code and pano_ldr firmware) is 
released under the GNU General Public License, version 2.

