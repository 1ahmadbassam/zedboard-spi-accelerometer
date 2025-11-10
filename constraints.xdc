# ZedBoard xdc
############################
# JA1Connector             #
############################

# Pin 1: ~CS
set_property PACKAGE_PIN Y11 [get_ports { spi_cs_n_0 }];
set_property IOSTANDARD LVCMOS33 [get_ports { spi_cs_n_0 }];

# Pin 2: MOSI
set_property PACKAGE_PIN AA11 [get_ports { spi_mosi_0 }];
set_property IOSTANDARD LVCMOS33 [get_ports { spi_mosi_0 }];

# Pin 3: MISO
set_property PACKAGE_PIN Y10 [get_ports { spi_miso_0 }];
set_property IOSTANDARD LVCMOS33 [get_ports { spi_miso_0 }];

# Pin 4: SCK
set_property PACKAGE_PIN AA9 [get_ports { spi_clk_0 }];
set_property IOSTANDARD LVCMOS33 [get_ports { spi_clk_0 }];