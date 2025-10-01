#ifndef PROTOCOLL_H
#define PROTOCOLL_H

//--- PUBLIC DEFINES --------------------------------------------------------------------------------------------------------
#define SERIAL_USER_PORT_WAIT_FOR_PAKET_END       0
#define SERIAL_USER_PORT_WAIT_FOR_PAKET_START     1
#define SERIAL_USER_PORT_READ_PAKET               2
#define SERIAL_USER_PORT_PARSE_PAKET              3

#define CMR_MAX_PAKET_LENGTH              25

#define CMR_DATAPAKET_LENGTH              8

#define CMR_DLE     '='
#define CMR_SOT     'N'
#define CMR_EOT     'E'

#define CMR_COMMANDONDEMAND         0x00
#define CMR_SUCCESSFULL				0x80
#define CMR_PARAMETERINVALID		0x01
#define CMR_PARAMETERCLIPEDMIN		0x82
#define CMR_PARAMETERCLIPEDMAX		0x83
#define CMR_PAKETLENGHTINVALID    	0x04
#define CMR_CHECKSUMINVALID			0x05
#define CMR_UNKNOWNCOMMAND			0x06
#define CMR_COMMANDDENIED			0x07
#define CMR_COMMANDNOTSUPPORTED 	0x08
#define CMR_UNITBUSY				0x09
#define CMR_PARAMETERADJUSTED		0x8A
#define CMR_WRONGPARAMETERFORMAT	0x0B
#define CMR_MISSINGPARAMETER		0x0C
#define CMR_EEPROMERROR				0x0D
#define CMR_EEPWRLOCKED				0x0E
#define CMR_WRONGOPMODE				0x0F
#define CMR_OPTIONNOTINSTALLED		0x10
#define CMR_MALFORMATTEDCOMMAND		0x1F
#define CMR_SEMICOLONONLY			0xFF

#define STACK_CMDINSTACK			0x11
#define STACK_UNKNOWNCMDRW			0x12
#define STACK_PRIOLIST_ERROR		0x13
#define STACK_INTSERT_OK			0x14
#define SPEC_MATCHINGCUBE_CMD		0x15

//Symbole fuer den Parameter
#define PARAMETER_START			3
#define PARAMETER_END			6

//die max. Anzahl des Pakets zu senden per SPI
#define  MAX_TOUCH_PACKET	3

//die Laenge des Binary_Paketes
//#define BINARY_PACKET_LENGTH	12
#define BINARY_PACKET_LENGTH	22 // ML changed to 22
#endif
