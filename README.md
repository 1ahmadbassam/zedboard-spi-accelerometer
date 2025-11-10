# AXI-Lite SPI Master for Accelerometer

This project is a complete system for interfacing a Xilinx Zynq (on a ZedBoard) with an ADXL345 accelerometer (on a PmodACL). It was developed for the EECE 423: Reconfigurable Computing course.

The core of the project is a custom AXI-Lite SPI master, created in Verilog, which is deployed to the Zynq's Programmable Logic (PL). This hardware block is then controlled by a C application running on the Processing System (PS), which reads the accelerometer data and calculates the board's inclination.

## Project Components

The project is split into two main parts: the Verilog hardware (PL) and the C software (PS).

### Hardware (Verilog - PL)

The hardware consists of a custom AXI-Lite IP block (`spiaccelIP`) that acts as an SPI master.

* **`spi_master.v`**: This is the core SPI master logic. It's a configurable FSM-based module that manages SPI transactions. It supports all 4 SPI modes, a configurable clock frequency (derived from the 100MHz AXI clock), and multi-byte transfers up to 16 bytes.
* **`spiaccelIP_slave_lite_v1_0_S00_AXI.v`**: This is the AXI-Lite slave interface that connects to the Zynq's processing system. It provides three 32-bit registers for software control:
    * **Register 0 (CSR):** Control/Status Register. Used to configure the SPI mode, clock scale, and reset the core. It also provides the `o_TX_Ready` status bit.
    * **Register 1 (TX):** Transmit Register. Writing to this register triggers a new SPI transaction, sending the specified byte(s).
    * **Register 2 (RX):** Receive Register. Polling this register allows the software to retrieve data received from the SPI slave.
* **`spiaccelIP.v`**: The top-level wrapper that instantiates both the AXI slave interface and the `spi_master` logic, connecting them.
* **`constraints.xdc`**: The Xilinx Design Constraints file. This is crucial as it maps the external SPI ports of the Verilog IP (`spi_clk`, `spi_mosi`, `spi_miso`, `spi_cs_n`) to the physical pins of the ZedBoard's **JA1** Pmod connector.

### Software (C - PS)

The software runs on the Zynq's ARM processor and controls the custom hardware.

* **`spiaccelIP.h` / `spiaccelIP.c`**: This is the low-level C driver for the custom IP. It abstracts the AXI-Lite register operations into a set of simple functions (e.g., `spi_configure`, `spi_write`, `spi_readaddr`).
* **`accelerometer.c`**: This is the main application that demonstrates the complete system. Its logic is as follows:
    1.  Initializes the AXI Timer to generate an interrupt every 500ms.
    2.  Initializes the custom SPI IP using the driver, setting it to **SPI Mode 3** and a 4MHz clock (100MHz / 25).
    3.  Initializes the ADXL345 accelerometer by writing to its `DATA_FORMAT` and `POWER_CTL` registers.
    4.  Enters an infinite loop (acting as a background thread).
    5.  The **Timer Interrupt Handler** (`TM_Intr_Handler`) executes every 500ms. Inside the handler:
        * It reads the 6 data registers (X, Y, Z acceleration) from the ADXL345 using a multi-byte read.
        * It calculates the board's Y-Z plane inclination in degrees using the `atan2` math function.
        * It prints the calculated angle to the serial terminal (PuTTY).

## Build and Run

1.  **Hardware (Vivado):** The Verilog files (`.v`) are used to create a new AXI-Lite IP in Vivado. This IP is added to a block design along with a Zynq Processing System and an AXI Timer. The `constraints.xdc` file is added to the project, and a bitstream is generated.
2.  **Software (Vitis/SDK):** The hardware platform (XSA) is exported from Vivado to Vitis. A new application is created, and the C files (`.c`, `.h`) are added. The application is built (linking the `m` library for `atan2`) and run on the ZedBoard.