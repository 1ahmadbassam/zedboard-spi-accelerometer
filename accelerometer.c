#include "xparameters.h"
#include "xinterrupt_wrap.h" 
#include "xil_printf.h"
#include "xtmrctr.h"
#include "spiaccelIP.h"
#include "cordicIP.h"
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

// Cordic Unit
#define CORDIC_BASE_ADDRESS         XPAR_CORDICIP_0_BASEADDR
#define C_Y CORDICIP_S00_AXI_SLV_REG0_OFFSET
#define C_Z CORDICIP_S00_AXI_SLV_REG1_OFFSET
#define C_CTRL CORDICIP_S00_AXI_SLV_REG2_OFFSET
#define C_STATUS CORDICIP_S00_AXI_SLV_REG3_OFFSET
#define C_ANGLE CORDICIP_S00_AXI_SLV_REG4_OFFSET
#define SCALE_FACTOR (1.0f / 4096.0f)

float atan2_cordic(short y_accel, short z_accel) {
    int ctrl;
    short angle;

    CORDICIP_mWriteReg(CORDIC_BASE_ADDRESS, C_Y, y_accel);
    CORDICIP_mWriteReg(CORDIC_BASE_ADDRESS, C_Z, z_accel);

    // start operation
    ctrl = CORDICIP_mReadReg(CORDIC_BASE_ADDRESS, C_CTRL);
    CORDICIP_mWriteReg(CORDIC_BASE_ADDRESS, C_CTRL, ctrl | 0x1);
    CORDICIP_mWriteReg(CORDIC_BASE_ADDRESS, C_CTRL, ctrl & 0xFFFFFFFE);

    // wait for operation to finish
    while (!(CORDICIP_mReadReg(CORDIC_BASE_ADDRESS, C_STATUS) & 0x1));

    angle = (short) (CORDICIP_mReadReg(CORDIC_BASE_ADDRESS, C_ANGLE) & 0x0000FFFF);
    return (float)angle * SCALE_FACTOR;
}

XTmrCtr TMRInst; 
static volatile int g_x;
static volatile int g_y;
static volatile int g_z;
static volatile u8 g_flag;

static int TimerInitFunction(XTmrCtr *InstancePtr, UINTPTR BaseAddress);
static XStatus ADXL345InitFunction(void);
static XStatus ADXL345WriteByte(u32 address, u32 byte);
static XStatus ADXL345ReadByte(u32 address, u32 *byte);
static XStatus ADXL345ReadBytes(u32 address, u32 *bytes, u32 count);
static XStatus ADXL345GetAcceleration(int *x, int *y, int *z);

void TM_Intr_Handler(void *CallbackRef) {
    XTmrCtr *TMRInst_LOCAL = (XTmrCtr*) CallbackRef;
    XTmrCtrStats stats;
    XTmrCtr_DisableIntr(TIMER_BASE_ADDR, 0);

    // Read and clear the interrupt status in the AXI timer immediately
    XTmrCtr_GetStats(TMRInst_LOCAL, &stats);
    XTmrCtr_ClearStats(TMRInst_LOCAL);
    
    g_flag = 1;
    ADXL345GetAcceleration(&g_x, &g_y, &g_z);

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
    float sw_angle, hw_angle;
    float error;
    
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
        while (!g_flag);
        if (g_flag) {
            g_flag = 0;
            sw_angle = (float) atan2((double)g_y, (double)g_z);
            hw_angle = atan2_cordic(g_y, g_z);
            error = fabsf(sw_angle - hw_angle);
            // multiplied by 1000 to see decimals in integer-only xil_printf
            xil_printf("HW: %d mRad, SW: %d mRad, Err: %d mRad\r\n", 
                            (int)(hw_angle*1000), (int)(sw_angle*1000), (int)(error*1000));
        }
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