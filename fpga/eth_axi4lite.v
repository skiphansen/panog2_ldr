//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
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

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

`include "eth_defs.v"

module eth_axi4lite
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
// Change this if you like, this was copied from the label on one of Skip's G2s
    parameter MAC_ADDRESS = 48'h5d1d70021c00
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // axi4 Inputs
     input          clk_i   // 32 Mhz
    ,input          rst_i
    ,input          cfg_awvalid_i
    ,input  [31:0]  cfg_awaddr_i
    ,input          cfg_wvalid_i
    ,input  [31:0]  cfg_wdata_i
    ,input  [3:0]   cfg_wstrb_i
    ,input          cfg_bready_i
    ,input          cfg_arvalid_i
    ,input  [31:0]  cfg_araddr_i
    ,input          cfg_rready_i

    // axi4 Outputs
    ,output         cfg_awready_o
    ,output         cfg_wready_o
    ,output         cfg_bvalid_o
    ,output [1:0]   cfg_bresp_o
    ,output         cfg_arready_o
    ,output         cfg_rvalid_o
    ,output [31:0]  cfg_rdata_o
    ,output [1:0]   cfg_rresp_o

   // peripheral inputs
  // Unbuffered 125 MHz clock input
    ,input          clock_125_i

    // MII (Media-independent interface)
    ,input         mii_tx_clk_i
    ,output        mii_tx_er_o
    ,output        mii_tx_en_o
    ,output [7:0]  mii_txd_o
    ,input         mii_rx_clk_i
    ,input         mii_rx_er_i
    ,input         mii_rx_dv_i
    ,input [7:0]   mii_rxd_i

    // GMII (Gigabit media-independent interface)
    ,output        gmii_gtx_clk_o

    // RGMII (Reduced pin count gigabit media-independent interface)
    ,output        rgmii_tx_ctl_o
    ,input         rgmii_rx_ctl_i

    // MII Management Interface
    ,output        mdc_o
    ,inout         mdio_io
);

//-----------------------------------------------------------------
// Write address / data split
//-----------------------------------------------------------------
// Address but no data ready
reg awvalid_q;

// Data but no data ready
reg wvalid_q;

wire wr_cmd_accepted_w  = (cfg_awvalid_i && cfg_awready_o) || awvalid_q;
wire wr_data_accepted_w = (cfg_wvalid_i  && cfg_wready_o)  || wvalid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    awvalid_q <= 1'b0;
else if (cfg_awvalid_i && cfg_awready_o && !wr_data_accepted_w)
    awvalid_q <= 1'b1;
else if (wr_data_accepted_w)
    awvalid_q <= 1'b0;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wvalid_q <= 1'b0;
else if (cfg_wvalid_i && cfg_wready_o && !wr_cmd_accepted_w)
    wvalid_q <= 1'b1;
else if (wr_cmd_accepted_w)
    wvalid_q <= 1'b0;

//-----------------------------------------------------------------
// Capture address (for delayed data)
//-----------------------------------------------------------------
reg [7:0] wr_addr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wr_addr_q <= 8'b0;
else if (cfg_awvalid_i && cfg_awready_o)
    wr_addr_q <= cfg_awaddr_i[7:0];

wire [7:0] wr_addr_w = awvalid_q ? wr_addr_q : cfg_awaddr_i[7:0];
wire [47:0] mac_address = MAC_ADDRESS;
wire rx_reset;
wire rx_empty;
reg rx_rd_en;
wire [7:0] rx_data;
wire [7:0] tx_data;
reg tx_wr_en;
wire tx_full;
wire tx_reset;
wire link_up;
wire [1:0] link_speed;


//-----------------------------------------------------------------
// Retime write data
//-----------------------------------------------------------------
reg [31:0] wr_data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wr_data_q <= 32'b0;
else if (cfg_wvalid_i && cfg_wready_o)
    wr_data_q <= cfg_wdata_i;

//-----------------------------------------------------------------
// Request Logic
//-----------------------------------------------------------------
wire read_en_w  = cfg_arvalid_i & cfg_arready_o;
wire write_en_w = wr_cmd_accepted_w && wr_data_accepted_w;

//-----------------------------------------------------------------
// Accept Logic
//-----------------------------------------------------------------
assign cfg_arready_o = ~cfg_rvalid_o;
assign cfg_awready_o = ~cfg_bvalid_o && ~cfg_arvalid_i && ~awvalid_q;
assign cfg_wready_o  = ~cfg_bvalid_o && ~cfg_arvalid_i && ~wvalid_q;


//-----------------------------------------------------------------
// Register ETH_tx
//-----------------------------------------------------------------

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_wr_en <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `ETH_TX))
    tx_wr_en <= 1'b1;
else
    tx_wr_en <= 1'b0;

assign tx_data = wr_data_q[`ETH_TX_DATA_R];


//-----------------------------------------------------------------
// Register ETH_status
//-----------------------------------------------------------------
reg eth_status_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    eth_status_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `ETH_STATUS))
    eth_status_wr_q <= 1'b1;
else
    eth_status_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Read mux
//-----------------------------------------------------------------
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;

    case (cfg_araddr_i[7:0])

    `ETH_RX:
    begin
        data_r[`ETH_RX_DATA_R] = rx_data;
    end
    `ETH_STATUS:
    begin
        data_r[`ETH_STATUS_IE_R] = 1'b0;
        data_r[`ETH_STATUS_TXFULL_R] = tx_full;
        data_r[`ETH_STATUS_RXEMPTY_R] = rx_empty;
        data_r[`ETH_STATUS_RXRESET_R] = rx_reset;
        data_r[`ETH_STATUS_TXRESET_R] = tx_reset;
        data_r[`ETH_STATUS_LINK_UP_R] = link_up;
        data_r[`ETH_STATUS_LINK_SPEED_R] = link_speed;
    end
    default :
        data_r = 32'b0;
    endcase
end

//-----------------------------------------------------------------
// RVALID
//-----------------------------------------------------------------
reg rvalid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rvalid_q <= 1'b0;
else if (read_en_w)
    rvalid_q <= 1'b1;
else if (cfg_rready_i)
    rvalid_q <= 1'b0;

assign cfg_rvalid_o = rvalid_q;

//-----------------------------------------------------------------
// Retime read response
//-----------------------------------------------------------------
reg [31:0] rd_data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rd_data_q <= 32'b0;
else if (!cfg_rvalid_o || cfg_rready_i)
    rd_data_q <= data_r;

assign cfg_rdata_o = rd_data_q;
assign cfg_rresp_o = 2'b0;

//-----------------------------------------------------------------
// BVALID
//-----------------------------------------------------------------
reg bvalid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    bvalid_q <= 1'b0;
else if (write_en_w)
    bvalid_q <= 1'b1;
else if (cfg_bready_i)
    bvalid_q <= 1'b0;

assign cfg_bvalid_o = bvalid_q;
assign cfg_bresp_o  = 2'b0;

ethernet_with_fifos 
#(
    .MIIM_CLOCK_DIVIDER(13) // 32Mhz / 13 = about 2.5 Mhz
)
eth_u (
 // Unbuffered 125 MHz clock input
    .clock_125_i(clock_125_i)
 // Asynchronous reset
    ,.reset_i(rst_i)
 // MAC address of this station
 // Must not change after reset is deasserted
    ,.mac_address_i(mac_address)

 // MII (Media-independent interface)
    ,.mii_tx_clk_i(mii_tx_clk_i)
    ,.mii_tx_er_o(mii_tx_er_o)
    ,.mii_tx_en_o(mii_tx_en_o)
    ,.mii_txd_o(mii_txd_o)
    ,.mii_rx_clk_i(mii_rx_clk_i)
    ,.mii_rx_er_i(mii_rx_er_i)
    ,.mii_rx_dv_i(mii_rx_dv_i)
    ,.mii_rxd_i(mii_rxd_i)

 // GMII (Gigabit media-independent interface)
    ,.gmii_gtx_clk_o(gmii_gtx_clk_o)
 // RGMII (Reduced pin count gigabit media-independent interface)
    ,.rgmii_tx_ctl_o(rgmii_tx_ctl_o)
    ,.rgmii_rx_ctl_i(rgmii_rx_ctl_i)

// MII Management Interface
    ,.mdc_o(mdc_o)
    ,.mdio_io(mdio_io)
    // Status, synchronous to clk_i
    ,.link_up_o(link_up)
    ,.speed_o(link_speed)

 // MII Management Interface
 // Clock, can be identical to clock_125_i
 // If not, adjust MIIM_CLOCK_DIVIDER accordingly
    ,.miim_clock_i(clk_i)

 // Also synchronous to miim_clock_i if used!
    ,.speed_override_i(`SPEED_UNSPECIFIED)

 // RX FIFO
    ,.rx_clock_i(clk_i)
 // Synchronous reset
 // When asserted, the content of the buffer was lost.
 // When empty is deasserted the next time, a packet size must be read out.
 // The data of the packet previously being read out is not available anymore then.
    ,.rx_reset_o(rx_reset)
    ,.rx_empty_o(rx_empty)
    ,.rx_rd_en_i(rx_rd_en)
    ,.rx_data_o(rx_data)
 // TX FIFO
    ,.tx_clock_i(clk_i)
 // Synchronous reset
 // When asserted, the content of the buffer was lost.
 // When full is deasserted the next time, a packet size must be written.
 // The data of the packet previously being written is not available anymore then.
    ,.tx_reset_o(tx_reset)
    ,.tx_data_i(tx_data)
    ,.tx_wr_en_i(tx_wr_en)
    ,.tx_full_o(tx_full)
);


always @ (posedge clk_i)
    if (read_en_w) begin
        if(cfg_araddr_i[7:0] == `ETH_RX) begin
        // Read fifo
            rx_rd_en <= 1'b1;
        end
    end
    else begin
        rx_rd_en <= 1'b0;
    end


endmodule
