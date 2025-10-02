#include "ad5592.h"
#include "gpio.h"
#include "spi.h"
#include "func.h"

extern SPI_HandleTypeDef hspi1;


#define MIO_CHN_COUNT		8
#define MIO_AVRG_LEN		(1<<4)

static uint16_t adc_val[MIO_CHN_COUNT][MIO_AVRG_LEN];
static uint8_t  adc_val_ptr[MIO_CHN_COUNT];
static uint16_t DACS = (1<<MIO_DAC0);
static uint16_t ADCS = (1<<MIO_ADC0);

static inline void MIO_CS_L(void){ HAL_GPIO_WritePin(UC_CS_AUX0_GPIO_Port, UC_CS_AUX0_Pin, GPIO_PIN_RESET); }
static inline void MIO_CS_H(void){ HAL_GPIO_WritePin(UC_CS_AUX0_GPIO_Port, UC_CS_AUX0_Pin, GPIO_PIN_SET); }

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
    mio_send_word(AD5592R_CMD_GPIO_WRITE_CONFIG);
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
}

void mio_sero_set(void)
{
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

		case MIO_ADC1:
			adc_val[MIO_ADC1][adc_val_ptr[MIO_ADC1]] = tmp;
			adc_val_ptr[MIO_ADC1]++;
			if(adc_val_ptr[MIO_ADC1] > MIO_AVRG_LEN)
				adc_val_ptr[MIO_ADC1] = 0;
		break;

		case MIO_ADC2:
			adc_val[MIO_ADC2][adc_val_ptr[MIO_ADC2]] = tmp;
			adc_val_ptr[MIO_ADC2]++;
			if(adc_val_ptr[MIO_ADC2] > MIO_AVRG_LEN)
				adc_val_ptr[MIO_ADC2] = 0;
		break;

		case MIO_ADC3:
			adc_val[MIO_ADC3][adc_val_ptr[MIO_ADC3]] = tmp;
			adc_val_ptr[MIO_ADC3]++;
			if(adc_val_ptr[MIO_ADC3] > MIO_AVRG_LEN)
				adc_val_ptr[MIO_ADC3] = 0;
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

