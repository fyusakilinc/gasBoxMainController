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
uint8_t system_powerup_ready_light(void);


// startup/self-test error bits
typedef enum {
    BOOT_ERR_APC  = 1u << 0,  // APC not responding
    BOOT_ERR_RFG  = 1u << 1,  // RFG not responding
    BOOT_ERR_GB   = 1u << 2,  // GasBox not responding
    BOOT_ERR_AIR  = 1u << 3,  // Pressurized air sensor == 0
    BOOT_ERR_PUMP = 1u << 4,  // Pump status == 0
} boot_err_t;

static volatile uint32_t g_boot_errmask = 0;
