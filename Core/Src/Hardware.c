
#include "main.h"
#include "Hardware.h"
#include "zentrale.h"
#include "timer0.h"
#include "iso.h"
#include "rfg.h"
#include "apc.h"
#include "gasbox.h"
#include "SG_global.h"
#include "uart4.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

//-----------------PRIVATE-BEREICH---------------------------------------------

// STM32: Defines and intial things provided by main.h and HAL stuff from MXCube

//-----------------FUNKTIONSDEFINITIONEN---------------------------------------

uint8_t u_ok(GPIO_TypeDef *port, uint16_t pin);
uint8_t update_uok(void);
volatile uint8_t g_ready_should_blink = 0;

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
		uint8_t okvdda = u_ok(MAX_VDDA_OK_GPIO_Port, MAX_VDDA_OK_Pin);
		uint8_t okitl = u_ok(UC_ITL_OK_GPIO_Port,UC_ITL_OK_Pin);
		uint8_t okpumpw = !u_ok(UC_PUMP_WARNING_GPIO_Port,~UC_PUMP_WARNING_Pin);
		uint8_t okpumpa = !u_ok(UC_PUMP_ALARM_GPIO_Port,~UC_PUMP_ALARM_Pin);
		if (!ok5) {
			z_set_error(SG_ERR_U5V);
		}
		if (!ok12) {
			z_set_error(SG_ERR_U12V);
		}
		if (!okvdda) {
			z_set_error(SG_ERR_MAXVDDA);
		}
		if (!okitl) {
			z_set_error(SG_ERR_ITL);
		}
		if (!okpumpw) {
			z_set_error(SG_ERR_PUMPWARNING);
		}
		if (!okpumpa) {
			z_set_error(SG_ERR_PUMPALARM);
		}
	}
}

// Serviceroutine Hadware Set
// Verwaltet die Heartbeat LED
void hw_sero_set(void) {
	if (ct_hbeat_null() == 1) {
		set_ct_hbeat(500);
		HAL_GPIO_TogglePin(UC_HEARTBEAT_GPIO_Port, UC_HEARTBEAT_Pin);

		if (g_ready_should_blink) {
			static uint8_t on = 0;
			on ^= 1u;
			ledbereit_set(on);           // blink when NOT ready
		} else {
			ledbereit_set(1);            // hold solid when ready
		}
	}
}


uint8_t u_ok(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_PinState s = HAL_GPIO_ReadPin(port, pin);
    return (s == GPIO_PIN_SET) ? 1u : 0u;
}

uint8_t update_uok(void) {
	uint8_t res1 = u_ok(UC__5VOK_GPIO_Port,UC__5VOK_Pin);
	uint8_t res2 = u_ok(UC__12VOK_GPIO_Port,UC__12VOK_Pin);
	uint8_t res3 = u_ok(MAX_VDDA_OK_GPIO_Port,MAX_VDDA_OK_Pin);
	uint8_t res4 = u_ok(UC_ITL_OK_GPIO_Port,UC_ITL_OK_Pin);
	uint8_t res5 = !u_ok(UC_PUMP_WARNING_GPIO_Port, UC_PUMP_WARNING_Pin);
	uint8_t res6 = !u_ok(UC_PUMP_ALARM_GPIO_Port, UC_PUMP_ALARM_Pin);
	if((res1 && res2 && res3 && res4 && res5 && res6)){
		return 1;
	}
	return 0;
}

void setStartPump(void) {
	if(readPumpRemote() == GPIO_PIN_RESET) // is it set or reset TODO
		HAL_GPIO_WritePin(UC_PUMP_START_GPIO_Port, UC_PUMP_START_Pin, GPIO_PIN_SET);
}

void setStopPump(void) {
	if (readPumpRemote() == GPIO_PIN_RESET)
		HAL_GPIO_WritePin(UC_PUMP_START_GPIO_Port, UC_PUMP_START_Pin, GPIO_PIN_RESET);
}

GPIO_PinState readPumpStatus(void) {
	GPIO_PinState status = 0;
	if (readPumpRemote() == GPIO_PIN_RESET)
		status = HAL_GPIO_ReadPin(UC_PUMP_STATUS_GPIO_Port, UC_PUMP_STATUS_Pin);
	return status;
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

void hw_xport_reset_disable(uint8_t val) {
    // val = TRUE um der export nicht mehr zu resetten
    // reset ist LOW-aktiv, wird auch LOW von HAL initialisiert.
    if(val)
        HAL_GPIO_WritePin(XPORT_RESET_GPIO_Port, XPORT_RESET_Pin, 1);
    else
        HAL_GPIO_WritePin(XPORT_RESET_GPIO_Port, XPORT_RESET_Pin, 0);
}

uint8_t check_gasbox_init();
uint8_t check_rfg_init();
uint8_t check_apc_init();
void notify_pc(uint32_t m);

void system_powerup_ready_light(void)
{
    // 1) all UART devices ok?
	uint8_t gbinit = check_gasbox_init();
	uint8_t rfginit = check_rfg_init();
	uint8_t apcinit = check_apc_init();
    uint8_t air_ok = air_sensor_get() ? 1u : 0u;
    uint8_t pump_ok = (readPumpStatus() == GPIO_PIN_SET) ? 1u : 0u;

    //uint8_t comms_ok = apcinit && rfginit && gbinit;
    // 3) pump running? (GPIO == SET)


    ledpumpe_set(pump_ok ? 1 : 0);  // pump LED (white)

    uint32_t m = 0;
    if (!apcinit)  m |= BOOT_ERR_APC;
    if (!rfginit)  m |= BOOT_ERR_RFG;
    if (!gbinit)   m |= BOOT_ERR_GB;
    if (!air_ok)   m |= BOOT_ERR_AIR;
    if (!pump_ok)  m |= BOOT_ERR_PUMP;
    g_boot_errmask = m;  // publish latest state

    notify_pc(g_boot_errmask);

    if (m == 0) {
		g_ready_should_blink = 0;
		ledbereit_set(1);               // solid green
	} else {
		g_ready_should_blink = 1;       // heartbeat will blink it
		ledbereit_set(0);               // start from OFF so blink is visible
	}

}

uint8_t check_gasbox_init() {
	GbReply r;
	if (!gasbox_xfer(0x01u, 0, &r, 50))
		return 0;
	return (r.status == 0x80);
}

uint8_t check_rfg_init() {
	return rfg_xfer(";;", 0, 0, 50, NULL);
}

uint8_t check_apc_init() {
	double dummy;
	return apc_get_valv_num(&dummy);   // 200 ms is a safe default
}

static void boot_err_to_text(uint32_t m, char *buf, size_t n)
{
    if (!buf || n==0) return;
    if (m == 0) { snprintf(buf, n, "OK"); return; }
    buf[0] = 0;
    #define APP(tag,bit) do{ if (m & (bit)) { if (buf[0]) strncat(buf,",",n-1); strncat(buf,(tag),n-1);} }while(0)
    APP("APC",  BOOT_ERR_APC);
    APP("RFG",  BOOT_ERR_RFG);
    APP("GBOX", BOOT_ERR_GB);
    APP("AIR",  BOOT_ERR_AIR);
    APP("PUMP", BOOT_ERR_PUMP);
    #undef APP
}

void notify_pc(uint32_t m){
	if(m == 0)
		return;
	char text[64];
	boot_err_to_text(m, text, sizeof text);

	char line[96];
	int n = snprintf(line, sizeof line, "EVT:BOOT %s;", text);
	if (n > 0) {
		uartRB_Puts(&usart2_rb, line);
	}
}


