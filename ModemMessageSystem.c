/*
 * ModemMessageSystem.c
 *
 *  Created on: 3 июл. 2024 г.
 *      Author: TURTTON
 */

#include "SoftTimerSystem.h"
#include "Other.h"

#include "stdint.h"
#include "usart.h"

//Structures
typedef struct textData {
	uint8_t *text;
	uint16_t length;
} textData;

//
#define mS_IB_Size 512
uint8_t messageSystem_InputBuffer[mS_IB_Size] = { 0 };
#define mS_OB_Size 128
uint8_t messageSystem_OutputBuffer[mS_OB_Size] = { 0 };
SoftTimer messageSystem_Message_Timer;
SoftTimer timeOutTimer;

uint8_t Setup();
uint8_t firstFunc();
uint8_t secondFunc();
uint8_t thirdFunc();

uint8_t voidFunc();

//Modem response
textData modem_Response_OK={"OK",2};
textData modem_Response_ERROR = {"ERROR", 5};
//Modem command

//Echo
//Set
textData modem_Command_EchoModeOn = { "ATE1", 4 };
textData modem_Command_EchoModeOff = { "ATE0", 4 };

//Local flow control
//Set
textData modem_Command_LocalFlowControl_On = {"AT+IFC=1",8};
textData modem_Command_LocalFlowControl_Off = {"AT+IFC=0",8};
//Get
textData modem_Command_LocalFlowControl_Get_SupportedParameters = {"AT+IFC=?",8};
textData modem_Command_LocalFlowControl_Get_SetedParameters = {"AT+IFC?",7};

//Line identification presentation
//Set
textData modem_Command_LineIdentificationPresentation_On = {"AT+CLIP=1",1};
textData modem_Command_LineIdentificationPresentation_Off = {"AT+CLIP=0",1};
//Get

uint8_t modem_Power = 0;
uint8_t (*modem_Action)() = &Setup;
uint8_t modem_Work_Status = 0;
uint8_t echoMode = 0;
uint8_t _HTTP_Service = 0;
uint8_t localDataFlowControl = 0;
uint8_t callingLineIdentificationPresentation = 0;
uint8_t localTimeStamp = 0;
uint8_t slowClock = 0;
uint8_t sendingNonASCII_CharacterSMS = 0;

//Func space

uint8_t modem_DoCommandWithConfirmed(uint8_t *sendedCommand, uint8_t size) {
	if (huart1.RxState == HAL_UART_STATE_READY) {
		HAL_UART_Receive_IT(&huart1, messageSystem_InputBuffer, mS_IB_Size);
	} else {
		if (Text_IsFindedInString((char*) messageSystem_InputBuffer, mS_IB_Size, "OK", 2)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			Array_uint8_t_Fill(messageSystem_InputBuffer, mS_IB_Size, 0);

			return 1;
		} else if (Text_IsFindedInString((char*) messageSystem_InputBuffer, mS_IB_Size, "ERROR", 5)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			Array_uint8_t_Fill(messageSystem_InputBuffer, mS_IB_Size, 0);
			return 2;
		}
	}

	if (huart1.gState == HAL_UART_STATE_READY && Timer_RunAlways_GetStatus(&messageSystem_Message_Timer, 20)) {
		modem_Work_Status = 2;
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) sendedCommand, size);
		Timer_ResetTimer(&messageSystem_Message_Timer);
	}
	return 0;
}

uint8_t modem_SendCommand(textData command){

}

//Func
uint8_t modem_Set_EchoMode(uint8_t state) {
	if (state == 1) {

	} else if (state == 2) {

	}
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
	return modem_DoCommandWithConfirmed((uint8_t*) "AT+CSCLK=0\n", 11);
}

uint8_t AT_CMGHEX_1() {
	return modem_DoCommandWithConfirmed((uint8_t*) "AT+CMGHEX=1\n", 12);
}

//Main space

uint8_t Setup() {
	modem_Action = &firstFunc;

	return 0;
}

uint8_t firstFunc() {
	uint8_t funcResult = modem_DoCommandWithConfirmed((uint8_t*) "AT\n", 3);
	if (funcResult) {
		modem_Action = &secondFunc;
	}
	return funcResult;
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
				modem_Action = &thirdFunc;
				break;
			default:
				break;
		}
	}

	return 0;
}

uint8_t thirdFunc() {
	static uint8_t thirdFunc_ActionNumber = 0;

	switch (thirdFunc_ActionNumber) {
		case 0:
			if (modem_DoCommandWithConfirmed((uint8_t*) "AT+HTTPINIT\n", 12)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 1:
			if (modem_DoCommandWithConfirmed((uint8_t*) "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\n", 30)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 2:
			if (modem_DoCommandWithConfirmed((uint8_t*) "AT+SAPBR=3,1,\"APN\",\"internet.mts.ru\"\n", 37)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 3:
			if (modem_DoCommandWithConfirmed((uint8_t*) "AT+SAPBR=1,1\n", 13)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 4:
			if (modem_DoCommandWithConfirmed((uint8_t*) "AT+HTTPPARA=\"URL\",\"http://urv.iot.turtton.ru/api/log/ep\"\n", 57)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 5:
			modem_Action = &voidFunc;
			break;
		default:
			break;
	}

	return 0;
}

uint8_t voidFunc() {
	return 0;
}

void Test() {
	modem_Action();
}
