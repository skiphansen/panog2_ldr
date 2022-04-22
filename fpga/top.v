//-----------------------------------------------------------------
// TOP
//-----------------------------------------------------------------
module top
(
     input           SYSCLK
    ,inout           pano_button
    ,output          GMII_RST_N
    ,output          led_red
    ,inout           led_green
    ,inout           led_blue

    // UART
    ,input           uart_txd_i
    ,output          uart_rxd_o
    // SPI-Flash
    ,output          flash_sck_o
    ,output          flash_cs_o
    ,output          flash_si_o
    ,input           flash_so_i

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
);

// Generate 50 Mhz system clock and 24 Mhz USB clock from 125 Mhz input clock
wire clk50;

IBUFG clk125_buf
(   .O (clk125),
    .I (SYSCLK)
);


PLL_BASE
    #(.BANDWIDTH              ("OPTIMIZED"),
      .CLKFBOUT_MULT          (24),
      .CLKFBOUT_PHASE         (0.000),
      .CLK_FEEDBACK           ("CLKFBOUT"),
      .CLKIN_PERIOD           (8.000),
      .COMPENSATION           ("SYSTEM_SYNCHRONOUS"),
      .DIVCLK_DIVIDE          (5),
      .REF_JITTER             (0.010),
      .CLKOUT0_DIVIDE         (15),
      .CLKOUT0_DUTY_CYCLE     (0.500),
      .CLKOUT0_PHASE          (0.000),
      .CLKOUT1_DIVIDE         (25),
      .CLKOUT1_DUTY_CYCLE     (0.500),
      .CLKOUT1_PHASE          (0.000)
    )
    pll_base_inst
      // Output clocks
     (.CLKFBOUT              (clkfbout),
      .CLKOUT0               (clkout50),
      .CLKOUT1               (clkout24),
      .CLKOUT2               (),
      .CLKOUT3               (),
      .CLKOUT4               (),
      .CLKOUT5               (),
      // Status and control signals
      .LOCKED                (),
      .RST                   (RESET),
       // Input clock control
      .CLKFBIN               (clkfbout_buf),
      .CLKIN                 (clk125)
);

// Output buffering
//-----------------------------------
BUFG clkf_buf
 (.O (clkfbout_buf),
  .I (clkfbout));

BUFG clk50_buf
  (.O (clk50),
   .I (clkout50));

BUFG clk24_buf
(.O (mhz24_buf),
 .I (clkout24));

ODDR2 clkout1_buf (
  .S(1'b0),
  .R(1'b0),
  .D0(1'b1),
  .D1(1'b0),
  .C0(mhz24_buf),
  .C1(!mhz24_buf),
  .CE(1'b1),
  .Q(usb_clk)
);

//-----------------------------------------------------------------
// Reset
//-----------------------------------------------------------------
wire rst;

reset_gen
u_rst
(
    .clk_i(clk50),
    .rst_o(rst)
);

//-----------------------------------------------------------------
// Core
//-----------------------------------------------------------------
wire        dbg_txd_w;
wire        uart_txd_w;

wire        spi_clk_w;
wire        spi_so_w;
wire        spi_si_w;
wire [7:0]  spi_cs_w;

wire [31:0] gpio_in_w;
wire [31:0] gpio_out_w;
wire [31:0] gpio_out_en_w;

fpga_top
#(
    .CLK_FREQ(40000000)
   ,.BAUDRATE(1000000)   // SoC UART baud rate
   ,.UART_SPEED(1000000) // Debug bridge UART baud (should match BAUDRATE)
   ,.C_SCK_RATIO(1)      // SPI clock divider (M25P128 maxclock = 54 Mhz)
   ,.CPU("riscv")        // riscv or armv6m
)
u_top
(
    .clock_125_i(clk125)
    ,.clk_i(clk50)
    ,.rst_i(rst)

    ,.dbg_rxd_o(dbg_txd_w)
    ,.dbg_txd_i(uart_txd_i)

    ,.uart_tx_o(uart_txd_w)
    ,.uart_rx_i(uart_txd_i)

    ,.spi_clk_o(spi_clk_w)
    ,.spi_mosi_o(spi_si_w)
    ,.spi_miso_i(spi_so_w)
    ,.spi_cs_o(spi_cs_w)
    ,.gpio_input_i(gpio_in_w)
    ,.gpio_output_o(gpio_out_w)
    ,.gpio_output_enable_o(gpio_out_en_w)

`ifdef INCLUDE_ETHERNET
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
  // 
    ,.mdc_o(mdc_o)
    ,.mdio_io(mdio_io)
`endif
);

//-----------------------------------------------------------------
// SPI Flash
//-----------------------------------------------------------------
assign flash_sck_o = spi_clk_w;
assign flash_si_o  = spi_si_w;
assign flash_cs_o  = spi_cs_w[0];
assign spi_so_w    = flash_so_i;

//-----------------------------------------------------------------
// GPIO bits
// 0: Not implmented
// 1: Pano button
// 2: Output only - red LED
// 3: In/out - green LED
// 4: In/out - blue LED
// 5: Wolfson codec SDA
// 6: Wolfson codec SCL
// 9...31: Not implmented
//-----------------------------------------------------------------

assign gpio_in_w[0]  = gpio_out_w[0];

assign pano_button = gpio_out_en_w[1]  ? gpio_out_w[1]  : 1'bz;
assign gpio_in_w[1]  = pano_button;

assign led_red = gpio_out_w[2];
assign gpio_in_w[2]  = led_red;

assign led_green = gpio_out_en_w[3]  ? gpio_out_w[3]  : 1'bz;
assign gpio_in_w[3]  = led_green;

assign led_blue = gpio_out_en_w[4]  ? gpio_out_w[4]  : 1'bz;
assign gpio_in_w[4]  = led_blue;

assign codec_sda = gpio_out_en_w[5]  ? gpio_out_w[5]  : 1'bz;
assign gpio_in_w[5]  = codec_sda;


assign codec_scl = gpio_out_en_w[6]  ? gpio_out_w[6]  : 1'bz;
assign gpio_in_w[6]  = codec_scl;

genvar i;
generate
for (i=7; i < 32; i=i+1) begin : gpio_in
    assign gpio_in_w[i]  = 1'b0;
end
endgenerate

//-----------------------------------------------------------------
// UART Tx combine
//-----------------------------------------------------------------
// Xilinx placement pragmas:
//synthesis attribute IOB of uart_rxd_o is "TRUE"
reg txd_q;

always @ (posedge clk50 or posedge rst)
if (rst)
    txd_q <= 1'b1;
else
    txd_q <= dbg_txd_w & uart_txd_w;

// 'OR' two UARTs together
assign uart_rxd_o  = txd_q;

//-----------------------------------------------------------------
// Tie-offs
//-----------------------------------------------------------------

// Must remove reset from the Ethernet Phy for 125 Mhz input clock.
// See https://github.com/tomverbeure/panologic-g2
assign GMII_RST_N = 1'b1;

endmodule
