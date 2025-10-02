#include "main.h"
#include "Hardware.h"
#include "zentrale.h"
#include "timer0.h"
#include "SG_global.h"
#include <stdbool.h>



//-----------------PRIVATE-BEREICH---------------------------------------------

// STM32: Defines and intial things provided by main.h and HAL stuff from MXCube

//-----------------FUNKTIONSDEFINITIONEN---------------------------------------

uint8_t u_ok(GPIO_TypeDef *port, uint16_t pin);
uint8_t update_uok(void);

// Initialisierung der Hardware
void hw_init(void)
{	

}



// Serviceroutine Hadware Get
// Prueft im wesentlichen auf Harwarefehlersignale
void hw_sero_get(void) {
	if (!update_uok()) {
		uint8_t ok5 = u_ok(UC__5VOK_GPIO_Port, UC__5VOK_Pin);
		uint8_t ok12 = u_ok(UC__12VOK_GPIO_Port, UC__12VOK_Pin);

		if (!ok5) {
			z_set_error(SG_ERR_U5V);
		}
		if (!ok12) {
			z_set_error(SG_ERR_U12V);
		}
	}
}

// Serviceroutine Hadware Set
// Verwaltet die Heartbeat LED
void hw_sero_set(void)
{	if (ct_hbeat_null()==1)
	 { set_ct_hbeat(500);
	 HAL_GPIO_TogglePin(UC_HEARTBEAT_GPIO_Port, UC_HEARTBEAT_Pin);
	}
}

void hw_set_error_out(uint8_t p)
{

}

uint8_t u_ok(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_PinState s = HAL_GPIO_ReadPin(port, pin);
    return (s == GPIO_PIN_SET) ? 1u : 0u;
}

uint8_t update_uok(void) {
	uint8_t res1 = u_ok(UC__5VOK_GPIO_Port,UC__5VOK_Pin);
	uint8_t res2 = u_ok(UC__12VOK_GPIO_Port,UC__12VOK_Pin);
	if((res1 && res2)){
		return 1;
	}
	return 0;
}

void setStartPump(bool set) {
    HAL_GPIO_WritePin(UC_PUMP_START_GPIO_Port, UC_PUMP_START_Pin,
                      set ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void setStopPump(bool set) {
    HAL_GPIO_WritePin(UC_PUMP_STOP_GPIO_Port, UC_PUMP_STOP_Pin,
                      set ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// these two functions might just need a pulse

uint8_t readPumpWarning(void) {
	GPIO_PinState s = HAL_GPIO_ReadPin(UC_PUMP_WARNING_GPIO_Port, UC_PUMP_WARNING_Pin);
	return (s == GPIO_PIN_SET) ? 1u : 0u;
}

uint8_t readPumpAlarm(void) {
	GPIO_PinState s = HAL_GPIO_ReadPin(UC_PUMP_ALARM_GPIO_Port, UC_PUMP_ALARM_Pin);
	return (s == GPIO_PIN_SET) ? 1u : 0u;
}

uint8_t readPumpRemote(void) {
	GPIO_PinState s = HAL_GPIO_ReadPin(UC_PUMP_REMOTE_GPIO_Port, UC_PUMP_REMOTE_Pin);
	return (s == GPIO_PIN_SET) ? 1u : 0u;

}




