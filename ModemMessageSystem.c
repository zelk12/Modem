/*
 * ModemMessageSystem.c
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#include "ModemMessageSystem.h"

#include "SoftTimerSystem.h"
#include "Other.h"

#include "stdint.h"
#include "usart.h"

#define mS_iB_Size 512
uint8_t messageSystem_inputBuffer[mS_iB_Size] = { 0 };
#define mS_oB_Size 128
uint8_t messageSystem_outputBuffer[mS_oB_Size] = { 0 };
SoftTimer messageSystem_Message_Timer;
SoftTimer timeOutTimer;

uint8_t Setup();
uint8_t firstFunc();
uint8_t secondFunc();

uint8_t voidFunc();

uint8_t modem_Power = 0;
uint8_t (*modem_Action)() = &Setup;
uint8_t modem_Work_Status = 0;

//Func space

uint8_t modem_DoCommandWithConfirmed(uint8_t *sendedCommand, uint8_t size) {
	if (huart1.RxState == HAL_UART_STATE_READY) {
		HAL_UART_Receive_IT(&huart1, messageSystem_inputBuffer, mS_iB_Size);
	} else {
		if (Text_IsFindedInString((char*) messageSystem_inputBuffer, mS_iB_Size, "OK", 2)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			modem_Action = &secondFunc;
			Array_uint8_t_Fill(messageSystem_inputBuffer, 0);

			return 1;
		}
	}

	if (huart1.gState == HAL_UART_STATE_READY && Timer_RunAlways_GetStatus(&messageSystem_Message_Timer, 2500)) {
		modem_Work_Status = 2;
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) sendedCommand, size);
		Timer_ResetTimer(&messageSystem_Message_Timer);
	}
	return 0;
}

//Repited func
uint8_t ATE0() {
	return modem_DoCommandWithConfirmed((uint8_t*) "ATE0\n", 5);
}

uint8_t AT_IFC_1_1() {
	return modem_DoCommandWithConfirmed((uint8_t*) "AT+IFC=1,1\n", 11);
}

uint8_t AT_CLIP_1() {
	return modem_DoCommandWithConfirmed((uint8_t*) "AT+CLIP=1\n", 10);
}

uint8_t AT_CLTS_1() {
	return modem_DoCommandWithConfirmed((uint8_t*) "AT+CLTS=1\n", 10);
}

uint8_t AT_CSCLK_0() {
	return modem_DoCommandWithConfirmed((uint8_t*) "AT+CLTS=1\n", 10);
}

uint8_t AT_CMGHEX_1() {
	if (huart1.RxState == HAL_UART_STATE_READY) {
		HAL_UART_Receive_IT(&huart1, messageSystem_inputBuffer, mS_iB_Size);
	} else {
		if (Text_IsFindedInString_AutoSize((char*) messageSystem_inputBuffer, "OK")) {
			modem_Power = 1;
			modem_Work_Status = 1;
			modem_Action = &secondFunc;
			Array_uint8_t_Fill(messageSystem_inputBuffer, 0);

			return 1;
		}
	}

	if (huart1.gState == HAL_UART_STATE_READY && Timer_RunAlways_GetStatus(&messageSystem_Message_Timer, 2500)) {
		modem_Work_Status = 2;
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) "AT+CMGHEX=1\n", 12);
		Timer_ResetTimer(&messageSystem_Message_Timer);
	}
	return 0;
}

//Main space

uint8_t Setup() {
	modem_Action = &firstFunc;

	return 0;
}

uint8_t firstFunc() {
	if (huart1.RxState == HAL_UART_STATE_READY) {
		HAL_UART_Receive_IT(&huart1, messageSystem_inputBuffer, mS_iB_Size);
	} else {
		if (Text_IsFindedInString((char*) messageSystem_inputBuffer, mS_iB_Size, "OK", 2)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			modem_Action = &secondFunc;
			HAL_UART_AbortReceive_IT(&huart1);
			Array_uint8_t_Fill(messageSystem_inputBuffer, 0);

			return 1;
		}
	}

	if (huart1.gState == HAL_UART_STATE_READY && Timer_RunAlways_GetStatus(&messageSystem_Message_Timer, 2500)) {
		modem_Work_Status = 2;
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) "AT\n", 3);
		Timer_ResetTimer(&messageSystem_Message_Timer);
	}
	return 0;
}

uint8_t secondFunc() {
	static uint8_t (*secondFunc_Action)() = &ATE0;
	static uint8_t secondFunc_ActionNumber = 0;

	if (secondFunc_Action()) {
		switch (secondFunc_ActionNumber) {
			case 0:
				secondFunc_Action = &AT_IFC_1_1;
				++secondFunc_ActionNumber;
				break;

			case 1:
				secondFunc_Action = &AT_CLIP_1;
				++secondFunc_ActionNumber;
				break;

			case 2:
				secondFunc_Action = &AT_CLTS_1;
				++secondFunc_ActionNumber;
				break;

			case 3:
				secondFunc_Action = &AT_CSCLK_0;
				++secondFunc_ActionNumber;
				break;

			case 4:
				secondFunc_Action = &AT_CMGHEX_1;
				++secondFunc_ActionNumber;
				break;

			case 5:
				modem_Action = &voidFunc;
				break;
			default:
				break;
		}
	}

	return 0;
}

uint8_t voidFunc() {
	return 0;
}

void Test() {
	modem_Action();
}
