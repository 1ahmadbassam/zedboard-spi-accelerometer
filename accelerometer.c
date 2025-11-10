#include "xparameters.h"
#include "xinterrupt_wrap.h" 
#include "xil_printf.h"
#include "xtmrctr.h"
#include "spiaccelIP.h"
#include <xtmrctr_l.h>
#include <math.h>

#define TIMER_BASE_ADDR		        XPAR_XTMRCTR_0_BASEADDR
#define TMR_INTERRUPT_ID            XPAR_FABRIC_XTMRCTR_0_INTR

// ADXL345 defines
#define ADXL345_DATA_FORMAT_REG     0x31
#define ADXL345_POWER_CTL_REG       0x2D
#define ADXL345_DEVID_REG           0x00

#define ADXL345_DATA_FORMAT         0x09
#define ADXL345_POWER_CTL           0x08
#define ADXL345_DEVID               0xE5

#define ADXL345_DATA_REG            0x32

// 500msec
#define TIMER_RELOAD_VALUE          50000000

XTmrCtr TMRInst; 

static int TimerInitFunction(XTmrCtr *InstancePtr, UINTPTR BaseAddress);
static XStatus ADXL345InitFunction(void);
static XStatus ADXL345WriteByte(u32 address, u32 byte);
static XStatus ADXL345ReadByte(u32 address, u32 *byte);
static XStatus ADXL345ReadBytes(u32 address, u32 *bytes, u32 count);
static XStatus ADXL345GetAcceleration(int *x, int *y, int *z);

void TM_Intr_Handler(void *CallbackRef) {
    XTmrCtr *TMRInst_LOCAL = (XTmrCtr*) CallbackRef;
    XTmrCtrStats stats;
    int x, y, z;

    XTmrCtr_DisableIntr(TIMER_BASE_ADDR, 0);

    // Read and clear the interrupt status in the AXI timer immediately
    XTmrCtr_GetStats(TMRInst_LOCAL, &stats);
    XTmrCtr_ClearStats(TMRInst_LOCAL);
    
    // print yz plane inclination
    if (ADXL345GetAcceleration(&x, &y, &z) == XST_SUCCESS) {
        xil_printf("Y-Z Plane Inclination: %f degrees \r\n", 
            atan2((double)y, (double)z) * (180.0 / M_PI));
    }

    XTmrCtr_ClearStats(TMRInst_LOCAL);

    // Enable timer interrupts
    XTmrCtr_EnableIntr(TIMER_BASE_ADDR, 0);
}

// Initialize Timer using Base Address
static int TimerInitFunction(XTmrCtr *InstancePtr, UINTPTR BaseAddress)
{
    XTmrCtr_Config *ConfigPtr;
    
    ConfigPtr = XTmrCtr_LookupConfig(BaseAddress);
    if (ConfigPtr == NULL) { return XST_FAILURE; }

    XTmrCtr_CfgInitialize(InstancePtr, ConfigPtr, ConfigPtr->BaseAddress);
    
    return XST_SUCCESS;
}

static XStatus ADXL345InitFunction(void) {
    int status;
    u32 byte;
    
    status = ADXL345WriteByte(ADXL345_DATA_FORMAT_REG, ADXL345_DATA_FORMAT);
    if (status != XST_SUCCESS) return XST_FAILURE;
    
    status = ADXL345WriteByte(ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL);
    if (status != XST_SUCCESS) return XST_FAILURE;

    // perform a test of reading the DEVID register
    status = ADXL345ReadByte(ADXL345_DEVID_REG, &byte);
    if (status != XST_SUCCESS || byte != ADXL345_DEVID) return XST_FAILURE;
    xil_printf("ADXL345 initialized successfully!\r\n");
    return XST_SUCCESS;
}

int main (void)
{
    int status;
    
    xil_printf("--- Accelerometer ---\r\n");
    
    // SPI
    status = spi_configure(SPI_MODE3, 25, 1);
    if(status != XST_SUCCESS) return XST_FAILURE;
    status = ADXL345InitFunction();
    if(status != XST_SUCCESS) return XST_FAILURE;
    
    // Timer
    status = TimerInitFunction(&TMRInst, TIMER_BASE_ADDR);
    if(status != XST_SUCCESS) return XST_FAILURE;

    XTmrCtr_SetResetValue(&TMRInst, 0, TIMER_RELOAD_VALUE);
    XTmrCtr_SetOptions(&TMRInst, 0, XTC_DOWN_COUNT_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_INT_MODE_OPTION);
    XTmrCtr_Start(&TMRInst, 0);
    
    status = XSetupInterruptSystem(&TMRInst,
                                (Xil_ExceptionHandler)TM_Intr_Handler,
                                XPAR_FABRIC_XTMRCTR_0_INTR,
                                0x0, 
                                XINTERRUPT_DEFAULT_PRIORITY);
    if(status != XST_SUCCESS) return XST_FAILURE;

    while(1) {
        // do nothing, will be interrupted
    }

    return 0;
}

static XStatus ADXL345WriteByte(u32 address, u32 byte) {
    u32 data[2] = {address & 0x3F, byte & 0xFF};
    return spi_write(data, 2);
}

static XStatus ADXL345ReadByte(u32 address, u32 *byte) {
    return spi_readaddr(0x80 | (address & 0x3F), byte, 1);
}

static XStatus ADXL345ReadBytes(u32 address, u32 *bytes, u32 count) {
    return spi_readaddr(0xC0 | (address & 0x3F), bytes, count);
}

static XStatus ADXL345GetAcceleration(int *x, int *y, int *z) {
    int status;
    u32 data[6];
    
    status = ADXL345ReadBytes(ADXL345_DATA_REG, data, 6);
    if(status != XST_SUCCESS) return XST_FAILURE;
    
    *x = (s16) (((data[1] & 0xFF) << 8) | (data[0] & 0xFF));
    *y = (s16) (((data[3] & 0xFF) << 8) | (data[2] & 0xFF));
    *z = (s16) (((data[5] & 0xFF) << 8) | (data[4] & 0xFF));
    return XST_SUCCESS;
}