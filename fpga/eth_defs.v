//-----------------------------------------------------------------
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

`define ETH_RX      8'h0

    `define ETH_RX_DATA_DEFAULT    0
    `define ETH_RX_DATA_B          0
    `define ETH_RX_DATA_T          7
    `define ETH_RX_DATA_W          8
    `define ETH_RX_DATA_R          7:0

`define ETH_TX    8'h4

    `define ETH_TX_DATA_DEFAULT    0
    `define ETH_TX_DATA_B          0
    `define ETH_TX_DATA_T          7
    `define ETH_TX_DATA_W          8
    `define ETH_TX_DATA_R          7:0

`define ETH_STATUS  8'h8

    `define ETH_STATUS_LINK_SPEED_R  7:6
    `define ETH_STATUS_LINK_UP_R     5:5

    `define ETH_STATUS_IE      4
    `define ETH_STATUS_IE_DEFAULT    0
    `define ETH_STATUS_IE_B          4
    `define ETH_STATUS_IE_T          4
    `define ETH_STATUS_IE_W          1
    `define ETH_STATUS_IE_R          4:4

    `define ETH_STATUS_TXFULL      3
    `define ETH_STATUS_TXFULL_DEFAULT    0
    `define ETH_STATUS_TXFULL_B          3
    `define ETH_STATUS_TXFULL_T          3
    `define ETH_STATUS_TXFULL_W          1
    `define ETH_STATUS_TXFULL_R          3:3

    `define ETH_STATUS_RXEMPTY      2
    `define ETH_STATUS_RXEMPTY_DEFAULT    0
    `define ETH_STATUS_RXEMPTY_B          2
    `define ETH_STATUS_RXEMPTY_T          2
    `define ETH_STATUS_RXEMPTY_W          1
    `define ETH_STATUS_RXEMPTY_R          2:2

    `define ETH_STATUS_RXRESET      1
    `define ETH_STATUS_RXRESET_DEFAULT    0
    `define ETH_STATUS_RXRESET_B          1
    `define ETH_STATUS_RXRESET_T          1
    `define ETH_STATUS_RXRESET_W          1
    `define ETH_STATUS_RXRESET_R          1:1

    `define ETH_STATUS_TXRESET      0
    `define ETH_STATUS_TXRESET_DEFAULT    0
    `define ETH_STATUS_TXRESET_B          0
    `define ETH_STATUS_TXRESET_T          0
    `define ETH_STATUS_TXRESET_W          1
    `define ETH_STATUS_TXRESET_R          0:0



// Speed constants
`define SPEED_1000MBPS      2'b10
`define SPEED_100MBPS       2'b01
`define SPEED_10MBPS        2'b00
`define SPEED_UNSPECIFIED   2'b11

