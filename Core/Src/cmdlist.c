#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "SG_global.h"
#include "cmdlist.h"

//--- PRIVATE VARIABLES------------------------------------------------------------------------------------------------------
//Definition der Struktur des Elementes der Ascii-Tabelle
typedef struct {
	char cmdline[CMD_LENGTH];           //für ASCII-Befehl
	uint16_t cmdindex;					//interne Befehlnummer
	uint8_t rw;					//gibt an, dass der Befehl READ/WRITE-Operation ist.
} Cmdline_Item;

//Definition der ASCII-Tabelle
static const Cmdline_Item ASCIICmdTable[] = {
		{"GAS:V3",  	CMD_V3_SET },
		{"GAS:V3?", 	CMD_V3_GET },
		{"GAS:V4", 		CMD_V4_SET },
		{"GAS:V4?",		CMD_V4_GET },
		{"GAS:PDE",		CMD_SET_GAS_PDE },
		{"GAS:PDE?",	CMD_GET_GAS_PDE },
		{"GAS:PDE:CLS",	CMD_CLOSE_GAS_PDE },
		{"GAS:PDE:STA?",CMD_STAT_GAS_PDE },
		{"GAS:AIR", 	CMD_SET_GAS_AIR },
		{"GAS:AIR?",	CMD_GET_GAS_AIR },
		{"GAS:AIR:CLS",	CMD_CLOSE_GAS_AIR },
		{"GAS:AIR:STA?",CMD_STAT_GAS_AIR },
		{"GAS:O2", 		CMD_SET_GAS_O2 },
		{"GAS:O2?",		CMD_GET_GAS_O2 },
		{"GAS:O2:CLS",	CMD_CLOSE_GAS_O2 },
		{"GAS:O2:STA?",	CMD_STAT_GAS_O2 },
		{"GAS:MFC4", 	CMD_SET_GAS_4 },
		{"GAS:MFC4?",	CMD_GET_GAS_4 },
		{"GAS:MFC4:CLS",CMD_CLOSE_GAS_4 },
		{"GAS:MFC4:STA?",CMD_STAT_GAS_4 },
		{"PUM", 		CMD_PUMP_SET },   // usage: "PUM 1;" or "PUM 0;"
		{"PUM?", 		CMD_PUMP_GET },   // returns 0/1 (running)
		{"PUM:STA?", 	CMD_PUMP_GET_STA }, // MP start/running status (DI)
		{"PUM:WAR?", 	CMD_PUMP_GET_WAR }, // warning bit (DI)
		{"PUM:ALA?", 	CMD_PUMP_GET_ALA }, // alarm bit (DI)
		{"PUM:REM?", 	CMD_PUMP_GET_RMT }, // remote bit (DI)
		{"CHA:T", 		CMD_SET_T }, 		// set chamber temp
		{"CHA:T?", 		CMD_GET_T }, 		// get chamber temp
		{"APC:CTL",		CMD_APC_CTL},
		{"APC:AMD?",	CMD_APC_AMD_RD },
		{"APC:AMD",		CMD_APC_AMD },
		{"APC:CTL:SEL?",CMD_APC_CTL_SEL_RD },
		{"APC:CTL:SEL",	CMD_APC_CTL_SEL },
		{"APC:ERN",		CMD_APC_ERN_RD },
		{"APC:ERC",		CMD_APC_ERC_RD },
		{"APC:VAL",		CMD_APC_VAL },
		{"APC:VAL?",	CMD_APC_VAL_RD },
		{"APC:POS",		CMD_APC_POS },
		{"APC:POS?",	CMD_APC_POS_RD },
		{"APC:POS:SPD",	CMD_APC_POS_SPD },
		{"APC:POS:SPD?",CMD_APC_POS_SPD_RD },
		{"APC:RAM",		CMD_APC_POS_RAM },
		{"APC:RAM?",	CMD_APC_POS_RAM_RD },
		{"APC:RAM:TI",	CMD_APC_POS_TI },
		{"APC:RAM:TI?",	CMD_APC_POS_TI_RD },
		{"APC:RAM:SLP",	CMD_APC_POS_SLP },
		{"APC:RAM:SLP?",CMD_APC_POS_SLP_RD },
		{"APC:RAM:MD",	CMD_APC_POS_MD },
		{"APC:RAM:MD?",	CMD_APC_POS_MD_RD },
		{"APC:PRE",		CMD_APC_PRE },
		{"APC:PRE?",	CMD_APC_PRE_RD },
		{"APC:PRE:SPD",	CMD_APC_PRE_SPD },
		{"APC:PRE:SPD?",CMD_APC_PRE_SPD_RD },
		{"APC:PRE:UNT",	CMD_APC_PRE_UNT },
		{"APC:PRE:UNT?",CMD_APC_PRE_UNT_RD },
		{"APC:POS:STA", CMD_POS_STA_RD },
		{"ISO:V1",		CMD_ISO_V1 },
		{"ISO:V1?",		CMD_ISO_V1_RD },
		{"REL",			CMD_REL },
		{"REL?",		CMD_REL_RD },
		{"BUZ",			CMD_BUZ },
		{"BUZ?",		CMD_BUZ_RD },
		{"SYS:LED",		CMD_SYS_LED },
		{"SYS:LED?",	CMD_SYS_LED_RD },
		{"PUM:LED",		CMD_PUM_LED },
		{"PUM:LED?",	CMD_PUM_LED_RD },
		{"ATM?",		CMD_ATM_RD },
		{"DOOR:SWM?",	CMD_DOOR_SWM_RD },
		{"AIR?",		CMD_AIR_RD },
		{"FRT:STP?",	CMD_STP_RD },
		{"RF",			CMD_RF },
		{"RF?",			CMD_RF_RD },
		{"SPC:CTL",		CMD_SPC_CTL },
		{"SPC:CTL?",	CMD_SPC_CTL_RD },
		{"SPC:T?",		CMD_SPC_T_RD },
		{"SPC:UDC?",	CMD_SPC_UDC_RD },
		{"PWR:PF?",		CMD_PWR_PF_RD },
		{"PWR:PF",		CMD_PWR_PF },
		{"PWR:PFS?",	CMD_PWR_PFS_RD },
		{"PWR:PMD",		CMD_PWR_PMD },
		{"PWR:PMD?",	CMD_PWR_PMD_RD },
		{"PWR:PMDS?",	CMD_PWR_PMDS_RD },
		{"PWR:PREA",	CMD_PWR_PREA },
		{"PWR:PREA?",	CMD_PWR_PREA_RD },
		{"PWR:PREAS?",	CMD_PWR_PREAS_RD },
		{"PWR:DCB",		CMD_PWR_DCB },
		{"PWR:DCB?",	CMD_PWR_DCB_RD },
		{"PWR:DCBS?",	CMD_PWR_DCBS_RD },
		{"PLS:P?",		CMD_PLS_P_RD },
		{"PLS:P",		CMD_PLS_P },
		{"PLS:LEN?",	CMD_PLS_LEN_RD },
		{"PLS:LEN",		CMD_PLS_LEN },
		{"PLS:PER?",	CMD_PLS_PER_RD },
		{"PLS:PER",		CMD_PLS_PER },
		{"IGN:CL?",		CMD_IGN_CL_RD },
		{"IGN:CL",		CMD_IGN_CL },
		{"IGN:CT?",		CMD_IGN_CT_RD },
		{"IGN:CT",		CMD_IGN_CT },
		{"IGN:I?",		CMD_IGN_I_RD },
		{"IGN:I",		CMD_IGN_I },
		{"IGN:PFI?",	CMD_IGN_PFI_RD },
		{"IGN:PFI",		CMD_IGN_PFI },
		{"IGN:TI?",		CMD_IGN_TI_RD },
		{"IGN:TI",		CMD_IGN_TI },
		{"IGN:TS?",		CMD_IGN_TS_RD },
		{"IGN:TS",		CMD_IGN_TS },
		{"TI:RST?",		CMD_TI_RST_RD },
		{"TI:RST",		CMD_TI_RST },
		{"MAT:MAT:CT?",		CMD_MAT_CT_RD },
		{"MAT:MAT:CT",		CMD_MAT_CT },
		{"MAT:MAT:CTS?",	CMD_MAT_CTS_RD },
		{"MAT:MAT:CL?",		CMD_MAT_CL_RD },
		{"MAT:MAT:CL",		CMD_MAT_CL },
		{"MAT:MAT:CLS?",	CMD_MAT_CLS_RD },
		{"MAT:MAT:MMD?",	CMD_MAT_MMD_RD },
		{"MAT:MAT:MMD",		CMD_MAT_MMD },
		{"MAT:SPC:EEP:PROF?",	CMD_EE_PRF_RD },
		{"MAT:SPC:EEP:LDPROF",	CMD_EE_LDPRF },
		{"MAT:SPC:EEP:DPROF?",	CMD_EE_DPRF_RD },
		{"MAT:SPC:EEP:DPROF",	CMD_EE_DPRF },
		{"MAT:SPC:T?",			CMD_MAT_T_RD },
		{"MAT:SPC:EEP:STPROF",	CMD_EE_STPRF },
		};


//--- FUNKTIONSDEFINITION ------------------------------------------------------------------------------------------------------------------
//Die interne Befehlnummer und READ/WRITE Flag werden für den eingegebenen Befehl zurückgeliefert.
//Hier wird es Binary-Suche-Methode verwendet: d.h. die ASCII-Binary-Tabelle muss zuerst nach den aufsteigenden Alphabeten sortiert werden.
void Binary_Search(uint8_t ncmd, char *key, uint16_t *cmdindex) {
	uint16_t low = 0;
	uint16_t high = ncmd - 1;
	uint16_t mid;
	int sflag;
	uint8_t flag = 0;

	while ((low <= high) && (flag == 0)) {
		mid = ((low + high) >> 1);
		sflag = strcmp(key, (char*) &(ASCIICmdTable[mid].cmdline));

		if (sflag < 0) {
			if (mid != 0) {
				high = mid - 1;
			} else {
				if (low != 0) {
					high = 0;
				} else {
					break;
				}
			}
			flag = 0;
		} else if (sflag == 0) {

			*cmdindex = ASCIICmdTable[mid].cmdindex;

			flag = 1;
		} else {
			low = mid + 1;
			flag = 0;
		}

	};

	if (flag == 0)   //Falls die Tabelle diesen Befehl nicht enthältet,
			{
		*cmdindex = BINARY_INDEX_MAX;
	};
}

//prüfe, ob string integer oder float ist
uint8_t isDec(const char *str) {
	uint8_t decpos = 0;

	if (!*str)
		return 0;

	for (uint8_t i = 0; i < strlen(str); i++) {

		if (str[i] > 47 && str[i] < 58)  //prüfe, ob es integer ist
			continue;
		else if (str[i] == 46 && decpos == 0) //prüfe, ob es eine Kommazahl ist und der Punkt nur einmal vorhanden ist*/
				{
			if (i == 0) {    //.1 = Fehler, 0.1 =richtig
				return 0;
			}
			decpos = 1;
			continue;
		} else if ((str[i] == 43 || str[i] == 45) && i == 0) //prüfe, ob +/- am Anfang ist.
				{
			continue;
		} else
			return 0;
	}

	if (decpos == 0) {
		return 1;  //integer
	} else
		return 2; //float
}

//prüfe, ob string ein hexdezimal Zahle ist und in interger umgewandlt wird.
uint8_t isHex(const char *str, int32_t *val) {

	uint8_t ret = 1;
	*val = 0;

	if (!*str)
		return 0;

	if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X'))) {

		for (uint8_t i = 2; i < strlen(str); i++) {

			if ((str[i] >= '0') && (str[i] <= '9')) {
				*val = *val * 16 + (str[i] - 48);

			} else if ((str[i] >= 'a') && (str[i] <= 'f')) {
				*val = *val * 16 + (str[i] - 87);

			} else if ((str[i] >= 'A') && (str[i] <= 'F')) {
				*val = *val * 16 + (str[i] - 55);
			} else {
				ret = 0;
				*val = 0;
				break;
			}
		}

	} else
		ret = 0;

	return ret;
}

//ermittle die Channelnummer aus dem Befehl. Die Channelnummer ist mit Punkt getrennt.
//Der Befehl wird ohne Channelnummer zurückgeliefert. len = die Laenge des Befehles
uint8_t getChannel(char *str, uint8_t len, int8_t *chanr){

	int8_t pos = -1;
	uint8_t i = 0;


	if (!*str) {
		*chanr = NOCHANZONE;                   //NOCHANNEL;
		return 0;    //Befehlformatfehler
	}

	if ((str[0] >= '0') && (str[0] <= '9')) {
		*chanr = NOCHANZONE;                       //NOCHANNEL;
		return 0; //Kein Befehl mit einer Zahl am Anfang.
	}

	*chanr = 0 ;
	for (i=0; i < len ; i++) {
		if (str[i] == 46){
			if (i == 0) {  //.1 ist nicht erlaubt
				*chanr = NOCHANZONE;                   //NOCHANNEL;
				return 0;
			}
			else if (pos == -1){   //die Position des ersten Punktes
				pos = i;
			}
			else {                    //mehrere Punkte sind nicht erlaubt, wie z.B. Dc.1.2
				*chanr = NOCHANZONE;        //NOCHANNEL;
				return 0;
			}
		}
	}

	if (pos > 0 ) {
		for (i=pos+1; i < len; i++){
			if ((str[i] >= '0') && (str[i] <= '9')) {
				*chanr =*chanr*10+ (str[i]-48);
			}
			else {                            //DC.13abc ist nicht erlaubt
				*chanr = NOCHANZONE;              //NOCHANNEL;
				return 0;
			}
		}
		str[pos] ='\0';
	}
	else *chanr = NOCHANZONE;                          //NOCHANNEL;

	return 1;
}

//ermittle die Zonenummer aus dem Befehl.Die Zonenummer ist direkt
//Der Befehl wird ohne Zonenummer zurückgeliefert. len = die Laenge des Befehles
uint8_t getZone(char *str, uint8_t len, int8_t *zonenr) {

	int8_t pos = -1;
	uint8_t i = 0;

	if (!*str) {
		*zonenr = NOCHANZONE;                //NOZONE;
		return 0;    //Befehlformatfehler
	}

	if ((str[0] >= '0') && (str[0] <= '9')) {
		*zonenr = NOCHANZONE;                         //NOZONE;
		return 0; //Kein Befehl mit einer Zahl am Anfang.
	}

	if (str[0] != ':') {
		*zonenr = NOCHANZONE;                           //NOZONE;
		return 0;    //Kein Konfigurationsbefehl
	}

	for (i = len - 1; i > 0; i--) {
		if ((str[i] >= '0') && (str[i] <= '9')) {
			if (i == len - 1) {
				pos = i;
				*zonenr = str[i] - 48;
			} else {
				if (pos > i) {
					pos = i;
					*zonenr = (str[i] - 48) * 10 + *zonenr;
				}
			}
		} else
			break;
	}

	if (pos > 1) {
		str[pos] = '\0';
	}
	//else if (pos == 1){   //:123 oder :DDSmax3. => nicht erlaubt
	else {
		*zonenr = NOCHANZONE;               //NOZONE;
		return 0;
	}

	return 1;

}

float str2float(const char *str) {
	char c;
	float ret = 0, fac = 1;
	for (c = 9; c & 1; str++) {
		c = *str == '-' ? (c & 6 ? c & 14 : (c & 47) | 36) :
			*str == '+' ? (c & 6 ? c & 14 : (c & 15) | 4) :
			*str > 47 && *str < 58 ? c | 18 :
			(c & 8) && *str == '.' ? (c & 39) | 2 :
			!(c & 2) && (*str == ' ' || *str == '\t') ? (c & 47) | 1 : c & 46;
		if (c & 16) {
			ret = c & 8 ?
					*str - 48 + ret * 10 : (*str - 48) / (fac *= 10) + ret;
		}
	}
	return c & 32 ? -ret : ret;
}

int32_t str2int(const char *str) {
	char c;           // unsigned char
	int32_t ret = 0;
	for (c = 1; c & 1; str++) {
		c = *str == '-' ? (c & 6 ? c & 6 : (c & 23) | 20) :
			*str == '+' ? (c & 6 ? c & 6 : (c & 7) | 4) :
			*str > 47 && *str < 58 ? c | 10 :
			!(c & 2) && (*str == ' ' || *str == '\t') ? (c & 23) | 1 : c & 22;
		if (c & 8) {
			ret = ret * 10 + *str - 48;
		}
	}
	return c & 16 ? -ret : ret;
}

