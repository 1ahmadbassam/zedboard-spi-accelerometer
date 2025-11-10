

/***************************** Include Files *******************************/
#include "spiaccelIP.h"
#include "xparameters.h"

#define SPI_BASE_ADDR 	XPAR_SPIACCELIP_0_BASEADDR
/************************** Function Definitions ***************************/

XStatus spi_reset(void) {
	u32 configData;
	
	configData = SPIACCELIP_mReadReg(SPI_BASE_ADDR, SPI_CSR_OFFSET);
	configData = configData | 0x1;
	SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_CSR_OFFSET, configData);
	configData = configData & 0xFFFFFFFE;
	SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_CSR_OFFSET, configData);
	configData = configData | 0x1;
	SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_CSR_OFFSET, configData);
	
	return XST_SUCCESS;
}

XStatus spi_configure(SPIMode mode, u32 clk_scale, u32 cs_inactive_clks) {
	u32 configData;

	if (clk_scale > 65535 || clk_scale < 2 || cs_inactive_clks == 0 || cs_inactive_clks > 7)
		return XST_FAILURE;
	
	spi_reset();
	configData = ((cs_inactive_clks & 0x7) << 19) | ((clk_scale & 0xFFFF) << 3) | ((mode & 0x3) << 1) | 0x1;
	SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_CSR_OFFSET, configData);
	return XST_SUCCESS;
}

XStatus spi_write(u32 *bytes, u32 count) {
	u32 i;
	u32 writeData;

	if (count > 16)
		return XST_FAILURE;
	
	for (i = 0; i < count; i++) {
		if (i == 0)
			writeData = ((count & 0x1F) << 9) | ((bytes[i] & 0xFF) << 1) | 0x1;
		else
			writeData = ((bytes[i] & 0xFF) << 1) | 0x1;
		// wait until the SPI master is ready
		while ( !(SPIACCELIP_mReadReg(SPI_BASE_ADDR, SPI_CSR_OFFSET) & 0x80000000) );

		// pulse TX_DV to send the data
		SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_TX_OFFSET, writeData);
		writeData = writeData & 0xFFFFFFFE;
		SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_TX_OFFSET, writeData);
	}
	
	return XST_SUCCESS;
}

XStatus spi_read(u32 *bytes, u32 count) {
	u32 i;
	// to read, we have to write some dummy data
	u32 dummyData;
	u32 readData;

	if (count > 16)
		return XST_FAILURE;

	for (i = 0; i < count; i++) {
		if (i == 0)
			dummyData = ((count & 0x1F) << 9) | 0x1;
		else 
			dummyData = 0x1;
		
		// wait until the SPI master is ready
		while ( !(SPIACCELIP_mReadReg(SPI_BASE_ADDR, SPI_CSR_OFFSET) & 0x80000000) );

		// pulse TX_DV to send the data
		SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_TX_OFFSET, dummyData);
		dummyData = dummyData & 0xFFFFFFFE;
		SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_TX_OFFSET, dummyData);

		// poll for RX_DV
		while ( !(SPIACCELIP_mReadReg(SPI_BASE_ADDR, SPI_RX_OFFSET) & 0x1) );

		// read the data
		readData = SPIACCELIP_mReadReg(SPI_BASE_ADDR, SPI_RX_OFFSET);
		bytes[(readData >> 9) & 0xF] = (readData >> 1) & 0xFF;	
	}
	
	return XST_SUCCESS;
}

XStatus spi_readaddr(u32 addr, u32 *bytes, u32 count) {
// this function is almost the same as read, but we write the first byte as an "address"
// so we count one additional byte
	u32 i;
	// to read, we have to write some dummy data
	u32 dummyData;
	u32 readData;
	
    u32 total_bytes = count + 1;
    // check against hardware limit (16 bytes max transfer)
    if (total_bytes > 16) {
        return XST_FAILURE;
    }	
    
	for (i = 0; i < total_bytes; i++) {
		if (i == 0)
			dummyData = ((total_bytes & 0x1F) << 9) | ((addr & 0xFF) << 1) | 0x1;
		else 
			dummyData = 0x1;
		
		// wait until the SPI master is ready
		while ( !(SPIACCELIP_mReadReg(SPI_BASE_ADDR, SPI_CSR_OFFSET) & 0x80000000) );

		// pulse TX_DV to send the data
		SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_TX_OFFSET, dummyData);
		dummyData = dummyData & 0xFFFFFFFE;
		SPIACCELIP_mWriteReg(SPI_BASE_ADDR, SPI_TX_OFFSET, dummyData);

		// poll for RX_DV
		while ( !(SPIACCELIP_mReadReg(SPI_BASE_ADDR, SPI_RX_OFFSET) & 0x1) );

		// read the data (ignore the first byte)
		if (i != 0) {
			readData = SPIACCELIP_mReadReg(SPI_BASE_ADDR, SPI_RX_OFFSET);
			bytes[i - 1] = (readData >> 1) & 0xFF;
		}
	}
	
	return XST_SUCCESS;
}
