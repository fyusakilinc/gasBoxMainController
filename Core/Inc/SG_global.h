//-----------------DEVICE DEPENDANT-BEREICH------------------------------------
#ifndef __SG_GLOBAL__
#define __SG_GLOBAL__

#define calmode 0		// This does not automatically set CAL_TABLE in correction.h

#define DEBUGMODE 0
#define DEBUGMODE_MLI 0

// Grenzwerte TODO anpassen

#define SG_T_LIMIT	  	50	// Max. Temperatur
#define SG_U_LIMIT_MAX  51
#define SG_U_LIMIT_MIN  46
#define SG_HMS_LIMIT	3100
#define SG_FS_LIMIT		2048
#define SG_SerialNumber 123456789ul

// Error - Bitdefinitionen
#define SG_ERRPF	0
#define SG_ERRPR	1
#define SG_ERR_U5V		2
#define SG_ERR_U12V		3

#define SG_ERR_PUMP_ALARM	4
#define SG_ERR_PUMP_WARNING	5
#define SG_ERRDTC	6
#define SG_ERRPLS   7

#define SG_ERREN	8
#define SG_ERRFS	9
#define SG_ERRHMS	10

#define SG_ERR_MAXVDDA   	11
#define SG_ERR_ITL    		12
#define SG_ERR_PUMPWARNING	13
#define SG_ERR_PUMPALARM	14

#define SG_ERRCH1	16
#define SG_ERRCH2	17
#define SG_ERRCH3	18
#define SG_ERRCH4	19

#define SG_ERR_DIAG_IN	20
#define SG_ERR_DIAG_OUT	21

#define SG_ERR_TC_SETPOINT_TIMEOUT	22


#define SG_ERR_CHNMASK      0x000F0000
#define SG_CRITICAL_ERRORS	0x00000000	// Derzeit keine kritischen Fehler, die einen Neustart erfordern

// 4095 counts = 70,50 V
#define SG_USMPSFAKTOR		805
#define SG_USMSPFAKTOR_NK	16 

// 4095 counts = 32,29 V
#define SG_U2FAKTOR	    	530
#define SG_U2FAKTOR_NK		16

#define SG_PF_NENN			500
//#define SG_pf_max 		500
#define SG_PF_LIMIT			590
#define SG_PF_LIMIT_PK		590
#define SG_PR_LIMIT			50
#define SG_PR_LIMIT_PK		50
#define SG_PR_LIMIT_ADM		50
#define SG_PR_LIMIT_MIN 	10
#define SG_PR_LIMIT_MAX 	50

#define SG_ONTIME_UP 		99
#define SG_ONTIME_UP_CW		50
#define SG_ONTIME_DOWN 		101
#define SG_ONTIME_CNTMAX	60890000
#define SG_ONTIME_LIMIT		60885000   // -> 30Sekunden Pulslänge +2,5%
#define SG_ONTIME_CNTMAX_CW	60880000   // -> CW löst keinen Fehler aus
#define SG_CW_LIMIT 		985        // -> 30W bei Pn=500W (6%)
#define SG_EN1_REF 			983040000  // -> 500W für 3s ohne Tolerranzen
#define SG_EN2_REF 			983040000  // -> 500W für 3s ohne Tolerranzen
#define SG_EN3_REF 			983040000  // -> 500W für 3s ohne Tolerranzen
#define SG_EN4_REF 			983040000  // -> 500W für 3s ohne Tolerranzen
#define SG_EN1_LIMIT 		1007616000 // -> 500W für 3s +2.5% Toleranz
#define SG_EN2_LIMIT 		1007616000 // -> 500W für 3s +2.5% Toleranz
#define SG_EN3_LIMIT 		1007616000 // -> 500W für 3s +2.5% Toleranz
#define SG_EN4_LIMIT 		1007616000 // -> 500W für 3s +2.5% Toleranz


#define SG_SET_HB_LOW       UC_HEARTBEAT_GPIO_Port->BRR = (uint32_t)UC_HEARTBEAT_Pin
#define SG_SET_HB_HIGH      UC_HEARTBEAT_GPIO_Port->BSRR = (uint32_t)UC_HEARTBEAT_Pin

#define SG_ADM_PIN   		12345ul

#ifndef _STDINT_H
#include <stdint.h>
#endif

typedef struct stack_node
{
    uint8_t    cmd_sender;
    uint8_t    cmd_receiver;
    uint16_t   cmd_index;
    uint8_t    cmd_ack;
    int32_t    parameter;
    uint8_t    next;
    uint8_t    prio;
    uint8_t    rwflg;
}stack_item;

//Definition des Elementes der Prioritätsliste
typedef struct priority_node
{
    uint8_t stackindex;
    uint8_t next;
}priority_item;

//Definitionen der Konstante für den Stacksverwaltung
#define NONEXT              255
#define ZENTRALE            1
#define MATCHINGCUBE        2

#define Z_MATCHINGCUBE      1
#define Z_ZENTRALE          0

#define  PRIO_LEVELS        3
#define  PRIO_LEVEL0        0       //die höchstene Priorität
#define  PRIO_LEVEL1        1       //die mittlene Priorität
#define  PRIO_LEVEL2        2       //die niedrigsten Priorität

#define READ                1
#define WRITE               2

#define NO_QUELLE           0
#define Q_TOUCHPANEL        5
#define Q_RS232_ASCII       4
#define Q_RS232_BINARY      3
#define Q_ANALOG            2
#define Q_ZENTRALE          1

#endif /* __SG_GLOBAL__ */
