#include <string.h>
#include <stdio.h>
#include "remote.h"
#include "uart4.h"
#include "usart.h"
#include "protocoll.h"
#include "stacks.h"
#include "resultqueue.h"
#include "zentrale.h"
#include "cmdlist.h"
#include "SG_global.h"

// ---- framing ----
#define RMT_MAX_PAKET_LENGTH  14
#define RMT_DLE               0x3D   // '='
#define RMT_SOT               0x53   // 'S'
#define RMT_EOT               0x45   // 'E'

// parser states
#define RMT_WAIT_FOR_PAKET_START  1
#define RMT_READ_PAKET            2
#define RMT_PARSE_PAKET           3

//----- PRIVATE DEFINES -------------------------------------------------------
// die Zustandsdefinitionen fuer die Zustandsautomaten im ASCII-PROTOKOLL
#define get_cmd 1
#define get_sign 2
#define get_val 3
#define proc_cmd 4

//die max.Größe der ASCII-Tabelle
#define ASCII_CMD_MAX			29
#define BINARY_INDEX_MAX		256
#define CMD_LENGTH				19

//--- PRIVATE VARIABLES------------------------------------------------------------------------------------------------------
//Definition der Struktur des Elementes der Ascii-Tabelle
typedef struct {
	char cmdline[CMD_LENGTH];           //für ASCII-Befehl
	uint16_t cmdindex;					//interne Befehlnummer
//uint8_t cmdattribut;				//gibt an, dass der Befehl READ/WRITE-Operation ist.
} Cmdline_Item;

//Definition der ASCII-Tabelle
static const Cmdline_Item ASCIICmdTable[] = {
		{"DO1?", CMD_DO1_GET },
		{"DO1",  CMD_DO1_SET },   // use "DO1 1;" or "DO1 0;" for write
		{"DO2?", CMD_DO2_GET },
		{"DO2",  CMD_DO2_SET },
		{"FLOW1", CMD_MFC1_SET },
		{"FLOW1?",CMD_MFC1_GET }, // add more: "VALV1?", "PUMP:ALARM?", etc.
		{"FLOW2", CMD_MFC2_SET },
		{"FLOW2?",CMD_MFC2_GET },
		{"FLOW3", CMD_MFC3_SET },
		{"FLOW3?",CMD_MFC3_GET },
		{"FLOW4", CMD_MFC4_SET },
		{"FLOW4?",CMD_MFC4_GET },
		};

//Definition der Einstellungsparameter für die Kommunikation mit ASCII-PROTOKOLL
volatile static uint8_t verbose = 0;
volatile static uint8_t crlf = 0;
volatile static uint8_t echo = 0;
volatile static uint8_t sloppy = 0;

// ---- internal parser storage ----
static volatile uint8_t msg[RMT_MAX_PAKET_LENGTH + 1];
static volatile uint8_t nzeichen = 0;       // bytes buffered from UART ring
static uint8_t state = RMT_WAIT_FOR_PAKET_START;

static uint8_t bufferRx[RMT_MAX_PAKET_LENGTH + 1];
static uint8_t lengthRx = 0;
static uint8_t dleFlag = 0;
static uint8_t checksum = 0;

// forward
void parse_ascii(void);
void output_ascii(int32_t);
void Binary_Search(uint8_t ncmd, char *key, uint16_t *cmdindex);
void output_ascii_cmdack(uint8_t verbose_flg, uint8_t crlf_flg, uint8_t cmd_ack);

// ---- public API ----
void remote_init(void) {
	nzeichen = 0;
	state = RMT_WAIT_FOR_PAKET_START;
	lengthRx = 0;
	dleFlag = 0;
	checksum = 0;
	memset((void*) msg, 0, sizeof(msg));
	memset(bufferRx, 0, sizeof(bufferRx));
}

// Pull bytes from UART4 RX ring into msg[] and feed parser
void remote_sero_get(void) {
	nzeichen = 0;
	while ((rb_rx_used(&usart3_rb) > 0) && (nzeichen < RMT_MAX_PAKET_LENGTH)) {
		msg[nzeichen++] = (uint8_t) uartRB_Getc(&usart3_rb);   // legacy getc()
	}
	if (nzeichen)
		parse_ascii();
}

// das Paket in ASCII-Format analysieren und das Paket in den Stack einfügen.
void parse_ascii(void) {

	static char cmd[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0 };
	static char char1[2] = { 0, 0 };
	volatile static uint8_t ret = 0;
	volatile static uint8_t nc = 0;
	volatile static int32_t val = 0;
	volatile static uint8_t a_state = get_cmd;

	volatile static uint8_t pflag = 0;
	volatile static uint8_t eflag = 0;

	static uint8_t negativ_zahl = 0;

	uint8_t ptr = 0;
	uint16_t cmd_index = BINARY_INDEX_MAX;

	stack_item stack_data;

	do {
		// wenn es ein Zeichen in UART1 Buffer gibt und die Automate nicht im Bearbeitungszustand ist
		if ((nzeichen > 0) && (a_state != proc_cmd)) {
			nc = msg[ptr++];				// hole ein Zeichen aus msg-buffer
			if (echo == 1)// Wenn die Kommunikationsmode im Echo-Mode ist, wird es das Zeichen direkt wieder zurückgeben.
					{
				char1[0] = nc;
				uartRB_Put(&usart3_rb, char1, 1);
			}
		} else
			nc = 0;

		if (strlen(cmd) > 22)// Ascii-Kommando darf nicht länger als 22 Zeichen.
				{
			strcpy(cmd, "");
			val = 0;
			pflag = 0;
			eflag = 0;
			a_state = get_cmd;
		}

		switch (a_state) {
		case get_cmd:
			if (((nc >= 65) && (nc <= 90)) || ((nc >= 48) && (nc <= 57))
					|| (nc == 58) || (nc == 63)) {
				char1[0] = nc;				//verkette char1 an cmd
				strcat(cmd, char1);
			} else if ((nc == 46) && (sloppy == 1)) {
				char1[0] = 58;
				strcat(cmd, char1);
			} else if ((nc == 35) && (sloppy == 1)) {
				char1[0] = 63;
				strcat(cmd, char1);
			} else if ((nc >= 97) && (nc <= 122)) {
				char1[0] = (nc - 32);
				strcat(cmd, char1);
			} else if (nc == 32) {
				if (strlen(cmd) > 0) {
					val = 0;
					a_state = get_sign;
				}
			} else if ((nc == 59) || ((nc == 13) && (sloppy == 1))) {
				a_state = proc_cmd;
			} else {
				if (nc != 0)
					strcat(cmd, "*");
			}
			break;

		case get_sign:
			if (nc == 45) {
				negativ_zahl = 1;
				a_state = get_val;
				break;
			} else if ((nc >= 48) && (nc <= 57) && (val < INT32_MAX)) //val < 1000000
					{
				val = val * 10 + (nc - 48);
				pflag = 1;
				a_state = get_val;
				break;
			} else if ((nc == 59) || (nc == 13)) {
				a_state = proc_cmd;
			} else {
				if (nc != 0) {
					negativ_zahl = 0;
					val = 0;
					pflag = 0;
					eflag = 1;
				}

			}
			;
			break;
		case get_val:

			if ((nc >= 48) && (nc <= 57) && (val < INT32_MAX))   //val < 1000000
					{
				val = val * 10 + (nc - 48);
				pflag = 1;
			} else if ((nc == 59) || (nc == 13)) {
				if (negativ_zahl) {
					if (pflag == 0) {
						negativ_zahl = 0;
						val = 0;
						pflag = 0;
						eflag = 1;
					}
				}
				a_state = proc_cmd;
			} else {
				if (nc != 0) {
					negativ_zahl = 0;
					val = 0;
					pflag = 0;
					eflag = 1;
				}

			}
			break;

		case proc_cmd:

			if (eflag == 1) {
				ret = CMR_MALFORMATTEDCOMMAND;
			}
			//hier beginnt die Verarbeitung der Befehle zum Einstellen der Kommunikation
			else if (strcmp(cmd, "VERB") == 0) {
				if (pflag == 0) {
					ret = CMR_MISSINGPARAMETER;
				} else {
					switch (val) {
					case 0:
						verbose = 0;
						ret = CMR_SUCCESSFULL;
						break;
					case 1:
						verbose = 1;
						ret = CMR_SUCCESSFULL;
						break;
					case 2:
						verbose = 2;
						ret = CMR_SUCCESSFULL;
						break;
					default:
						ret = CMR_PARAMETERINVALID;
						break;
					};
				}
			} else if (strcmp(cmd, "ECHO") == 0) {
				if (pflag == 0) {
					ret = CMR_MISSINGPARAMETER;
				} else {
					if (val == 0) {
						echo = 0;
						ret = CMR_SUCCESSFULL;
					} else if (val == 1) {
						echo = 1;
						ret = CMR_SUCCESSFULL;
					} else {
						ret = CMR_PARAMETERINVALID;
					}
				}
			} else if (strcmp(cmd, "CRLF") == 0) {
				if (pflag == 0) {
					ret = CMR_MISSINGPARAMETER;
				} else {
					if (val == 0) {
						crlf = 0;
						ret = CMR_SUCCESSFULL;
					} else if (val == 1) {
						crlf = 1;
						ret = CMR_SUCCESSFULL;
					} else if (val == 2) {
						crlf = 2;
						ret = CMR_SUCCESSFULL;
					} else if (val == 3) {
						crlf = 3;
						ret = CMR_SUCCESSFULL;
					} else {
						ret = CMR_PARAMETERINVALID;
					}
				}
			} else if (strcmp(cmd, "SLOPPY") == 0) {
				if (pflag == 0) {
					ret = CMR_MISSINGPARAMETER;
				} else {
					if (val == 0) {
						sloppy = 0;
						ret = CMR_SUCCESSFULL;
					} else if (val == 1) {
						sloppy = 1;
						ret = CMR_SUCCESSFULL;
					} else {
						ret = CMR_PARAMETERINVALID;
					}
				}
			} else if (strcmp(cmd, "IBL") == 0) {
				verbose = 2;
				echo = 1;
				crlf = 3;
				sloppy = 1;
				ret = CMR_SUCCESSFULL;
			} else if (strcmp(cmd, "") == 0) {
				ret = CMR_SEMICOLONONLY;
			}
			//die anderen ASCII-Befehle werden per Binäre-Suche-Funktion eine interne Befehlnummer und ein Attribut zugeordnet
			//und in den Stack eingefügt.
			else {
				Binary_Search(ASCII_CMD_MAX, cmd, &cmd_index);
				//uart0_puts(cmd);
				//uart0_puti(cmd_index);

				if (cmd_index != BINARY_INDEX_MAX) {
					stack_data.cmd_sender = Q_RS232_ASCII;
					stack_data.cmd_index = cmd_index;
					stack_data.cmd_ack = 0;
					stack_data.next = NONEXT;
					stack_data.prio = PRIO_LEVEL1;

					if (cmd_index & 1) {

						if ((pflag == 1)) {
							if (negativ_zahl) {
								stack_data.parameter = -val;
							} else {
								stack_data.parameter = val;
							}

							stack_data.rwflg = WRITE;
							ret = stack_insert_sero(stack_data);
							//uart0_puti(ret);
							//uart0_puti(stack_data.cmd_index);

						} else if (cmd_index == CMD_RESET_ERROR) {
							stack_data.parameter = 0;
							stack_data.rwflg = WRITE;
							ret = stack_insert_sero(stack_data);
						} else {
							{
								ret = CMR_MISSINGPARAMETER;
							}
						};
					} else  //Lese-Operation
					{
						stack_data.parameter = 0;
						stack_data.rwflg = READ;
						ret = stack_insert_sero(stack_data);
					};

				} else //Falls cmd_index = ASCII_CMD_MAX, d.h. der Befehl ist ungültig.
				{
					ret = CMR_UNKNOWNCOMMAND;
				};
			}
			;

			if (ret != STACK_CMDINSTACK) {
				output_ascii_cmdack(verbose, crlf, ret);
			}
			;

			uartRB_KickTx(&usart3_rb);
			strcpy(cmd, "");
			val = 0;
			ret = 0;
			pflag = 0;
			eflag = 0;
			negativ_zahl = 0;

			a_state = get_cmd;
			break;

		default:
			a_state = get_cmd;
			break;

		};

		//	char1[0]=nc;
		//  if(nzeichen >0)	uart1_put(char1, 1);
		//	versandstart1();

	} while (ptr < nzeichen);
}

void serialSendAnswer(uint8_t *message) {
	uint8_t i;
	uint8_t n = 0;
	uint8_t checksum = 0;
	char buffer[RMT_MAX_PAKET_LENGTH + 1];

	if ((message[2] == 0x00) || (message[2] == 0x02) || (message[2] == 0x03)
			|| (message[2] == 0x0A)) //für die Kompabilität vom altem MatchingCube-Programm. später zu löschen
			{
		message[2] |= 0x80;
	}

	buffer[n++] = RMT_DLE;
	buffer[n++] = RMT_SOT;
	for (i = 0; i < (CMR_DATAPAKET_LENGTH - 1); i++) {
		buffer[n++] = message[i];
		if (message[i] == RMT_DLE) {
			buffer[n++] = message[i];
			// Die Prüfsumme erstreckt sich nur noch über die NETTO-Payload!
			//checksum += message[i];
		}
		checksum += message[i];
	}
	buffer[n++] = checksum;
	if (checksum == RMT_DLE) {
		buffer[n++] = checksum;
	}
	buffer[n++] = RMT_DLE;
	buffer[n++] = RMT_EOT;

	uartRB_Put(&usart3_rb, buffer, n);
	uartRB_KickTx(&usart3_rb);
}

void output_ascii_cmdack(uint8_t verbose_flg, uint8_t crlf_flg, uint8_t cmd_ack) {

	if (verbose_flg > 0) {
		if (cmd_ack == CMR_SUCCESSFULL) {
			uartRB_Put(&usart3_rb, ">OK;", 4);
		} else if (cmd_ack == CMR_SEMICOLONONLY) {
			uartRB_Put(&usart3_rb, ";", 1);
		} else {
			if (verbose_flg == 1) {
				char tmp[10];
				char tmp2[12];
				sprintf(tmp, "%3.3u", (cmd_ack & 0x7F));

				if (cmd_ack > 128) {
					strcpy(tmp2, ">W");
				} else {
					strcpy(tmp2, ">E");
				}
				strcat(tmp2, tmp);
				strcat(tmp2, ";");
				uartRB_Put(&usart3_rb, tmp2, strlen(tmp2));
			} else if (verbose_flg == 2) {
				char tmp[35];
				char tmp2[40];
				if (cmd_ack > 128) {
					strcpy(tmp2, ">W:");
				} else {
					strcpy(tmp2, ">E:");
				}

				switch (cmd_ack & 0xFF)          //(cmd_ack & 0x7F)
				{
				case CMR_COMMANDONDEMAND:
					strcpy(tmp, "No Answer!");
					break;

				case CMR_PARAMETERINVALID:
					strcpy(tmp, "Parameter Invalid!");
					break;

				case CMR_PARAMETERCLIPEDMIN:
					strcpy(tmp, "Parameter Clipped to Minimum!");
					break;

				case CMR_PARAMETERCLIPEDMAX:
					strcpy(tmp, "Parameter Clipped to Maximum!");
					break;

				case CMR_PARAMETERADJUSTED:
					strcpy(tmp, "Parameter Adjusted!");
					break;

				case CMR_WRONGPARAMETERFORMAT:
					strcpy(tmp, "Wrong Parameter Format!");
					break;

				case CMR_UNKNOWNCOMMAND:
					strcpy(tmp, "Unknown Command!");
					break;

				case CMR_COMMANDDENIED:
					strcpy(tmp, "Command Denied!");
					break;

				case CMR_COMMANDNOTSUPPORTED:
					strcpy(tmp, "Command Not Supported!");
					break;

				case CMR_EEPROMERROR:
					strcpy(tmp, "EEPROM Error!");
					break;

				case CMR_EEPWRLOCKED:
					strcpy(tmp, "EEPROM Write Lock!");
					break;

				case CMR_WRONGOPMODE:
					strcpy(tmp, "Wrong Operation Mode!");
					break;

				case CMR_UNITBUSY:
					strcpy(tmp, "Unit Busy!");
					break;

				case CMR_MISSINGPARAMETER:
					strcpy(tmp, "Missing Parameter!");
					break;

				case CMR_OPTIONNOTINSTALLED:
					strcpy(tmp, "Required Option Not Installed!");
					break;

				case CMR_MALFORMATTEDCOMMAND:
					strcpy(tmp, "Malformatted Command!");
					break;

				default:
					sprintf(tmp, "%3.3u", (cmd_ack & 0x7F));
					break;
				}

				strcat(tmp2, tmp);
				strcat(tmp2, ";");
				uartRB_Put(&usart3_rb, tmp2, strlen(tmp2));
			}
		}
	} else {
		if (cmd_ack != STACK_CMDINSTACK) {
			//if (((cmd_ack & 0x80) == CMR_SUCCESSFULL))
			if ((cmd_ack == CMR_SEMICOLONONLY)
					|| ((cmd_ack & 0x80) == CMR_SUCCESSFULL))
				uartRB_Put(&usart3_rb, ";", 1);

		};
	}

	if ((crlf_flg & 0x01) > 0) {	//crlf_char= "\r";        //d.h. CR
		uartRB_Put(&usart3_rb, "\r", 1);
	}
	if ((crlf_flg & 0x02) > 0) {	//*crlf_char=";
		uartRB_Put(&usart3_rb, "\n", 1);
	}
	uartRB_KickTx(&usart3_rb);
}

//Die interne Befehlnummer werden für den eingegebenen Befehl zurückgeliefert.
void Binary_Search(uint8_t ncmd, char *key, uint16_t *cmdindex) {
	volatile uint16_t low = 0;
	volatile uint16_t high = ncmd - 1;
	volatile uint16_t mid;
	volatile int sflag;
	volatile uint8_t flag = 0;

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

void output_ascii_result(uint8_t verbose_data, uint8_t crlf_data,
		stack_item *result_data) {
	//uart0_puts("output");
	switch (result_data->rwflg) {
	case READ:
		switch (result_data->cmd_index) {
		case CMD_GET_STATUS:
			//output_ascii_Piii_status(result_data->parameter, verbose_data);
			break;
		case CMD_GET_ERR:
			//output_ascii_Piii_error(result_data->parameter, verbose_data);
			break;
		default:
			if ((result_data->cmd_ack == CMR_SUCCESSFULL)
					|| (result_data->cmd_ack == CMR_PARAMETERCLIPEDMIN)
					|| (result_data->cmd_ack == CMR_PARAMETERCLIPEDMAX)
					|| (result_data->cmd_ack == CMR_PARAMETERADJUSTED)) {
				output_ascii(result_data->parameter);
			}
			break;
		}
		;
		output_ascii_cmdack(verbose_data, crlf_data, result_data->cmd_ack);
		break;
	case WRITE:
		output_ascii_cmdack(verbose_data, crlf_data, result_data->cmd_ack);
		break;
	};
}

void output_binary_result(stack_item *cmd) {
	uint8_t buffer[7];
	uint16_t s_tmp = cmd->cmd_sender;
	uint16_t r_tmp = cmd->cmd_receiver;
	int32_t param = cmd->parameter;

	buffer[0] = (s_tmp << 5) | (r_tmp << 3);
	buffer[1] = cmd->cmd_index;
	buffer[2] = cmd->cmd_ack;
	buffer[3] = (param >> 24) & 0xFF;
	buffer[4] = (param >> 16) & 0xFF;
	buffer[5] = (param >> 8) & 0xFF;
	buffer[6] = param & 0xFF;
	serialSendAnswer(buffer);
}

void output_ascii(int32_t val) {
	char tmp[34];
	sprintf(tmp, "%-ld", val);
	uartRB_Put(&usart3_rb, tmp, strlen(tmp));
	uartRB_KickTx(&usart3_rb);
}

uint8_t remote_ascii_verbose(void) {
	return verbose;
}

uint8_t remote_ascii_crlf(void) {
	return crlf;
}

