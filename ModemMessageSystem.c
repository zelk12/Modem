/*
 * ModemMessageSystem.c
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#define MESSAGE_BUFFER_SIZE 20

#include "ModemMessageSystem.h"

SoftTimer ZeroTimer = { 0, 0 };
SoftTimer TestTimer = { 0, 5000 / 2 };
SoftTimer MessageTimer = { 0, 100 };

uint8_t manual = 0;

uint8_t messageBuffer[MESSAGE_BUFFER_SIZE] = { 0 };

bool modemReadyToTX = true;

void readMessage_ByModem(uint8_t size) {
	HAL_UART_Receive_IT(&huart1, messageBuffer, size);
}

void sendMessage_ToModem(uint8_t *pData, uint16_t Size) {
	if (huart1.gState == HAL_UART_STATE_READY) {
		if (SoftTimer_PeriodRun(&MessageTimer)) {
			HAL_UART_Transmit_IT(&huart1, pData, Size);
			modemReadyToTX = false;
		}
	}
}

void sendMessage_ToModemAutoSize(uint8_t *pData) {
	uint8_t size = strlen(pData);

	if (modemReadyToTX) {
		if (huart1.gState == HAL_UART_STATE_READY) {

			sendMessage_ToModem(&pData, size);

			return;
		}
	}
}

bool CommandEnded() {
	for (uint8_t i = 0; i < MESSAGE_BUFFER_SIZE; ++i) {
		if (messageBuffer[i] != '\0') {
			modemReadyToTX = FindStrInArray(&messageBuffer, "OK");
			return modemReadyToTX;
		}
	}
	return false;
}

void Test() {

	if (SoftTimer_PeriodRun(&TestTimer)) {
		CommandEnded();
		readMessage_ByModem(MESSAGE_BUFFER_SIZE);
		sendMessage_ToModemAutoSize("AT\n");
	}
}

uint8_t isTestRuned = 0;
uint8_t isTestCompleted = 0;
uint8_t isTestFailed = 0;

typedef enum {
	SIMPLE_TEST_START = 0x01U, //Тест командой "AT"
	placeholder = 0x02U,    // инициализирован и готов к использованию
} ModemTest_Type;

typedef enum {
	TEST_STATE_RUNED = 0x00U, //Тест запущен
	TEST_STATE_COMPLETED = 0x01U, //Тест успешно завершен
	TEST_STATE_FAILED = 0x02U, //Тест провален
} ModemTest_State;

void modemSimpleTest() {

	//Включить ожидание "AT"
	HAL_UART_Receive_IT(&huart1, messageBuffer, 2);
	//Отправить "AT"
	HAL_UART_Transmit_IT(&huart1, "AT", 2);
	isTestRuned |= 1;

}

void SwitchTestStateTo(uint8_t ModemTest_Type, uint8_t newState) {
	switch (newState) {
		case TEST_STATE_RUNED:
			if (isTestRuned & ModemTest_Type != 1) {
				isTestRuned += ModemTest_Type;
				isTestCompleted -= ModemTest_Type;
				isTestFailed -= ModemTest_Type;
			}
			break;
		case TEST_STATE_COMPLETED:
			if (isTestRuned & ModemTest_Type != 1) {
				isTestRuned -= ModemTest_Type;
				isTestCompleted += ModemTest_Type;
				isTestFailed -= ModemTest_Type;
			}
			break;
		case TEST_STATE_FAILED:
			if (isTestRuned & ModemTest_Type != 1) {
				isTestRuned -= ModemTest_Type;
				isTestCompleted -= ModemTest_Type;
				isTestFailed += ModemTest_Type;
			}
			break;
		default:
			break;
	}
}
