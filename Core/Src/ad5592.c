#include "ad5592.h"
#include "gpio.h"
#include "spi.h"
#include "func.h"
#include "tim.h"
#include "timer0.h"
#include "zentrale.h"
#include "SG_global.h"
#include <math.h>
extern SPI_HandleTypeDef hspi1;


#define MIO_CHN_COUNT		8
#define MIO_AVRG_LEN		(1<<4)

static uint16_t adc_val[MIO_CHN_COUNT][MIO_AVRG_LEN];
static uint8_t  adc_val_ptr[MIO_CHN_COUNT];
static uint16_t DACS = (1<<MIO_DAC0);
static uint16_t ADCS = (1<<MIO_ADC0);
#define MIO_GPIO_HB_PIN   3
#define MIO_GPIO_HB_BIT   (1u << MIO_GPIO_HB_PIN)
static uint8_t mio_gpio_out_shadow = 0x00; // shadow of AD5592 GPIO outputs


static inline void MIO_RESET_Pulse(void){
    // Optional reset pulse like your AVR did
    HAL_GPIO_WritePin(UC_AUX_RESET_GPIO_Port, UC_AUX_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);   // 1 ms is plenty
    HAL_GPIO_WritePin(UC_AUX_RESET_GPIO_Port, UC_AUX_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
}

static inline void mio_send_word(uint16_t w){
	spi_access_device(spi_mio);
	spiTransferWord(w);
	spi_release_device(spi_mio);
}

void mio_init(void)
{
    MIO_RESET_Pulse();  // optional
    delay_us(10);

    mio_send_word(AD5592R_CMD_SW_RESET | 0x0DAC);
    // Configure channels on AVR:
    mio_send_word(AD5592R_CMD_ADC_PIN_SELECT | ADCS);
    mio_send_word(AD5592R_CMD_DAC_PIN_SELECT | DACS);
    mio_send_word(AD5592R_CMD_PULL_DOWN_SET);
    mio_send_word(AD5592R_CMD_GPIO_WRITE_CONFIG | MIO_GPIO_HB_BIT);
    mio_send_word(AD5592R_CMD_GPIO_READ_CONFIG);
    mio_send_word(AD5592R_CMD_GPIO_DRAIN_CONFIG);
    mio_send_word(AD5592R_CMD_THREE_STATE_CONFIG);
    mio_send_word(AD5592R_CMD_GP_CNTRL | (0b1100<<6));   // buf precharge + ADC buffer enable

    // Power-down unused DACs (same trick you used)
    uint16_t ref_ctrl_bits = ~(DACS) & 0xFF;
    mio_send_word(AD5592R_CMD_POWER_DWN_REF_CNTRL | ref_ctrl_bits);

    mio_send_word(AD5592R_CMD_CNTRL_REG_READBACK);
    mio_send_word(AD5592R_CMD_ADC_READ | (1u<<9) | ADCS); // program ADC sequence

	// Felder löschen
	for(uint8_t i=0; i < MIO_CHN_COUNT; i++)
	{
		adc_val_ptr[i] = 0;

		for (uint8_t j=0; j < MIO_AVRG_LEN; j++)
		{
			adc_val[i][j] = 0;
		}
	}
    // init set
	mio_set_dac(MIO_DAC0, 0);
	// Set initial GPIO output levels (HB off -> 0)
	mio_send_word(AD5592R_CMD_GPIO_WRITE_DATA | mio_gpio_out_shadow);
}

void mio_hb_toggle(void) {
    mio_gpio_out_shadow ^= MIO_GPIO_HB_BIT;
    mio_send_word(AD5592R_CMD_GPIO_WRITE_DATA | (mio_gpio_out_shadow & 0xFF));     // single 16-bit write to AD5592
}

void mio_sero_set(void) {
	if (ct_mio_hbeat_null() == 1) {          // same timer API
		set_ct_mio_hbeat(500);               // 500 ms -> ~1 Hz blink
		mio_hb_toggle();                 // AD5592 heartbeat toggle
	}
}

void mio_sero_get(void)
{
	// Step-by-step reading of the analog signals. One signal per call (4 in total)

	uint8_t chn = 0;
	uint16_t tmp = 0;

	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	spi_access_device(spi_mio);

	tmp = spiTransferWord(0); // Get result

	spi_release_device(spi_mio);
	HAL_NVIC_EnableIRQ(SPI1_IRQn);

	chn = ((tmp & 0x7000) >> 12);
	tmp &= AD5592R_DAC_VALUE_MASK;

	switch(chn)
	{
		case MIO_ADC0:
			adc_val[MIO_ADC0][adc_val_ptr[MIO_ADC0]] = tmp;
			adc_val_ptr[MIO_ADC0]++;
			if(adc_val_ptr[MIO_ADC0] > MIO_AVRG_LEN)
				adc_val_ptr[MIO_ADC0] = 0;
		break;

		default:
		break;
	}

	// From here, process all 10ms values further and provide query functions
}

void mio_set_dac(uint8_t channel, uint16_t val) {
	uint16_t tmp = 0;

	tmp = (channel & 0x7);
	tmp <<= 12;
	if (val > AD5592R_DAC_VALUE_MASK)
		tmp += AD5592R_DAC_VALUE_MASK;
	else
		tmp += val;
	//tmp += (val & 0x0FFF);
	tmp |= (AD5592R_DAC_WRITE_MASK);
	HAL_NVIC_DisableIRQ(SPI1_IRQn); 	// atomic restorate
	mio_send_word(tmp);					// transfer the word
	HAL_NVIC_EnableIRQ(SPI1_IRQn);
}

uint16_t read_adc_single(uint8_t chn) {
	uint16_t val;
	uint16_t cmd = AD5592R_CMD_ADC_READ | (chn & 0x7);  // Select single channel

	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	spi_access_device(spi_mio);

	spiTransferWord(cmd);     // Trigger read for channel
	val = spiTransferWord(0); // Get result

	spi_release_device(spi_mio);
	HAL_NVIC_EnableIRQ(SPI1_IRQn);

	return val;
}

uint16_t mio_get_adcval_filt(uint8_t chn)
{
	uint32_t erg = 0;

	for(uint8_t i=0; i<MIO_AVRG_LEN; i++)
	{
		erg += adc_val[chn][i];
	}

	switch(MIO_AVRG_LEN)
	{	case 1: 	return erg; 		break;
		case 2: 	return (erg>>1); 	break;
		case 4: 	return (erg>>2); 	break;
		case 8: 	return (erg>>3); 	break;
		case 16: 	return (erg>>4); 	break;
		default: 	return 0; 			break;
	}
}

uint16_t mio_readback_dac(uint8_t ch) {
	uint16_t val;
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	// --- 1) Turn off any ongoing ADC sequence ---
	spi_access_device(spi_mio);      // CS low

	// --- 2) Enable DAC readback for channel ch ---
	uint16_t cmd = AD5592R_CMD_DAC_READBACK       // REG_ADDR = DAC_RD
	| (0b11 << 3)        // DAC_RD_EN = 11
			| (ch);      // DAC_CH_SEL = ch
	spiTransferWord(cmd);

	// --- 3) Immediately clock out the DAC value ---
	val = spiTransferWord(0x0000);

	spi_release_device(spi_mio);      // CS high
	HAL_NVIC_EnableIRQ(SPI1_IRQn);
	return val;  // only the lower 12 bits are valid
}

int set_TC_STP(float SP) {
	float frac = ((float)SP - TMIN) / (TMAX - TMIN);
	if (frac < 0)
		frac = 0;
	else if (frac > 1)
		frac = 1;

	uint16_t stp_ad = (uint16_t) lroundf(frac * 0xFFF);
	mio_set_dac(MIO_DAC0, stp_ad);


	// from here can be commented out, this reads the data for timeout ms and raises an error if tol not achieved for 3 ok read
	uint16_t timeout_ms = 1000;
	float tolC = 0.5f;
	uint32_t t0 = HAL_GetTick();
	uint8_t ok = 0;
	while ((HAL_GetTick() - t0) < timeout_ms) {
		float pv = (float) get_TIST();
		if (fabsf(pv - SP) <= tolC) {
			if (++ok >= 3) // needs to read for 3 times
				return 1;  // success
		} else {
			ok = 0; // not stable yet, reset consecutive counter
		}
		HAL_Delay(5);
	}

	z_set_error(SG_ERR_TC_SETPOINT_TIMEOUT);
	return 0;

}

float get_TIST(void) {
	uint16_t read = mio_get_adcval_filt(MIO_ADC0);
	float frac = (float)read / 0xFFF;
	if (frac < 0)
			frac = 0;
		else if (frac > 1)
			frac = 1;
	float tist = (float) lroundf(frac * (TMAX-TMIN) + TMIN);

	return tist;
}

