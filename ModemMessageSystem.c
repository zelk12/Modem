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

// Structures
// Содержит текстовую информацию и длину текста
// uint8_t *text - текстовая информация, uint16_t length - длина текстовой информации в байтах
typedef struct textData {
	uint8_t *text;
	uint16_t length;
} textData;

//
#define mS_IB_Size 512
uint8_t modem_Init_Message_InputBuffer[mS_IB_Size] = { 0 };
#define mS_OB_Size 128
uint8_t modem_Init_Message_OutputBuffer[mS_OB_Size] = { 0 };
SoftTimer modem_Init_Message_Timer_Repeat;
SoftTimer modem_Init_Message_Timer_timeOut;
uint16_t modem_Init_Message_timeOut_Delay = 5000;

uint8_t Setup();
uint8_t testFunc();
uint8_t firstFunc();
uint8_t secondFunc();
uint8_t thirdFunc();

uint8_t voidFunc();

// Modem response
textData modem_Response_OK = { (uint8_t*) "OK", 2 };
textData modem_Response_ERROR = { (uint8_t*) "ERROR", 5 };

// Modem command
// SimpleTest
textData modem_Command_SimpleTest = { (uint8_t*) "AT", 2 };

// Echo
// Set
textData modem_Command_EchoModeOn = { (uint8_t*) "ATE1", 4 };
textData modem_Command_EchoModeOff = { (uint8_t*) "ATE0", 4 };

// Local flow control
// Set
textData modem_Command_LocalFlowControl_On = { (uint8_t*) "AT+IFC=1", 8 };
textData modem_Command_LocalFlowControl_Off = { (uint8_t*) "AT+IFC=0", 8 };
// Get
textData modem_Command_LocalFlowControl_Get_SupportedParameters = { (uint8_t*) "AT+IFC=?", 8 };
textData modem_Command_LocalFlowControl_Get_SetedParameters = { (uint8_t*) "AT+IFC?", 7 };

// Line identification presentation
// Set
textData modem_Command_LineIdentificationPresentation_On = { (uint8_t*) "AT+CLIP=1", 1 };
textData modem_Command_LineIdentificationPresentation_Off = { (uint8_t*) "AT+CLIP=0", 1 };
// Get

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

// Func space

// Отправляет подготовленную команду модему в текстовом виде
// input: uint8_t *sendedCommand - команда втекстовом виде, uint8_t size - длина строки
// return: 0 - команда не выполнена, 1 - команда выполнена, 2 - возвращена ошибка, 3 - начато выполнение команды, 4 превышено время ожидания.
uint8_t modem_Do_SendCommand_Confirmed(uint8_t *sendedCommand, uint8_t size) {
	uint8_t info = 0;
	if (huart1.RxState == HAL_UART_STATE_READY) {
		HAL_UART_Receive_IT(&huart1, modem_Init_Message_InputBuffer, mS_IB_Size);
		info = 3;
	} else {
		if (Text_IsFindedInString((char*) modem_Init_Message_InputBuffer, mS_IB_Size, "OK", 2)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			Array_uint8_t_Fill(modem_Init_Message_InputBuffer, mS_IB_Size, 0);

			return 1;
		} else if (Text_IsFindedInString((char*) modem_Init_Message_InputBuffer, mS_IB_Size, "ERROR", 5)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			Array_uint8_t_Fill(modem_Init_Message_InputBuffer, mS_IB_Size, 0);
			return 2;
		}
	}

	if (huart1.gState == HAL_UART_STATE_READY && Timer_RunAlways_GetStatus(&modem_Init_Message_Timer_Repeat, 20)) {
		modem_Work_Status = 2;
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) sendedCommand, size);
		Timer_ResetTimer(&modem_Init_Message_Timer_Repeat);
	}
	if (Timer_RunAlways_GetStatus(&modem_Init_Message_Timer_timeOut, modem_Init_Message_timeOut_Delay)) {
		Timer_ResetTimer(&modem_Init_Message_Timer_timeOut);
		return 4;
	}
	return info;
}

// Отправляет подготовленную команду модему в текстовом виде
// input: textData command необходимая к отправке команда
// return: 0 - команда не выполнена, 1 - команда выполнена, 2 - возвращена ошибка, 3 - начато выполнение команды
uint8_t modem_Do_SendCommand_Confirmed_textData(textData *command) {
	uint8_t CommandText[100];
	sprintf(CommandText, "%s\n", &(*command).text);
	uint8_t size = (*command).length + 1;
	return modem_Do_SendCommand_Confirmed(CommandText, size);
}

// Command
// REFERENCE_COMMAND
// Простой запуск команд модема автоматический отправляет команду модему, ожидает результата, и в случае выполнения меняет переменную на 1
// input: uint8_t *commandStarted - переменная для проверки выполняеться ли данная команда в данный момент времени 1 - идет выполнение 0 - выполнение не запущено,
// textData *command - команда которую необходимо выполнить, uint8_t *modem_Status_REF - значение которое будет изменено в случае выполнения команды или получения ошибки 1 -команда выполнена 0 - выполнения не было\ошибка
uint8_t modem_Do_RunCommand(uint8_t *commandStarted, textData *command, uint8_t *modem_Status_REF) {
	uint8_t confirmed = 0;

	if (*commandStarted == 0 && modem_Work_Status == 0) {
		confirmed = modem_Do_SendCommand_Confirmed_textData(command);
		if (confirmed == 3) {
			*commandStarted = 1;
		}
	} else if (*commandStarted == 1 && modem_Work_Status == 1) {
		confirmed = modem_Do_SendCommand_Confirmed_textData(command);
		if (confirmed == 1) {
			*modem_Status_REF = 1;
			return 1;
		} else if (confirmed == 2) {
			modem_Status_REF = 0;
			return 0;
		}
	}
	return 0;
}

// Запуск простого теста модема, отправляет команду "AT", ожидает ответ "OK"
uint8_t modem_Do_RunCommand_Simple_Test() {
	static uint8_t commandStarted = 0;
	return modem_Do_RunCommand(&commandStarted, &modem_Command_SimpleTest, &modem_Power);
}

// Устанавливает состояние эхо режима
// input: uint8_t newState - новое состояние эхо режима 1 -включен, 0 - выключен
uint8_t modem_Do_RunCommand_Set_EchoMode(uint8_t newState) {
	uint8_t confirmed = 0;
	static uint8_t commandStarted = 0;
	if (commandStarted == 0 && modem_Work_Status == 0) {
		if (newState == 0) {
			confirmed = modem_Do_SendCommand_Confirmed_textData(&modem_Command_EchoModeOff);
		} else if (newState == 1) {
			confirmed = modem_Do_SendCommand_Confirmed_textData(&modem_Command_EchoModeOn);
		}

		if (confirmed == 3) {
			commandStarted = 1;
		}
	} else if (commandStarted == 1 && modem_Work_Status == 1) {
		if (newState == 0) {
			confirmed = modem_Do_SendCommand_Confirmed_textData(&modem_Command_EchoModeOff);

			if (confirmed == 1) {
				echoMode = 1;
				return 1;
			}
		} else if (newState == 1) {
			confirmed = modem_Do_SendCommand_Confirmed_textData(&modem_Command_EchoModeOn);
			if (confirmed == 1) {
				echoMode = 0;
				return 1;
			}
		}
	}

	return 0;
}

// Включает эхо режим
uint8_t modem_Do_RunCommand_EchoMode_On() {
	return modem_Do_RunCommand_Set_EchoMode(1);
}

// Выключает эхо режим
uint8_t modem_Do_RunCommand_EchoMode_Off() {
	return modem_Do_RunCommand_Set_EchoMode(0);
}

uint8_t modem_Do_RunCommand_Set_FlowControl() {
	return 0;
}

// Func
// Repited func
uint8_t ATE0() {
	return modem_Do_SendCommand_Confirmed((uint8_t*) "ATE0\n", 5);
}

uint8_t AT_IFC_1_1() {
	return modem_Do_SendCommand_Confirmed((uint8_t*) "AT+IFC=1,1\n", 11);
}

uint8_t AT_CLIP_1() {
	return modem_Do_SendCommand_Confirmed((uint8_t*) "AT+CLIP=1\n", 10);
}

uint8_t AT_CLTS_1() {
	return modem_Do_SendCommand_Confirmed((uint8_t*) "AT+CLTS=1\n", 10);
}

uint8_t AT_CSCLK_0() {
	return modem_Do_SendCommand_Confirmed((uint8_t*) "AT+CSCLK=0\n", 11);
}

uint8_t AT_CMGHEX_1() {
	return modem_Do_SendCommand_Confirmed((uint8_t*) "AT+CMGHEX=1\n", 12);
}

// Main space
// Инициализация параметров
uint8_t Init() {
	return 0;
}

// Установка параметров работы
uint8_t Setup() {
	modem_Action = &testFunc;

	return 0;
}

uint8_t testFunc() {
	if (modem_Power == 0) {
		modem_Do_RunCommand_Simple_Test();
	}
	return 0;
}

uint8_t firstFunc() {
	uint8_t funcResult = modem_Do_SendCommand_Confirmed((uint8_t*) "AT\n", 3);
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
			if (modem_Do_SendCommand_Confirmed((uint8_t*) "AT+HTTPINIT\n", 12)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 1:
			if (modem_Do_SendCommand_Confirmed((uint8_t*) "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\n", 30)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 2:
			if (modem_Do_SendCommand_Confirmed((uint8_t*) "AT+SAPBR=3,1,\"APN\",\"internet.mts.ru\"\n", 37)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 3:
			if (modem_Do_SendCommand_Confirmed((uint8_t*) "AT+SAPBR=1,1\n", 13)) {
				++thirdFunc_ActionNumber;
			}
			break;
		case 4:
			if (modem_Do_SendCommand_Confirmed((uint8_t*) "AT+HTTPPARA=\"URL\",\"http://urv.iot.turtton.ru/api/log/ep\"\n", 57)) {
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

// Пустотная функция ничего не делает.
uint8_t voidFunc() {
	return 0;
}

void Test() {
	modem_Action();
}
