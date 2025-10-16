// Definitionen f�r die verwendete Hardware
#include <stdbool.h>
// Hardware Initialisierung
void hw_init(void);

// Hardware Serviceroutine get
void hw_sero_get(void);

// Harware Serviceroutine set
void hw_sero_set(void);

uint8_t update_uok(void);
uint8_t u_ok(GPIO_TypeDef *port, uint16_t pin);

void setStartPump(void);
void setStopPump(void);
GPIO_PinState readPumpStatus(void);
uint8_t readPumpWarning(void);
uint8_t readPumpAlarm(void);
uint8_t readPumpRemote(void);
void hw_xport_reset_disable(uint8_t);
void system_powerup_ready_light(void);
