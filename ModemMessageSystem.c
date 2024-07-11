/*
 * ModemMessageSystem.c
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#include "ModemMessageSystem.h"
//Private

SoftTimer timeOutTimer;
Modem_Info modem;

// Инициализация изночальных значений модема
void Modem_InfoInit() {
	modem.state = UNDEFINED;
	modem.timeOutTimer = timeOutTimer;
}

bool Modem_IsFirstRuned = false;
void Modem_FirstRun() {
	if (!Modem_IsFirstRuned) {
		modem.action = Modem_InfoInit;
	}
}

//Выполнение установленого действия модема
void Modem_RunAction() {

}

//Индексы команд в текстовом представлении
typedef enum modemCommandEnum {
	simpleTest, _CGNS_PowerOff, _CGNS_PowerOn,
} serverCommandEnum;

//При выполнении все команды возвращают "OK"
//Текстовое предстовление команд для модема, используеться при отправке
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

//Буфер чтения сообщений
char *messageBuffer[512];
//Запускает чтение сообщения
void MessageSystem_ReadMessage() {
	HAL_UART_Receive_IT(&huart1, (uint8_t*) messageBuffer, 512);
}

//Отправляет команду по UART модему
void Modem_SendCommand(uint16_t command) {
	MessageSystem_SendMessage(modemCommand[command]);
}

void Modem_CheckIsReady() {

}

//Public
//Общая функция для проверки
void Test() {

}
