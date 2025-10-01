#include "ad5592.h"
#include "gpio.h"

extern SPI_HandleTypeDef hspi1;


#define MIO_CHN_COUNT		8
#define MIO_AVRG_LEN		(1<<4)

static inline void MIO_CS_L(void){ HAL_GPIO_WritePin(UC_CS_AUX0_GPIO_Port, UC_CS_AUX0_Pin, GPIO_PIN_RESET); }
static inline void MIO_CS_H(void){ HAL_GPIO_WritePin(UC_CS_AUX0_GPIO_Port, UC_CS_AUX0_Pin, GPIO_PIN_SET); }

static inline void MIO_RESET_Pulse(void){
    // Optional reset pulse like your AVR did
    HAL_GPIO_WritePin(UC_AUX_RESET_GPIO_Port, UC_AUX_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);   // 1 ms is plenty
    HAL_GPIO_WritePin(UC_AUX_RESET_GPIO_Port, UC_AUX_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
}

static inline void init_helper(uint16_t w){
    ad5592_write16(w);
    // Tiny delay not usually needed at SPI speeds, but harmless:
    // for very short µs delays use a NOP loop or leave it out.
}

void ad5592_init(void)
{
    MIO_CS_H();         // idle high
    MIO_RESET_Pulse();  // optional

    init_helper(AD5592R_CMD_SW_RESET | 0x0DAC);

    // Configure channels as you did on AVR:
    init_helper(AD5592R_CMD_ADC_PIN_SELECT | MIO_ADC0);
    init_helper(AD5592R_CMD_DAC_PIN_SELECT | MIO_DAC0);
    init_helper(AD5592R_CMD_PULL_DOWN_SET);
    init_helper(AD5592R_CMD_GPIO_WRITE_CONFIG);
    init_helper(AD5592R_CMD_GPIO_READ_CONFIG);
    init_helper(AD5592R_CMD_GPIO_DRAIN_CONFIG);
    init_helper(AD5592R_CMD_THREE_STATE_CONFIG);
    init_helper(AD5592R_CMD_GP_CNTRL | (0b1100<<6));   // buf precharge + ADC buffer enable

  /*  // Power-down unused DACs (same trick you used)
    uint16_t ref_ctrl_bits =  & 0xFF;
    init_helper(AD5592R_CMD_POWER_DWN_REF_CNTRL | ref_ctrl_bits);

    init_helper(AD5592R_CMD_CNTRL_REG_READBACK);
    init_helper(AD5592R_CMD_ADC_READ | (1u<<9) | ); // program ADC sequence
*/
    // Zero DACs if you want
    ad5592_set_dac(0, 0);
    ad5592_set_dac(2, 0);
    ad5592_set_dac(4, 0);
    ad5592_set_dac(6, 0);
}

// full-duplex 16-bit transfer; rx can be NULL if dont care
static inline HAL_StatusTypeDef ad5592_xfer16(uint16_t tx, uint16_t *rx){
    uint16_t _rx = 0;
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&tx, (uint8_t*)&_rx, 1, 10);
    if (rx) *rx = _rx;
    return st;
}

// write a single 16-bit command (no readback)
void ad5592_write16(uint16_t w){
    MIO_CS_L();
    (void) ad5592_xfer16(w, NULL);
    MIO_CS_H();
}

// write, then immediately read one 16-bit word
static inline uint16_t ad5592_read16(uint16_t w){
    uint16_t r;
    MIO_CS_L();
    (void) ad5592_xfer16(w, &r);
    MIO_CS_H();
    return r;
}

//--- PRIVATE VARIABLES ---------------------------------------------------------------------------------------------------

static uint16_t adc_val[MIO_CHN_COUNT][MIO_AVRG_LEN];
static uint8_t  adc_val_ptr[MIO_CHN_COUNT];
static uint16_t DACS = (1<<MIO_DAC0)|(1<<MIO_DAC1)|(1<<MIO_DAC2)|(1<<MIO_DAC3);
static uint16_t ADCS = (1<<MIO_ADC0)|(1<<MIO_ADC1)|(1<<MIO_ADC2)|(1<<MIO_ADC3);

