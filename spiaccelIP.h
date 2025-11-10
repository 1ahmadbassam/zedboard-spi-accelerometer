
#ifndef SPIACCELIP_H
#define SPIACCELIP_H


/****************** Include Files ********************/
#include "xil_types.h"
#include "xil_io.h"
#include "xstatus.h"

#define SPIACCELIP_S00_AXI_SLV_REG0_OFFSET 0
#define SPIACCELIP_S00_AXI_SLV_REG1_OFFSET 4
#define SPIACCELIP_S00_AXI_SLV_REG2_OFFSET 8
#define SPIACCELIP_S00_AXI_SLV_REG3_OFFSET 12

#define SPI_CSR_OFFSET 	SPIACCELIP_S00_AXI_SLV_REG0_OFFSET
#define SPI_TX_OFFSET	SPIACCELIP_S00_AXI_SLV_REG1_OFFSET
#define SPI_RX_OFFSET	SPIACCELIP_S00_AXI_SLV_REG2_OFFSET

typedef enum {SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3} SPIMode;

/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a SPIACCELIP register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the SPIACCELIPdevice.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void SPIACCELIP_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define SPIACCELIP_mWriteReg(BaseAddress, RegOffset, Data) \
  	Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a SPIACCELIP register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the SPIACCELIP device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 SPIACCELIP_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define SPIACCELIP_mReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))

/************************** Function Prototypes ****************************/
/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the SPIACCELIP instance to be worked on.
 *
 * @return
 *
 *    - XST_SUCCESS   if all self-test code passed
 *    - XST_FAILURE   if any self-test code failed
 *
 * @note    Caching must be turned off for this function to work.
 * @note    Self test may fail if data memory and device are not on the same bus.
 *
 */
XStatus SPIACCELIP_Reg_SelfTest(void * baseaddr_p);

XStatus spi_reset(void);
XStatus spi_configure(SPIMode mode, u32 clk_scale, u32 cs_inactive_clks);
XStatus spi_write(u32 *bytes, u32 count);
XStatus spi_read(u32 *bytes, u32 count);
XStatus spi_readaddr(u32 addr, u32 *bytes, u32 count);

#endif // SPIACCELIP_H
