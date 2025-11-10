
`timescale 1 ns / 1 ps

	module spiaccelIP #
	(
		// Users to add parameters here

		// User parameters ends
		// Do not modify the parameters beyond this line


		// Parameters of Axi Slave Bus Interface S00_AXI
		parameter integer C_S00_AXI_DATA_WIDTH	= 32,
		parameter integer C_S00_AXI_ADDR_WIDTH	= 4
	)
	(
		// Users to add ports here
		output wire spi_clk,
		input  wire spi_miso,
		output wire spi_mosi,
		output wire spi_cs_n,
		// User ports ends
		// Do not modify the ports beyond this line

		// Ports of Axi Slave Bus Interface S00_AXI
		input wire  s00_axi_aclk,
		input wire  s00_axi_aresetn,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_awaddr,
		input wire [2 : 0] s00_axi_awprot,
		input wire  s00_axi_awvalid,
		output wire  s00_axi_awready,
		input wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_wdata,
		input wire [(C_S00_AXI_DATA_WIDTH/8)-1 : 0] s00_axi_wstrb,
		input wire  s00_axi_wvalid,
		output wire  s00_axi_wready,
		output wire [1 : 0] s00_axi_bresp,
		output wire  s00_axi_bvalid,
		input wire  s00_axi_bready,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_araddr,
		input wire [2 : 0] s00_axi_arprot,
		input wire  s00_axi_arvalid,
		output wire  s00_axi_arready,
		output wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_rdata,
		output wire [1 : 0] s00_axi_rresp,
		output wire  s00_axi_rvalid,
		input wire  s00_axi_rready
	);
    wire s00_axi_rst_l;
    wire [1:0] s00_axi_spi_mode;
    wire [15:0] s00_axi_clk_scale; // must be >=2
    wire [2:0] s00_axi_cs_inactive_clks;
    // TX signals
    wire [4:0] s00_axi_tx_count;
    wire [7:0] s00_axi_tx_byte;
    wire s00_axi_tx_dv;
    wire s00_axi_tx_ready;
    // RX signals
    wire [3:0] s00_axi_rx_count;
    wire s00_axi_rx_dv;
    wire [7:0] s00_axi_rx_byte;

    // Instantiation of Axi Bus Interface S00_AXI
	spiaccelIP_slave_lite_v1_0_S00_AXI # ( 
		.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
		.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
	) spiaccelIP_slave_lite_v1_0_S00_AXI_inst (
		.S_AXI_ACLK(s00_axi_aclk),
		.S_AXI_ARESETN(s00_axi_aresetn),
		.S_AXI_AWADDR(s00_axi_awaddr),
		.S_AXI_AWPROT(s00_axi_awprot),
		.S_AXI_AWVALID(s00_axi_awvalid),
		.S_AXI_AWREADY(s00_axi_awready),
		.S_AXI_WDATA(s00_axi_wdata),
		.S_AXI_WSTRB(s00_axi_wstrb),
		.S_AXI_WVALID(s00_axi_wvalid),
		.S_AXI_WREADY(s00_axi_wready),
		.S_AXI_BRESP(s00_axi_bresp),
		.S_AXI_BVALID(s00_axi_bvalid),
		.S_AXI_BREADY(s00_axi_bready),
		.S_AXI_ARADDR(s00_axi_araddr),
		.S_AXI_ARPROT(s00_axi_arprot),
		.S_AXI_ARVALID(s00_axi_arvalid),
		.S_AXI_ARREADY(s00_axi_arready),
		.S_AXI_RDATA(s00_axi_rdata),
		.S_AXI_RRESP(s00_axi_rresp),
		.S_AXI_RVALID(s00_axi_rvalid),
		.S_AXI_RREADY(s00_axi_rready),
		.S_AXI_O_RST_L(s00_axi_rst_l),
        .S_AXI_O_SPI_MODE(s00_axi_spi_mode),
        .S_AXI_O_CLK_SCALE(s00_axi_clk_scale),
        .S_AXI_O_CS_INACTIVE_CLKS(s00_axi_cs_inactive_clks),
        .S_AXI_O_TX_COUNT(s00_axi_tx_count),
        .S_AXI_O_TX_BYTE(s00_axi_tx_byte),
        .S_AXI_O_TX_DV(s00_axi_tx_dv),
        .S_AXI_I_TX_READY(s00_axi_tx_ready),
        .S_AXI_I_RX_COUNT(s00_axi_rx_count),
        .S_AXI_I_RX_DV(s00_axi_rx_dv),
        .S_AXI_I_RX_BYTE(s00_axi_rx_byte)
	);

	// Add user logic here
    spi_master spi_master_inst(
        // Control signals
        .i_Rst_L(s00_axi_rst_l),
        .i_Clk(s00_axi_aclk),
        .i_spi_mode(s00_axi_spi_mode),
        .i_clk_scale(s00_axi_clk_scale), // MUST BE >=2
        .i_cs_inactive_clks(s00_axi_cs_inactive_clks),
        // TX signals
        .i_TX_Count(s00_axi_tx_count),
        .i_TX_Byte(s00_axi_tx_byte),
        .i_TX_DV(s00_axi_tx_dv),
        .o_TX_Ready(s00_axi_tx_ready),
        // RX signals
        .o_RX_Count(s00_axi_rx_count),
        .o_RX_DV(s00_axi_rx_dv),
        .o_RX_Byte(s00_axi_rx_byte),
        // SPI signals
        .o_SPI_Clk(spi_clk),
        .i_SPI_MISO(spi_miso),
        .o_SPI_MOSI(spi_mosi),
        .o_SPI_CS_n(spi_cs_n)
    );
	// User logic ends

	endmodule
