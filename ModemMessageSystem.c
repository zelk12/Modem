/*
 * ModemMessageSystem.c
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#include "ModemMessageSystem.h"

typedef enum modemCommandEnum {
	simpleTest, _CGNS_PowerOff, _CGNS_PowerOn,
} serverCommandEnum;

//При выполнении все команды возвращают "OK"
const char *modemCommand[] = { "AT", // (simppleTest) Простая проверка возвращает
		"AT+CGNSPWR=0", // (_CGNS_PowerOff) Выключает питание системы CGNS (GPS)
		"AT+CGNSPWR=1", // (_CGNS_PowerOn) Включает питание системы CGNS (GPS)
		"AT+CGNSINF", // (_CGNS_GetInfo) Запрос данных от модема
		};

// Информация для поключения к серверу wialon
serverData galyleoskyServer = { "nl.gpsgsm.org", // SeverAdress
		22022, // ServerPort
		TCP, // Protocols
		false, // TLS/SSL
		GALILEOSKY, // TransferProtocols
		};

//Отправляет текстовое сообщение по UART
void MessageSystem_SendMessage(const char *text) {
	HAL_UART_Transmit_IT(&huart1, (uint8_t*) text, strlen(text));
}

char *messageBuffer[512];
void MessageSystem_ReadMessage() {
	HAL_UART_Receive_IT(&huart1, (uint8_t*) messageBuffer, 512);
}

//Отправляет команду по UART модему
void Modem_SendCommand(uint16_t command) {
	MessageSystem_SendMessage(modemCommand[command]);
}

//Общая функция для проверки
void Test() {

}
