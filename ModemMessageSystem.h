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

#include "SoftTimerSystem.h"

const typedef enum {
	TCP, UDP,
} protocols;

const typedef enum{
	GALILEOSKY,
	EGTS_56360_2015,
	EGTS_33472_2015,
	GALYLEOSKY_COMPRESSED,
} transferProtocols;

typedef struct {
	string mainServerAdres;
	uint32_t port;
	uint8_t protocol;
	bool isUseTLSorSSL;
	uint8_t transferProtocol;
} serverData;

void Test();

#endif /* INC_MODEMMESSAGESYSTEM_H_ */
