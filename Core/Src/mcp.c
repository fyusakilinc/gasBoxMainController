#include "spi.h"
#include "mcp.h"
#include "zentrale.h"
#include "timer0.h"
#include "SG_global.h"    // SG_ERR_* defines

//for iocon.bank == 0
#define MCP_IODIRA   0x00
#define MCP_IODIRB   0x01
#define MCP_IPOLA    0x02
#define MCP_IPOLB    0x03
#define MCP_GPINTENA 0x04
#define MCP_GPINTENB 0x05
#define MCP_DEFVALA  0x06
#define MCP_DEFVALB  0x07
#define MCP_INTCONA  0x08
#define MCP_INTCONB  0x09
#define MCP_IOCON    0x0A   // (same at 0x0B)
#define MCP_GPPUA    0x0C
#define MCP_GPPUB    0x0D
#define MCP_INTFA    0x0E
#define MCP_INTFB    0x0F
#define MCP_INTCAPA  0x10
#define MCP_INTCAPB  0x11
#define MCP_GPIOA    0x12
#define MCP_GPIOB    0x13
#define MCP_OLATA    0x14
#define MCP_OLATB    0x15

// IOCON bits (BANK=0 layout)
#define IOCON_BANK   (0u<<7)
#define IOCON_MIRROR (0u<<6)
#define IOCON_SEQOP  (0u<<5)
#define IOCON_DISSLW (0u<<4)
#define IOCON_HAEN   (1u<<3)  // enable hardware addressing on SPI
#define IOCON_ODR    (0u<<2)
#define IOCON_INTPOL (0u<<1)

#define MCP_ADDR     0x00  // A2..A0
#define MCP_OP_WRITE (0x40 | (MCP_ADDR<<1) | 0)  // 0x40
#define MCP_OP_READ  (0x40 | (MCP_ADDR<<1) | 1)  // 0x41

#define LED_MASK_A  0x0F

static HAL_StatusTypeDef mcp_write_reg(uint8_t reg, uint8_t val);
static HAL_StatusTypeDef mcp_read_reg(uint8_t reg, uint8_t *val);
static HAL_StatusTypeDef mcp_update_bits(uint8_t reg, uint8_t mask, uint8_t value);
static inline void mcp_hb_toggle(void);
static uint8_t s_led_shadow = 0x00;

static inline void MCP_RESET_Pulse(void){
    // Active-low reset pin
    HAL_GPIO_WritePin(UC_AUX_RESET_GPIO_Port, UC_AUX_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(UC_AUX_RESET_GPIO_Port, UC_AUX_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
}

HAL_StatusTypeDef mcp_init(void)
{
    MCP_RESET_Pulse();

    // IOCON: BANK=0, SEQOP=1 (no sequential), HAEN=1 (optional)
    uint8_t iocon = IOCON_BANK | IOCON_MIRROR | IOCON_SEQOP | IOCON_DISSLW | IOCON_HAEN | IOCON_ODR | IOCON_INTPOL;
    HAL_StatusTypeDef st;
    st = mcp_write_reg(MCP_IOCON, iocon); if (st != HAL_OK) return st;

    // Directions: 1=input, 0=output
    // GPA: [7]=SPS_OUT_DISABLE(output=0), [3..0]=LEDs(output=0)
    uint8_t iodira = 0x00;            // all outputs by default
    // If any GPA4..6 should be inputs, set bits.
    st = mcp_write_reg(MCP_IODIRA, iodira); if (st != HAL_OK) return st;

    // GPB: [1]=DIAG_INPUT(input=1), [0]=SPS_DIAG_OUTPUT(output=1)
    uint8_t iodirb = (1u<<1) | (1u<<0);         // only GPB0 and GPB1
    st = mcp_write_reg(MCP_IODIRB, iodirb); if (st != HAL_OK) return st;

    // Pull-ups for inputs (enable for DIAG_INPUT on GPB1)
    st = mcp_write_reg(MCP_GPPUA, 0x00); if (st != HAL_OK) return st;  // none on A
    st = mcp_write_reg(MCP_GPPUB, iodirb); if (st != HAL_OK) return st;

    // Clear outputs to a known state: LEDs off
    st = mcp_write_reg(MCP_OLATA, 0x00); if (st != HAL_OK) return st;

    // toggle´s first time
    (void)mcp_update_bits(MCP_OLATA, LED_MASK_A, s_led_shadow);

    return HAL_OK;
}

void mcp_sero_get(void) {
	// Read DIAG pins (GPB0/1) via MCP; returns 0..3, active-low (0 = fault), 0xFF on read error
	uint8_t raw = mcp_diag_read_b_raw();
	if (raw == 0xFF) return;
	// Raise errors when DIAG lines are low
	if ((raw & (1u << 0)) == 0) {
		z_set_error(SG_ERR_DIAG_OUT);
	}
	if ((raw & (1u << 1)) == 0) {
		z_set_error(SG_ERR_DIAG_IN); // TODO: ask if this is enough or do we need to turn on the interrupt of the mcp
	}
}


void mcp_sero_set(void) {

	if (ct_mcp_hbeat_null() == 1) {          // same timer API
		set_ct_mcp_hbeat(500);               // 500 ms -> ~1 Hz blink
		mcp_hb_toggle();                 // AD5592 heartbeat toggle
	}
}

static inline void mcp_hb_toggle(void){
	s_led_shadow ^= LED_MASK_A;
	// toggle leds for heartbeat
	mcp_update_bits(MCP_OLATA, LED_MASK_A, s_led_shadow & LED_MASK_A);
}

// Returns GPB[1:0] raw (no inversion): bit0=DIAG_OUT, bit1=DIAG_IN
uint8_t mcp_diag_read_b_raw(void)
{
    uint8_t v = 0;
    if (mcp_read_reg(MCP_GPIOB, &v) != HAL_OK) return 0xFF;  // error sentinel
    return v & 0x03; // keep only GPB1..0
}

// SPS_OUT_DISABLE on GPA7 (set 1=disable? depends on wiring)
HAL_StatusTypeDef mcp_set_sps_out_disable(uint8_t disable)   // 0 or 1
{
	HAL_StatusTypeDef st;
    st = mcp_update_bits(MCP_OLATA, (1u<<7), (disable ? 0 : (1u<<7)));
    return st;
}


static HAL_StatusTypeDef mcp_write_reg(uint8_t reg, uint8_t val)
{
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
    uint8_t tx[3] = { MCP_OP_WRITE, reg, val };
    spi_access_device(spi_mcp_cs);
    HAL_StatusTypeDef st = HAL_SPI_Transmit(&hspi1, tx, sizeof(tx), 10);
    spi_release_device(spi_mcp_cs);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    return st;
}

static HAL_StatusTypeDef mcp_read_reg(uint8_t reg, uint8_t *val)
{
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
    uint8_t tx[3] = { MCP_OP_READ, reg, 0x00 };
    uint8_t rx[3] = {0};
    spi_access_device(spi_mcp_cs);
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, tx, rx, sizeof(tx), 10);
    spi_release_device(spi_mcp_cs);
    if (st == HAL_OK) *val = rx[2];
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    return st;
}

static HAL_StatusTypeDef mcp_update_bits(uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t v;
    HAL_StatusTypeDef st = mcp_read_reg(reg, &v);
    if (st != HAL_OK) return st;
    v = (v & ~mask) | (value & mask);
    return mcp_write_reg(reg, v);
}


