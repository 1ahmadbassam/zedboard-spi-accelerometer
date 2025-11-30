# Zynq Accelerometer System with Hardware CORDIC Calculation

This project implements a complete system for interfacing a Xilinx Zynq (ZedBoard) with an ADXL345 accelerometer (PmodACL). Originally designed for **EECE 423**, the project has been expanded to move computational load from the processor to the Programmable Logic (PL).

The system features two custom AXI-Lite IP blocks:
1.  **SPI Master IP:** Handles communication with the accelerometer.
2.  **CORDIC Angle IP:** A hardware accelerator that computes the inclination angle using the CORDIC algorithm in fixed-point arithmetic, replacing the software `atan2` function.

## Project Components

The design is partitioned between the Programmable Logic (Verilog) and the Processing System (C Application).

### Hardware (Verilog - PL)

#### 1. SPI Master IP (`spiaccelIP`)
This block acts as the bridge between the Zynq and the ADXL345.
* **`spi_master.v`**: The core FSM managing SPI transactions. Supports all 4 modes and multi-byte transfers.
* **`spiaccelIP_slave...v`**: The AXI-Lite interface exposing Control, TX, and RX registers to the CPU.
* **`constraints.xdc`**: Maps the SPI signals to the ZedBoard **JA1** Pmod connector.

#### 2. CORDIC Angle IP (`cordicIP`)
A dedicated hardware unit that computes $\theta = atan2(Y, Z)$ using integer arithmetic.
* **`cordic_angle.v`**: The DSP datapath. It implements a 13-iteration CORDIC algorithm (Vectoring Mode) using only shifts and adds.
    * **Input:** 16-bit signed integers (Y, Z) from the accelerometer.
    * **Output:** 16-bit **Q3.12 Fixed-Point** angle (1 sign bit, 3 integer bits, 12 fractional bits).
    * **Logic:** Uses a lookup table for arctan constants to converge on the angle.
* **`cordicIP_slave...v`**: The AXI-Lite interface mapping the CORDIC signals to a 5-register memory map:
    * `0x00` (C_Y) & `0x04` (C_Z): Input registers.
    * `0x08` (C_CTRL): Control register (Bit 0 = Start).
    * `0x0C` (C_STATUS): Status register (Bit 0 = Done).
    * `0x10` (C_ANGLE): Result output register.

### Software (C - PS)

The application running on the ARM processor orchestrates the data flow between the two hardware IPs.

* **Drivers (`spiaccelIP.c` / `cordicIP.h`)**: Low-level drivers to handle AXI register read/writes.
* **`accelerometer.c`**: The main application logic.
    1.  **Initialization:** Configures the SPI core (Mode 3, 4MHz) and the ADXL345 (Data Format, Power Control).
    2.  **Interrupt Loop:** A timer triggers an interrupt every 500ms.
    3.  **Data Acquisition:** Reads raw X, Y, Z acceleration data via the **SPI IP**.
    4.  **Hardware Computation:** Writes Y and Z to the **CORDIC IP**, asserts the "Start" bit, polls for "Done", and reads the resulting fixed-point angle.
    5.  **Software Verification:** Computes the same angle using the C math library `atan2f`.
    6.  **Reporting:** Prints the Hardware Angle, Software Angle, and the error margin to the serial terminal.

## Fixed-Point Format & Accuracy

The CORDIC IP outputs data in **Q3.12 format**. The conversion logic used in the software is:

$$\theta_{radians} = \frac{\text{Hardware\_Output}}{2^{12}} = \frac{\text{Hardware\_Output}}{4096.0}$$

* **Precision:** The hardware uses 13 iterations.
* **Accuracy:** The hardware result typically matches the floating-point software result with an error margin of < 0.004 radians (< 0.2 degrees).

## Build and Run

1.  **Hardware (Vivado):**
    * Package both `spiaccelIP` and `cordicIP` as generic AXI peripherals.
    * Create a Block Design connecting both IPs to the Zynq Processing System and an AXI Timer.
    * Import `constraints.xdc` and generate the bitstream.
2.  **Software (Vitis/SDK):**
    * Export the XSA hardware platform.
    * Import the C sources (`accelerometer.c`, drivers).
    * Ensure the linker includes the math library (`-lm`) for the software comparison.
    * Run on ZedBoard and view output via Serial Terminal (115200 baud).
