/*
 * ModemMessageSystem.c
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#include "ModemMessageSystem.h"
#include "Other.h"

#include "stdio.h"
#include "string.h"
#include "usart.h"

//Private

SoftTimer timeOutTimer;
Modem_Info modem = { .state = Modem_State_UNDEFINED, };

//Индексы команд в текстовом представлении
const typedef enum modem_CommandEnum {
	modem_command_simpleTest, modem_command_CGNS_PowerOff, modem_command_CGNS_PowerOn, modem_command_EchoMode_Off, modem_command_EchoMode_on,
} serverCommandEnum;

//При выполнении все команды возвращают "OK"
//Текстовое предстовление команд для модема, используеться при отправке
const char *modem_Command[] = { "AT", // (simppleTest) Простая проверка возвращает
		"AT+CGNSPWR=0", // (CGNS_PowerOff) Выключает питание системы CGNS (GPS)
		"AT+CGNSPWR=1", // (CGNS_PowerOn) Включает питание системы CGNS (GPS)
		"AT+CGNSINF", // (CGNS_GetInfo) Запрос данных от модема
		"ATE0", // (EchoMode_Off) Отключает режим эхо
		"ATE1", // (EchoMode_On) Включает режим эхо
		};

void Modem_EchoOff() {
	if (modem.state == Modem_State_READY) {
		Modem_SendCommand((uint16_t)modem_command_EchoMode_Off);
	}

}

void Modem_setup() {
	modem.action = Modem_EchoOff;
}

//Выполнение установленого действия модема
void Modem_RunAction() {
	modem.action();
}

// Инициализация изночальных значений модема
void Modem_InfoInit() {
	modem.timeOutTimer = timeOutTimer;
}

// Информация для поключения к серверу wialon
serverData galyleoskyServer = { "nl.gpsgsm.org", // SeverAdress
		22022, // ServerPort
		TCP, // Protocols
		false, // TLS/SSL
		GALILEOSKY, // TransferProtocols
		};

//Отправляет текстовое сообщение по UART
void MessageSystem_SendMessage(char *text) {
	HAL_UART_Transmit_IT(&huart1, (uint8_t*) text, strlen(text));
	//HAL_UART_Transmit(&huart1, (uint8_t*)text, strlen(text), 5000);
}

//Максимальная длинна буфера сообщений
#define MESSAGE_BUFFER_LENGHT 100
//Буфер чтения сообщений
char messageBuffer[MESSAGE_BUFFER_LENGHT];
//Запускает чтение сообщения
void MessageSystem_StartReadMaxMessage() {
	HAL_UART_Receive_IT(&huart1, (uint8_t*) messageBuffer, MESSAGE_BUFFER_LENGHT);
}

//Останавливает ожидание данных от модема
void MessageSystem_StopReadMessage() {
	HAL_UART_AbortReceive_IT(&huart1);
}

#define MESSAGE_SEND_BUFFER_LENGHT 100
//Данны для отправки
char toSend[MESSAGE_SEND_BUFFER_LENGHT] = { 0 };
//Отправляет команду по UART модему
void Modem_SendCommand(uint16_t command) {
	if (huart1.gState == HAL_UART_STATE_READY) {

		for (uint8_t i = 0; i < MESSAGE_SEND_BUFFER_LENGHT; ++i) {
			toSend[i] = 0;
		}
		snprintf(toSend, MESSAGE_SEND_BUFFER_LENGHT, "%s\n", modem_Command[command]);
		MessageSystem_SendMessage((char*) toSend);
	}
}

// Проверка модема на готовность получать сообщения
//Отправляет команду "AT" ожидает команду "OK"
void Modem_CheckIsReady() {
	if (huart1.RxState == HAL_UART_STATE_BUSY_RX) {
		if (Text_IsFindedInString(messageBuffer, "OK")) {

			modem.state = Modem_State_READY;
			MessageSystem_StopReadMessage();
		} else if (Timer_RunAlways_GetStatus(&modem.timeOutTimer, 5000)) {

			modem.state = Modem_State_BUSY;
			//HAL_UART_Transmit_IT(&huart1,(uint8_t *) "AT\n", 3);
			Timer_ResetTimer(&modem.timeOutTimer);
		}
	} else if (huart1.RxState == HAL_UART_STATE_READY) {

		MessageSystem_StartReadMaxMessage();
	}
}

SoftTimer SendTimer;
void Modem_simpleTest() {
	if (modem.state == Modem_State_UNDEFINED) {

		Modem_CheckIsReady();
		Modem_SendCommand(modem_command_simpleTest);
	}else if (modem.state == Modem_State_BUSY && Timer_RunAlways_GetStatus(&SendTimer, 500)) {

		Modem_SendCommand(modem_command_simpleTest);
	}else if (modem.state == Modem_State_READY) {

		modem.state = Modem_State_BUSY;
	}
}

bool Modem_IsFirstRun = true;
//Выполняеться при начале работы с модемом
void Modem_FirstRun() {
	if (Modem_IsFirstRun) {
		modem.action = Modem_InfoInit;
		Modem_RunAction();
		modem.action = modem_command_simpleTest;
		Modem_IsFirstRun = false;
	}
}

//Public
//Общая функция для проверки
uint8_t testtest = 0;
void Test() {
	Modem_FirstRun();
	Modem_RunAction();
}
