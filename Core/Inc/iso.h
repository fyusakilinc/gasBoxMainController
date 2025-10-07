#include "main.h"

HAL_StatusTypeDef isoWrite(uint8_t);
uint8_t isoRead(void);

void iso_init(void);

void iso_valve_set(uint8_t value);
void relais_set(uint8_t value);
void buzzer_set(uint8_t value);
void ledbereit_set(uint8_t value);
void ledpumpe_set(uint8_t value);

uint8_t iso_valve_get(void);
uint8_t atm_sensor_get(void);
uint8_t door_switch_get(void);
uint8_t air_sensor_get(void);
uint8_t stop_button_get(void);

uint8_t relais_get(void);
uint8_t buzzer_get(void);
uint8_t ledbereit_get(void);
uint8_t ledpumpe_get(void);

