/*
 * ModemMessageSystem.h
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#ifndef INC_MODEMMESSAGESYSTEM_H_
#define INC_MODEMMESSAGESYSTEM_H_



//#include "stdlib.h"

//#include "fsm_gc.h"

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

//Статус GPS
typedef struct CGNS_State {
	uint8_t _GNSS_RunStatus;
	uint8_t fixSatus;
	long double _UTC_DateAndTime;
	float latitude;
	float longitude;
	float _MSL_Altitude;
	float speed_OverGround;
	float course_OverGround;
	uint8_t fixMode;
	uint8_t reserved1;
	uint8_t _HDOP;
	uint8_t _PDOP;
	uint8_t _VDOP;
	uint8_t reserved2;
	uint8_t satellitesInView;
	uint8_t satellitesUsed;
	uint8_t reserved3;
	uint8_t _C_OrN0_max;
	float _HPA;
	float _VPA;
} CGNS_State;

//Существующие состояния модема
typedef enum Modem_State{
	 Modem_State_UNDEFINED, // Состояние не определено
	 Modem_State_READY, // Готов для получения новых команд
	 Modem_State_BUSY, // Занят
} Modem_State;

//Информация о модеме
typedef struct Modem_Info {
	uint8_t state;
	void (*action)();
	SoftTimer timeOutTimer;
	CGNS_State _GPS_State;
} Modem_Info;

void Test();

#endif /* INC_MODEMMESSAGESYSTEM_H_ */
