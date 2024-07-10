/*
 * ModemMessageSystem.h
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#ifndef INC_MODEMMESSAGESYSTEM_H_
#define INC_MODEMMESSAGESYSTEM_H_

#include "usart.h"

#include "string.h"
#include "stdlib.h"

#include "SoftTimerSystem.h"

//Server
const typedef enum protocols {
	TCP, UDP,
} protocols;

const typedef enum transferProtocols {
	GALILEOSKY, EGTS_56360_2015, EGTS_33472_2015, GALYLEOSKY_COMPRESSED,
} transferProtocols;

typedef struct serverData {
	char *mainServerAdres;
	uint32_t port;
	uint8_t protocol;
	bool isUseTLSorSSL;
	uint8_t transferProtocol;
} serverData;

//Modem
extern const char *modemCommand[];

//uint32_t _ CGNS_Sate[]

void Test();

#endif /* INC_MODEMMESSAGESYSTEM_H_ */
