#include "main.h"
#include "timer0.h"
#include "tim.h"

//-----------------PRIVATE-BEREICH---------------------------------------------
//#define I2C_TIMEOUT_MS  75


volatile static uint16_t ct_start=0;
volatile static uint16_t ct_hbeat=0;
volatile static uint16_t ct_mio_hbeat=0;
volatile static uint16_t ct_mcp_hbeat=0;
volatile static uint16_t ct_init=0;

volatile static uint16_t ct_hbeat_slv=0;

volatile static uint16_t ct_rst_err=0;
volatile static uint16_t ct_rst_dev=0;

volatile static uint16_t ct_ctl=0;
volatile static uint16_t ct_zreset=0;


//-----------------INTERRUPT ROUTINE-------------------------------------------

// Triggered by the main HAL timer ISR every 1 ms
void timer0_incTick(void)
{
	if (ct_start) ct_start--;
    if (ct_hbeat) ct_hbeat--;
    if (ct_mio_hbeat) ct_mio_hbeat--;
    if (ct_mcp_hbeat) ct_mcp_hbeat--;
    if (ct_init) ct_init--;
    if(ct_rst_err) ct_rst_err--;
    if(ct_rst_dev) ct_rst_dev--;
    if(ct_zreset) ct_zreset--;
    if (ct_ctl) ct_ctl--;


}

//-----------------FUNKTIONSDEFINITIONEN---------------------------------------


// START Stoppuhr setzen
void set_ct_start(uint16_t ct_startval)
{
  ct_start = ct_startval;

}


// Stoppuhr X-Port setzen
void set_ct_ctl(uint16_t ct_ctlval)
{
  ct_ctl = ct_ctlval;

}

// Stoppuhr X-Port abfragen
uint8_t ct_ctl_null(void)
{
    uint16_t tmp = ct_ctl;

    if (tmp <= 0) return 1;
    else return 0;
}
// START Stoppuhr abfragen
uint8_t ct_start_null(void)
{
    uint16_t tmp = ct_start;

    if (tmp <= 0) return 1;
    else return 0;
}

// HEARTBEAT Stoppuhr setzen
void set_ct_hbeat(uint16_t ct_hbeatval)
{
    ct_hbeat = ct_hbeatval;

}

// HEARTBEAT Stoppuhr abfragen
uint8_t ct_hbeat_null(void)
{
    uint16_t tmp = ct_hbeat;

    if (tmp <= 0) return 1;
    else return 0;
}

// HEARTBEAT Stoppuhr setzen
void set_ct_mio_hbeat(uint16_t ct_mio_hbeatval)
{
    ct_mio_hbeat = ct_mio_hbeatval;

}

// HEARTBEAT Stoppuhr abfragen
uint8_t ct_mio_hbeat_null(void)
{
    uint16_t tmp = ct_mio_hbeat;

    if (tmp <= 0) return 1;
    else return 0;
}

// HEARTBEAT Stoppuhr setzen
void set_ct_mcp_hbeat(uint16_t ct_mcp_hbeatval)
{
    ct_mcp_hbeat = ct_mcp_hbeatval;

}

// HEARTBEAT Stoppuhr abfragen
uint8_t ct_mcp_hbeat_null(void)
{
    uint16_t tmp = ct_mcp_hbeat;

    if (tmp <= 0) return 1;
    else return 0;
}

// INIT Stoppuhr setzen
void set_ct_init(uint16_t ct_initval)
{
    ct_init = ct_initval;

}

// INIT Stoppuhr abfragen
uint8_t ct_init_null(void)
{
    uint16_t tmp = ct_init;

    if (tmp <= 0) return 1;
    else return 0;
}

// Stoppuhr Reset Errors setzen
void set_ct_rst_err(uint16_t ct_cmdval)
{
  ct_rst_err = ct_cmdval;

}

// Stoppuhr Reset Errors abfragen
uint8_t ct_rst_err_null(void)
{
    uint16_t tmp = ct_rst_err;

    if (tmp <= 0) return 1;
    else return 0;
}

// Stoppuhr Reset Device setzen
void set_ct_rst_dev(uint16_t ct_cmdval)
{
  ct_rst_dev = ct_cmdval;

}

// Stoppuhr Reset Device abfragen
uint8_t ct_rst_dev_null(void)
{
    uint16_t tmp = ct_rst_dev;

    if (tmp <= 0) return 1;
    else return 0;
}

uint8_t ct_zreset_null(void)
{
    uint16_t tmp = ct_zreset;

    if (tmp <= 0) return 1;
    else return 0;
}

void set_ct_zreset(uint16_t ct_cmdval)
{
    ct_zreset = ct_cmdval;
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1) {
	  timer0_incTick();
  }
}













