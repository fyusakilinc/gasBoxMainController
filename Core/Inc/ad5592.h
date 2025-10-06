
#include "spi.h"

#ifndef AD5592_H_
#define AD5592_H_

#define MIO_DO_LED			2
#define MIO_HBLED			2

#define MIO_ADC0			0
#define MIO_DAC0			1


#define AD5592R_INT_REF_mV          2500
#define AD5592R_MIN_EXT_REF_mV      1000

#define AD5592R_MIN_VDD_mV          2700 /* Minimum supply voltage */
#define AD5592R_MAX_VDD_mV          5500 /* Maximum supply voltage*/

#define AD5592R_SAMPLE_CODE_MIN     0
#define AD5592R_SAMPLE_CODE_MAX     500


#define AD5592R_CNTRL_ADDRESS_MASK      0x7800  /* Control register bit mask */

#define AD5592R_CMD_NOP                 0x0000  /* No operation */
#define AD5592R_CMD_DAC_READBACK        0x0800  /* Selects and enables DAC read back */
#define AD5592R_CMD_ADC_READ            0x1000  /* Selects ADCs for conversion */
#define AD5592R_CMD_GP_CNTRL            0x1800  /* General purpose control register */
#define AD5592R_CMD_ADC_PIN_SELECT      0x2000  /* Selects which pins are ADC inputs */
#define AD5592R_CMD_DAC_PIN_SELECT      0x2800  /* Selects which pins are DAC outputs */
#define AD5592R_CMD_PULL_DOWN_SET       0x3000  /* Selects which pins have 85kOhm pull-down resistor to GND */
#define AD5592R_CMD_CNTRL_REG_READBACK  0x3800  /* Read back control registers and/or set LDAC */
#define AD5592R_CMD_GPIO_WRITE_CONFIG   0x4000  /* Selects which pins are GPIO outputs */
#define AD5592R_CMD_GPIO_WRITE_DATA     0x4800  /* Writes data to the GPIO outputs */
#define AD5592R_CMD_GPIO_READ_CONFIG    0x5000  /* Selects which pins are GPIO inputs */
#define AD5592R_CMD_GPIO_READ_INPUT     0x5400  /* Read GPIO inputs */
#define AD5592R_CMD_POWER_DWN_REF_CNTRL 0x5800  /* Powers down DACs and enables/disables the reference */
#define AD5592R_CMD_GPIO_DRAIN_CONFIG   0x6000  /* Selects open-drain or push/pull for GPIO outputs */
#define AD5592R_CMD_THREE_STATE_CONFIG  0x6800  /* Selects which pins are three-state */
#define AD5592R_CMD_SW_RESET            0x7DAC  /* Software reset of the AD5592R */

#define AD5592R_PIN_SELECT_MASK         0x00FF  /* Pin select bit mask */

/* DAC registers */
#define AD5592R_DAC_WRITE_MASK          0x8000  /* DAC write bit mask */
#define AD5592R_DAC_ADDRESS_MASK        0x7000  /* DAC pin address bit mask */
#define AD5592R_DAC_VALUE_MASK          0x0FFF  /* DAC output value bit mask */

/* Range selection 2xVref */
#define AD5592R_ADC_TT_VREF             0x0010  /* Set ADC input range to 2 times Vref */
#define AD5592R_DAC_TT_VREF             0x0020  /* Set DAC output range to 2 times Vref */

#define TMIN 							0
#define TMAX							100    /* max and min t values */
#define VREF 							4095


//--- PUBLIC FUNKTIONSDEKLARATION -------------------------------------------------------------------------------------------
// Initialize the Modules
void mio_init(void);

// Service-routine
void mio_sero_get(void);
void mio_sero_set(void);
void mio_init_set(void);
void mio_set_do(uint8_t, uint8_t);
void mio_set_dac(uint8_t, uint16_t);

uint16_t mio_get_adcval_filt(uint8_t);
uint16_t mio_readback_dac(uint8_t);
uint16_t read_adc_single(uint8_t);

int set_TC_STP(float SP);
float get_TIST(void);


#endif /* AD5592_H_ */
