/*
 * ModemMessageSystem.c
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#include "ModemMessageSystem.h"

// Информация для поключения к серверу wialon
serverData galyleoskyServer = { "nl.gpsgsm.org", // SeverAdress
		22022, // ServerPort
		TCP, // Protocols
		false, // TLS/SSL
		GALILEOSKY, // TransferProtocols
		};

void Modem_SendCommand(char *message){

}

void Test(){

}
