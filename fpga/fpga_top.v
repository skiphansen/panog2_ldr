//-----------------------------------------------------------------
//                        FPGA Test Soc
//                           V0.1
//                     Ultra-Embedded.com
//                       Copyright 2019
//
//                 Email: admin@ultra-embedded.com
//
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

module fpga_top
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter CLK_FREQ         = 50000000
    ,parameter BAUDRATE         = 1000000
    ,parameter UART_SPEED       = 1000000
    ,parameter C_SCK_RATIO      = 50
    ,parameter CPU              = "riscv" // riscv or armv6m
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           dbg_txd_i
    ,input           spi_miso_i
    ,input           uart_rx_i
    ,input  [ 31:0]  gpio_input_i
    ,input           clock_125_i

    // Outputs
    ,output          dbg_rxd_o
    ,output          spi_clk_o
    ,output          spi_mosi_o
    ,output [  7:0]  spi_cs_o
    ,output          uart_tx_o
    ,output [ 31:0]  gpio_output_o
    ,output [ 31:0]  gpio_output_enable_o
    ,output [ 23:0]  boot_spi_adr_o
    ,output          reboot_o

`ifdef INCLUDE_ETHERNET
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
`endif
     
// UTMI Interface
    ,input  [  7:0]  utmi_data_out_i
    ,input  [  7:0]  utmi_data_in_i
    ,input           utmi_txvalid_i
    ,input           utmi_txready_i
    ,input           utmi_rxvalid_i
    ,input           utmi_rxactive_i
    ,input           utmi_rxerror_i
    ,input  [  1:0]  utmi_linestate_i

    ,output [  1:0]  utmi_op_mode_o
    ,output [  1:0]  utmi_xcvrselect_o
    ,output          utmi_termselect_o
    ,output          utmi_dppulldown_o
    ,output          utmi_dmpulldown_o
);

wire  [  3:0]  axi_t_awid_w;
wire           axi_l_awvalid_w;
wire           axi_l_arvalid_w;
wire  [  1:0]  axi_i_bresp_w;
wire  [  1:0]  axi_l_bresp_w;
wire  [ 31:0]  axi_t_wdata_w;
wire           axi_t_rlast_w;
wire  [  3:0]  axi_i_wstrb_w;
wire  [ 31:0]  axi_t_rdata_w;
wire           axi_t_bvalid_w;
wire           axi_t_awready_w;
wire           axi_l_wvalid_w;
wire  [  3:0]  axi_t_arid_w;
wire  [ 31:0]  axi_t_awaddr_w;
wire  [  1:0]  axi_i_rresp_w;
wire  [  7:0]  axi_t_arlen_w;
wire           axi_l_bvalid_w;
wire           axi_i_wlast_w;
wire           axi_i_arready_w;
wire           axi_t_wvalid_w;
wire  [ 31:0]  axi_t_araddr_w;
wire  [  3:0]  axi_i_bid_w;
wire  [  1:0]  axi_t_rresp_w;
wire           axi_l_awready_w;
wire           axi_t_wlast_w;
wire  [ 31:0]  axi_l_rdata_w;
wire  [  1:0]  axi_i_awburst_w;
wire           axi_t_rvalid_w;
wire           axi_i_rvalid_w;
wire           axi_t_arvalid_w;
wire           axi_t_arready_w;
wire  [  1:0]  axi_i_arburst_w;
wire           axi_t_awvalid_w;
wire  [  7:0]  axi_i_arlen_w;
wire           axi_l_rready_w;
wire  [  1:0]  axi_l_rresp_w;
wire  [ 31:0]  axi_i_rdata_w;
wire           axi_i_rlast_w;
wire  [ 31:0]  cpu_intr_w;
wire  [  3:0]  axi_i_awid_w;
wire  [ 31:0]  axi_l_awaddr_w;
wire  [  7:0]  axi_t_awlen_w;
wire           axi_i_wready_w;
wire  [ 31:0]  axi_l_wdata_w;
wire  [ 31:0]  enable_w;
wire  [ 31:0]  axi_l_araddr_w;
wire  [  3:0]  axi_l_wstrb_w;
wire           soc_intr_w;
wire  [  1:0]  axi_t_arburst_w;
wire  [  1:0]  axi_t_awburst_w;
wire  [  3:0]  axi_i_rid_w;
wire  [  7:0]  axi_i_awlen_w;
wire           axi_l_wready_w;
wire           axi_i_arvalid_w;
wire           axi_l_bready_w;
wire  [ 31:0]  axi_i_awaddr_w;
wire           axi_t_rready_w;
wire           axi_i_bready_w;
wire  [ 31:0]  axi_i_wdata_w;
wire           axi_t_bready_w;
wire  [  3:0]  axi_i_arid_w;
wire           axi_i_bvalid_w;
wire           axi_l_rvalid_w;
wire  [  3:0]  axi_t_bid_w;
wire           axi_l_arready_w;
wire           axi_i_rready_w;
wire  [  3:0]  axi_t_wstrb_w;
wire  [  1:0]  axi_t_bresp_w;
wire           axi_i_wvalid_w;
wire           axi_i_awready_w;
wire           rst_cpu_w;
wire           axi_i_awvalid_w;
wire  [ 31:0]  axi_i_araddr_w;
wire           axi_t_wready_w;
wire  [  3:0]  axi_t_rid_w;

wire           ext1_cfg_awready_w;
wire           ext1_cfg_wready_w;
wire           ext1_cfg_bvalid_w;
wire  [  1:0]  ext1_cfg_bresp_w;
wire           ext1_cfg_arready_w;
wire           ext1_cfg_rvalid_w;
wire  [ 31:0]  ext1_cfg_rdata_w;
wire  [  1:0]  ext1_cfg_rresp_w;
wire           ext1_irq_w;
wire           ext2_cfg_awready_w;
wire           ext2_cfg_wready_w;
wire           ext2_cfg_bvalid_w;
wire  [  1:0]  ext2_cfg_bresp_w;
wire           ext2_cfg_arready_w;
wire           ext2_cfg_rvalid_w;
wire  [ 31:0]  ext2_cfg_rdata_w;
wire  [  1:0]  ext2_cfg_rresp_w;
wire           ext2_irq_w;
wire           ext3_cfg_awready_w;
wire           ext3_cfg_wready_w;
wire           ext3_cfg_bvalid_w;
wire  [  1:0]  ext3_cfg_bresp_w;
wire           ext3_cfg_arready_w;
wire           ext3_cfg_rvalid_w;
wire  [ 31:0]  ext3_cfg_rdata_w;
wire  [  1:0]  ext3_cfg_rresp_w;
wire           ext3_irq_w;
wire          ext1_cfg_awvalid_w;
wire [ 31:0]  ext1_cfg_awaddr_w;
wire          ext1_cfg_wvalid_w;
wire [ 31:0]  ext1_cfg_wdata_w;
wire [  3:0]  ext1_cfg_wstrb_w;
wire          ext1_cfg_bready_w;
wire          ext1_cfg_arvalid_w;
wire [ 31:0]  ext1_cfg_araddr_w;
wire          ext1_cfg_rready_w;
wire          ext2_cfg_awvalid_w;
wire [ 31:0]  ext2_cfg_awaddr_w;
wire          ext2_cfg_wvalid_w;
wire [ 31:0]  ext2_cfg_wdata_w;
wire [  3:0]  ext2_cfg_wstrb_w;
wire          ext2_cfg_bready_w;
wire          ext2_cfg_arvalid_w;
wire [ 31:0]  ext2_cfg_araddr_w;
wire          ext2_cfg_rready_w;
wire          ext3_cfg_awvalid_w;
wire [ 31:0]  ext3_cfg_awaddr_w;
wire          ext3_cfg_wvalid_w;
wire [ 31:0]  ext3_cfg_wdata_w;
wire [  3:0]  ext3_cfg_wstrb_w;
wire          ext3_cfg_bready_w;
wire          ext3_cfg_arvalid_w;
wire [ 31:0]  ext3_cfg_araddr_w;
wire          ext3_cfg_rready_w;

dbg_bridge
#(
     .CLK_FREQ(CLK_FREQ)
    ,.UART_SPEED(UART_SPEED)
)
u_dbg
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.uart_rxd_i(dbg_txd_i)
    ,.mem_awready_i(axi_t_awready_w)
    ,.mem_wready_i(axi_t_wready_w)
    ,.mem_bvalid_i(axi_t_bvalid_w)
    ,.mem_bresp_i(axi_t_bresp_w)
    ,.mem_bid_i(axi_t_bid_w)
    ,.mem_arready_i(axi_t_arready_w)
    ,.mem_rvalid_i(axi_t_rvalid_w)
    ,.mem_rdata_i(axi_t_rdata_w)
    ,.mem_rresp_i(axi_t_rresp_w)
    ,.mem_rid_i(axi_t_rid_w)
    ,.mem_rlast_i(axi_t_rlast_w)
    ,.gpio_inputs_i(32'b0)

    // Outputs
    ,.uart_txd_o(dbg_rxd_o)
    ,.mem_awvalid_o(axi_t_awvalid_w)
    ,.mem_awaddr_o(axi_t_awaddr_w)
    ,.mem_awid_o(axi_t_awid_w)
    ,.mem_awlen_o(axi_t_awlen_w)
    ,.mem_awburst_o(axi_t_awburst_w)
    ,.mem_wvalid_o(axi_t_wvalid_w)
    ,.mem_wdata_o(axi_t_wdata_w)
    ,.mem_wstrb_o(axi_t_wstrb_w)
    ,.mem_wlast_o(axi_t_wlast_w)
    ,.mem_bready_o(axi_t_bready_w)
    ,.mem_arvalid_o(axi_t_arvalid_w)
    ,.mem_araddr_o(axi_t_araddr_w)
    ,.mem_arid_o(axi_t_arid_w)
    ,.mem_arlen_o(axi_t_arlen_w)
    ,.mem_arburst_o(axi_t_arburst_w)
    ,.mem_rready_o(axi_t_rready_w)
    ,.gpio_outputs_o(enable_w)
);


axi4_axi4lite_conv
u_conv
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.inport_awvalid_i(axi_i_awvalid_w)
    ,.inport_awaddr_i(axi_i_awaddr_w)
    ,.inport_awid_i(axi_i_awid_w)
    ,.inport_awlen_i(axi_i_awlen_w)
    ,.inport_awburst_i(axi_i_awburst_w)
    ,.inport_wvalid_i(axi_i_wvalid_w)
    ,.inport_wdata_i(axi_i_wdata_w)
    ,.inport_wstrb_i(axi_i_wstrb_w)
    ,.inport_wlast_i(axi_i_wlast_w)
    ,.inport_bready_i(axi_i_bready_w)
    ,.inport_arvalid_i(axi_i_arvalid_w)
    ,.inport_araddr_i(axi_i_araddr_w)
    ,.inport_arid_i(axi_i_arid_w)
    ,.inport_arlen_i(axi_i_arlen_w)
    ,.inport_arburst_i(axi_i_arburst_w)
    ,.inport_rready_i(axi_i_rready_w)
    ,.outport_awready_i(axi_l_awready_w)
    ,.outport_wready_i(axi_l_wready_w)
    ,.outport_bvalid_i(axi_l_bvalid_w)
    ,.outport_bresp_i(axi_l_bresp_w)
    ,.outport_arready_i(axi_l_arready_w)
    ,.outport_rvalid_i(axi_l_rvalid_w)
    ,.outport_rdata_i(axi_l_rdata_w)
    ,.outport_rresp_i(axi_l_rresp_w)

    // Outputs
    ,.inport_awready_o(axi_i_awready_w)
    ,.inport_wready_o(axi_i_wready_w)
    ,.inport_bvalid_o(axi_i_bvalid_w)
    ,.inport_bresp_o(axi_i_bresp_w)
    ,.inport_bid_o(axi_i_bid_w)
    ,.inport_arready_o(axi_i_arready_w)
    ,.inport_rvalid_o(axi_i_rvalid_w)
    ,.inport_rdata_o(axi_i_rdata_w)
    ,.inport_rresp_o(axi_i_rresp_w)
    ,.inport_rid_o(axi_i_rid_w)
    ,.inport_rlast_o(axi_i_rlast_w)
    ,.outport_awvalid_o(axi_l_awvalid_w)
    ,.outport_awaddr_o(axi_l_awaddr_w)
    ,.outport_wvalid_o(axi_l_wvalid_w)
    ,.outport_wdata_o(axi_l_wdata_w)
    ,.outport_wstrb_o(axi_l_wstrb_w)
    ,.outport_bready_o(axi_l_bready_w)
    ,.outport_arvalid_o(axi_l_arvalid_w)
    ,.outport_araddr_o(axi_l_araddr_w)
    ,.outport_rready_o(axi_l_rready_w)
);

riscv_tcm_wrapper
u_cpu
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.rst_cpu_i(rst_cpu_w)
    ,.axi_i_awready_i(axi_i_awready_w)
    ,.axi_i_wready_i(axi_i_wready_w)
    ,.axi_i_bvalid_i(axi_i_bvalid_w)
    ,.axi_i_bresp_i(axi_i_bresp_w)
    ,.axi_i_bid_i(axi_i_bid_w)
    ,.axi_i_arready_i(axi_i_arready_w)
    ,.axi_i_rvalid_i(axi_i_rvalid_w)
    ,.axi_i_rdata_i(axi_i_rdata_w)
    ,.axi_i_rresp_i(axi_i_rresp_w)
    ,.axi_i_rid_i(axi_i_rid_w)
    ,.axi_i_rlast_i(axi_i_rlast_w)
    ,.axi_t_awvalid_i(axi_t_awvalid_w)
    ,.axi_t_awaddr_i(axi_t_awaddr_w)
    ,.axi_t_awid_i(axi_t_awid_w)
    ,.axi_t_awlen_i(axi_t_awlen_w)
    ,.axi_t_awburst_i(axi_t_awburst_w)
    ,.axi_t_wvalid_i(axi_t_wvalid_w)
    ,.axi_t_wdata_i(axi_t_wdata_w)
    ,.axi_t_wstrb_i(axi_t_wstrb_w)
    ,.axi_t_wlast_i(axi_t_wlast_w)
    ,.axi_t_bready_i(axi_t_bready_w)
    ,.axi_t_arvalid_i(axi_t_arvalid_w)
    ,.axi_t_araddr_i(axi_t_araddr_w)
    ,.axi_t_arid_i(axi_t_arid_w)
    ,.axi_t_arlen_i(axi_t_arlen_w)
    ,.axi_t_arburst_i(axi_t_arburst_w)
    ,.axi_t_rready_i(axi_t_rready_w)
    ,.intr_i(cpu_intr_w)

    // Outputs
    ,.axi_i_awvalid_o(axi_i_awvalid_w)
    ,.axi_i_awaddr_o(axi_i_awaddr_w)
    ,.axi_i_awid_o(axi_i_awid_w)
    ,.axi_i_awlen_o(axi_i_awlen_w)
    ,.axi_i_awburst_o(axi_i_awburst_w)
    ,.axi_i_wvalid_o(axi_i_wvalid_w)
    ,.axi_i_wdata_o(axi_i_wdata_w)
    ,.axi_i_wstrb_o(axi_i_wstrb_w)
    ,.axi_i_wlast_o(axi_i_wlast_w)
    ,.axi_i_bready_o(axi_i_bready_w)
    ,.axi_i_arvalid_o(axi_i_arvalid_w)
    ,.axi_i_araddr_o(axi_i_araddr_w)
    ,.axi_i_arid_o(axi_i_arid_w)
    ,.axi_i_arlen_o(axi_i_arlen_w)
    ,.axi_i_arburst_o(axi_i_arburst_w)
    ,.axi_i_rready_o(axi_i_rready_w)
    ,.axi_t_awready_o(axi_t_awready_w)
    ,.axi_t_wready_o(axi_t_wready_w)
    ,.axi_t_bvalid_o(axi_t_bvalid_w)
    ,.axi_t_bresp_o(axi_t_bresp_w)
    ,.axi_t_bid_o(axi_t_bid_w)
    ,.axi_t_arready_o(axi_t_arready_w)
    ,.axi_t_rvalid_o(axi_t_rvalid_w)
    ,.axi_t_rdata_o(axi_t_rdata_w)
    ,.axi_t_rresp_o(axi_t_rresp_w)
    ,.axi_t_rid_o(axi_t_rid_w)
    ,.axi_t_rlast_o(axi_t_rlast_w)
);

core_soc
#(
     .CLK_FREQ(CLK_FREQ)
    ,.BAUDRATE(BAUDRATE)
    ,.C_SCK_RATIO(C_SCK_RATIO)
)
u_soc
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.inport_awvalid_i(axi_l_awvalid_w)
    ,.inport_awaddr_i(axi_l_awaddr_w)
    ,.inport_wvalid_i(axi_l_wvalid_w)
    ,.inport_wdata_i(axi_l_wdata_w)
    ,.inport_wstrb_i(axi_l_wstrb_w)
    ,.inport_bready_i(axi_l_bready_w)
    ,.inport_arvalid_i(axi_l_arvalid_w)
    ,.inport_araddr_i(axi_l_araddr_w)
    ,.inport_rready_i(axi_l_rready_w)
    ,.spi_miso_i(spi_miso_i)
    ,.uart_rx_i(uart_rx_i)
    ,.gpio_input_i(gpio_input_i)
    ,.ext1_cfg_awready_i(ext1_cfg_awready_w)
    ,.ext1_cfg_wready_i(ext1_cfg_wready_w)
    ,.ext1_cfg_bvalid_i(ext1_cfg_bvalid_w)
    ,.ext1_cfg_bresp_i(ext1_cfg_bresp_w)
    ,.ext1_cfg_arready_i(ext1_cfg_arready_w)
    ,.ext1_cfg_rvalid_i(ext1_cfg_rvalid_w)
    ,.ext1_cfg_rdata_i(ext1_cfg_rdata_w)
    ,.ext1_cfg_rresp_i(ext1_cfg_rresp_w)
    ,.ext1_irq_i(ext1_irq_w)
    ,.ext2_cfg_awready_i(ext2_cfg_awready_w)
    ,.ext2_cfg_wready_i(ext2_cfg_wready_w)
    ,.ext2_cfg_bvalid_i(ext2_cfg_bvalid_w)
    ,.ext2_cfg_bresp_i(ext2_cfg_bresp_w)
    ,.ext2_cfg_arready_i(ext2_cfg_arready_w)
    ,.ext2_cfg_rvalid_i(ext2_cfg_rvalid_w)
    ,.ext2_cfg_rdata_i(ext2_cfg_rdata_w)
    ,.ext2_cfg_rresp_i(ext2_cfg_rresp_w)
    ,.ext2_irq_i(ext2_irq_w)
    ,.ext3_cfg_awready_i(ext3_cfg_awready_w)
    ,.ext3_cfg_wready_i(ext3_cfg_wready_w)
    ,.ext3_cfg_bvalid_i(ext3_cfg_bvalid_w)
    ,.ext3_cfg_bresp_i(ext3_cfg_bresp_w)
    ,.ext3_cfg_arready_i(ext3_cfg_arready_w)
    ,.ext3_cfg_rvalid_i(ext3_cfg_rvalid_w)
    ,.ext3_cfg_rdata_i(ext3_cfg_rdata_w)
    ,.ext3_cfg_rresp_i(ext3_cfg_rresp_w)
    ,.ext3_irq_i(ext3_irq_w)

    // Outputs
    ,.intr_o(soc_intr_w)
    ,.inport_awready_o(axi_l_awready_w)
    ,.inport_wready_o(axi_l_wready_w)
    ,.inport_bvalid_o(axi_l_bvalid_w)
    ,.inport_bresp_o(axi_l_bresp_w)
    ,.inport_arready_o(axi_l_arready_w)
    ,.inport_rvalid_o(axi_l_rvalid_w)
    ,.inport_rdata_o(axi_l_rdata_w)
    ,.inport_rresp_o(axi_l_rresp_w)
    ,.spi_clk_o(spi_clk_o)
    ,.spi_mosi_o(spi_mosi_o)
    ,.spi_cs_o(spi_cs_o)
    ,.uart_tx_o(uart_tx_o)
    ,.gpio_output_o(gpio_output_o)
    ,.gpio_output_enable_o(gpio_output_enable_o)
    ,.boot_spi_adr_o(boot_spi_adr_o)
    ,.reboot_o(reboot_o)

    ,.ext1_cfg_awvalid_o(ext1_cfg_awvalid_w)
    ,.ext1_cfg_awaddr_o(ext1_cfg_awaddr_w)
    ,.ext1_cfg_wvalid_o(ext1_cfg_wvalid_w)
    ,.ext1_cfg_wdata_o(ext1_cfg_wdata_w)
    ,.ext1_cfg_wstrb_o(ext1_cfg_wstrb_w)
    ,.ext1_cfg_bready_o(ext1_cfg_bready_w)
    ,.ext1_cfg_arvalid_o(ext1_cfg_arvalid_w)
    ,.ext1_cfg_araddr_o(ext1_cfg_araddr_w)
    ,.ext1_cfg_rready_o(ext1_cfg_rready_w)
    ,.ext2_cfg_awvalid_o(ext2_cfg_awvalid_w)
    ,.ext2_cfg_awaddr_o(ext2_cfg_awaddr_w)
    ,.ext2_cfg_wvalid_o(ext2_cfg_wvalid_w)
    ,.ext2_cfg_wdata_o(ext2_cfg_wdata_w)
    ,.ext2_cfg_wstrb_o(ext2_cfg_wstrb_w)
    ,.ext2_cfg_bready_o(ext2_cfg_bready_w)
    ,.ext2_cfg_arvalid_o(ext2_cfg_arvalid_w)
    ,.ext2_cfg_araddr_o(ext2_cfg_araddr_w)
    ,.ext2_cfg_rready_o(ext2_cfg_rready_w)
    ,.ext3_cfg_awvalid_o(ext3_cfg_awvalid_w)
    ,.ext3_cfg_awaddr_o(ext3_cfg_awaddr_w)
    ,.ext3_cfg_wvalid_o(ext3_cfg_wvalid_w)
    ,.ext3_cfg_wdata_o(ext3_cfg_wdata_w)
    ,.ext3_cfg_wstrb_o(ext3_cfg_wstrb_w)
    ,.ext3_cfg_bready_o(ext3_cfg_bready_w)
    ,.ext3_cfg_arvalid_o(ext3_cfg_arvalid_w)
    ,.ext3_cfg_araddr_o(ext3_cfg_araddr_w)
    ,.ext3_cfg_rready_o(ext3_cfg_rready_w)
);

// enable_w[0] from the dbg_bridge is used for a reset, but  we want to 
// come up running so ignore it until it has been asserted and released once
reg bridge_rst_enable;
reg rst_cpu_r;

always @(posedge clk_i or posedge rst_i) 
    if (rst_i) begin
        bridge_rst_enable <= 0;
        rst_cpu_r <= 1;
    end
    else begin
        if(!bridge_rst_enable)
            bridge_rst_enable <= enable_w[0];

        if(bridge_rst_enable)
            rst_cpu_r <= ~enable_w[0];
        else
            rst_cpu_r <= 0;
    end


assign rst_cpu_w       = rst_cpu_r;
assign cpu_intr_w      = {31'b0, soc_intr_w};

`ifdef INCLUDE_ETHERNET
eth_axi4lite u_eth (
      // axi4lite Inputs
    .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.cfg_awvalid_i(ext1_cfg_awvalid_w)
    ,.cfg_awaddr_i(ext1_cfg_awaddr_w)
    ,.cfg_wvalid_i(ext1_cfg_wvalid_w)
    ,.cfg_wdata_i(ext1_cfg_wdata_w)
    ,.cfg_wstrb_i(ext1_cfg_wstrb_w)
    ,.cfg_bready_i(ext1_cfg_bready_w)
    ,.cfg_arvalid_i(ext1_cfg_arvalid_w)
    ,.cfg_araddr_i(ext1_cfg_araddr_w)
    ,.cfg_rready_i(ext1_cfg_rready_w)

    // axi4lite Outputs
    ,.cfg_awready_o(ext1_cfg_awready_w)
    ,.cfg_wready_o(ext1_cfg_wready_w)
    ,.cfg_bvalid_o(ext1_cfg_bvalid_w)
    ,.cfg_bresp_o(ext1_cfg_bresp_w)
    ,.cfg_arready_o(ext1_cfg_arready_w)
    ,.cfg_rvalid_o(ext1_cfg_rvalid_w)
    ,.cfg_rdata_o(ext1_cfg_rdata_w)
    ,.cfg_rresp_o(ext1_cfg_rresp_w)


    // peripheral inputs
    ,.clock_125_i(clock_125_i)

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
);
`endif

endmodule


