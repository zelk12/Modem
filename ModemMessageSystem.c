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

#include "stdio.h"

//================================================================================================================================================
//================================================================================================================================================

#pragma region Structures
// Содержит текстовую информацию и длину текста
// uint8_t *text - текстовая информация, uint16_t length - длина текстовой информации в байтах
typedef struct textData {
	uint8_t *text;
	uint16_t length;
} textData;

//================================================================================================================================================

#define modem_actionUsedType_first int8_t
#define modem_statusData_first int8_t

//
#define mS_IB_Size 512
uint8_t modem_Message_InputBuffer[mS_IB_Size] = { 0 };
#define mS_OB_Size 128
uint8_t modem_Message_OutputBuffer[mS_OB_Size] = { 0 };
textData modem_SendedCommand_OutputBuffer = { modem_Message_OutputBuffer, 0 };
SoftTimer modem_Init_Message_Timer_Repeat;
SoftTimer modem_Init_Message_Timer_timeOut;
uint16_t modem_Init_Message_timeOut_Delay = 5000;

modem_actionUsedType_first Setup();
modem_actionUsedType_first testFunc();
modem_actionUsedType_first firstFunc();
modem_actionUsedType_first secondFunc();
modem_actionUsedType_first firstFunc_Old();
modem_actionUsedType_first secondFunc_Old();
modem_actionUsedType_first thirdFunc();

modem_actionUsedType_first voidFunc();

//================================================================================================================================================
//================================================================================================================================================

enum SystemInfo_Message {
	SystemInfo_Message_Unknow = -1, SystemInfo_Message_False, SystemInfo_Message_True, SystemInfo_Message_Ok, SystemInfo_Message_Busy, SystemInfo_Message_Error, SystemInfo_Message_TimeOut, SystemInfo_Message_Started
};

//================================================================================================================================================
//================================================================================================================================================
#pragma region Command List
// Modem response
const textData modem_Response_OK = { (uint8_t*) "OK", 2 };
const textData modem_Response_ERROR = { (uint8_t*) "ERROR", 5 };

// Modem command
// SimpleTest
const textData modem_Command_SimpleTest = { (uint8_t*) "AT", 2 };

// Echo
// Set
const textData modem_Command_EchoModeOn = { (uint8_t*) "ATE1", 4 };
const textData modem_Command_EchoModeOff = { (uint8_t*) "ATE0", 4 };

// Local flow control
const textData modem_Command_LocalFlowControl = { (uint8_t*) "AT+IFC=", 7 };

// Set

//const textData modem_Command_LocalFlowControl_Enable = { (uint8_t*) "AT+IFC=1", 8 };
//const textData modem_Command_LocalFlowControl_Disable = { (uint8_t*) "AT+IFC=0", 8 };

// Get
const textData modem_Command_LocalFlowControl_Get_AvailableParameters = { (uint8_t*) "AT+IFC=?", 8 };
const textData modem_Command_LocalFlowControl_Get_SetedParameters = { (uint8_t*) "AT+IFC?", 7 };

// Line identification presentation
const textData modem_Command_LineIdentificationPresentation = { (uint8_t*) "AT+CLIP=", 8 };

// Set
const textData modem_Command_LineIdentificationPresentation_Enable = { (uint8_t*) "AT+CLIP=1", 9 };
const textData modem_Command_LineIdentificationPresentation_Disable = { (uint8_t*) "AT+CLIP=0", 9 };

// Get

// Local TimeStamp
textData modem_Command_TimeStamp_TimeRefresh_Network = { (uint8_t*) "AT+CLTS", 7 };
textData modem_Command_TimeStamp_TimeRefresh_Network_Enable = { (uint8_t*) "AT+CLTS=1", 9 };

// Slow Clock
textData modem_Command_SlowClock = { (uint8_t*) "AT+CSCLK=", 9 };
textData modem_Command_SlowClock_Disable = { (uint8_t*) "AT+CSCLK=0", 10 };

// Enable or Disable Sending Non-ASCII Character SMS
textData modem_Command_EoDSendingNonASCII_SMS = { (uint8_t*) "AT+CMGHEX=", 10 };
textData modem_Command_EoDSendingNonASCII_SMS_Enable = { (uint8_t*) "AT+CMGHEX=1", 11 };

// Initialize HTTP Service
textData modem_Command_HTTP_Initialize_Service = { (uint8_t*) "AT+HTTPINIT", 11 };

// Bearer Settings for Applications Based on IP
textData modem_Command_SAPBR = { (uint8_t*) "AT+SAPBR=", 9 };

// Set HTTP Parameters Value
textData modem_Command_HTTPPARA = { (uint8_t*) "AT+HTTPPARA=", 12 };

//================================================================================================================================================
//================================================================================================================================================

modem_statusData_first modem_Power = -1;
modem_actionUsedType_first (*modem_Action)() = &Setup;
uint8_t modem_Work_Status = 0;
modem_statusData_first echoMode = -1;
uint8_t _HTTP_Service = 0;
modem_statusData_first localDataFlowControl = -1;
modem_statusData_first callingLineIdentificationPresentation = -1;
modem_statusData_first localTimeStamp = -1;
modem_statusData_first slowClock = -1;
modem_statusData_first sendingNonASCII_CharacterSMS = -1;

//================================================================================================================================================
//================================================================================================================================================

#pragma region Func space

#pragma region SendCommand
// Отправляет подготовленную команду модему в текстовом виде
// input: uint8_t *sendedCommand - команда втекстовом виде, uint8_t size - длина строки
// return: 0 - команда не выполнена, 1 - команда выполнена, 2 - возвращена ошибка, 3 - начато выполнение команды, 4 превышено время ожидания.
int8_t modem_Do_SendCommand_Confirmed(uint8_t *sendedCommand, uint8_t size) {
	int8_t info = SystemInfo_Message_Unknow;
	if (huart1.RxState == HAL_UART_STATE_READY) {
		HAL_UART_Receive_IT(&huart1, modem_Message_InputBuffer, mS_IB_Size);
		modem_Work_Status = 2;
		info = SystemInfo_Message_Started;
	} else {
		if (Text_IsFindedIn((char*) modem_Message_InputBuffer, mS_IB_Size, "OK", 2)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			Array_uint8_t_Fill(modem_Message_InputBuffer, mS_IB_Size, 0);

			return SystemInfo_Message_Ok;
		} else if (Text_IsFindedIn((char*) modem_Message_InputBuffer, mS_IB_Size, "ERROR", 5)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			Array_uint8_t_Fill(modem_Message_InputBuffer, mS_IB_Size, 0);
			return SystemInfo_Message_Error;
		}
	}

	if (huart1.gState == HAL_UART_STATE_READY && Timer_RunAlways_GetStatus(&modem_Init_Message_Timer_Repeat, 20)) {
		modem_Work_Status = 3;
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) sendedCommand, size);
		Timer_ResetTimer(&modem_Init_Message_Timer_Repeat);
	}
	if (Timer_RunAlways_GetStatus(&modem_Init_Message_Timer_timeOut, modem_Init_Message_timeOut_Delay)) {
		Timer_ResetTimer(&modem_Init_Message_Timer_timeOut);
		return SystemInfo_Message_TimeOut;
	}
	return info;
}

uint8_t modem_Do_SendCommand_Confirmed_GetData_Bool(uint8_t *sendedCommand, uint8_t size, uint8_t *commandBeforData, uint8_t commandBeforDataSize) {
	uint8_t info = SystemInfo_Message_Unknow;
	if (huart1.RxState == HAL_UART_STATE_READY) {
		HAL_UART_Receive_IT(&huart1, modem_Message_InputBuffer, mS_IB_Size);
		modem_Work_Status = 2;
		info = SystemInfo_Message_Started;
	} else {
		if (Text_IsFindedIn((char*) modem_Message_InputBuffer, mS_IB_Size, "OK", 2)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			Array_uint8_t_Fill(modem_Message_InputBuffer, mS_IB_Size, 0);

			uint8_t data = 2;
			uint8_t position = Text_IsFindedIn_FirstPosition((char*) modem_Message_InputBuffer, mS_IB_Size, (char*) commandBeforData, commandBeforDataSize);
			if (modem_Message_InputBuffer[position + 1] == commandBeforData[1]) {
				data = modem_Message_InputBuffer[position + 5];
			}

			return data;
		} else if (Text_IsFindedIn((char*) modem_Message_InputBuffer, mS_IB_Size, "ERROR", 5)) {
			modem_Power = 1;
			modem_Work_Status = 1;
			Array_uint8_t_Fill(modem_Message_InputBuffer, mS_IB_Size, 0);
			return 2;
		}
	}

	if (huart1.gState == HAL_UART_STATE_READY && Timer_RunAlways_GetStatus(&modem_Init_Message_Timer_Repeat, 20)) {
		modem_Work_Status = 3;
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) sendedCommand, size);
		Timer_ResetTimer(&modem_Init_Message_Timer_Repeat);
	}
	if (Timer_RunAlways_GetStatus(&modem_Init_Message_Timer_timeOut, modem_Init_Message_timeOut_Delay)) {
		Timer_ResetTimer(&modem_Init_Message_Timer_timeOut);
		return 2;
	}
	return info;
}

// Отправляет подготовленную команду модему в текстовом виде
// input: textData command необходимая к отправке команда
// return: 0 - команда не выполнена, 1 - команда выполнена, 2 - возвращена ошибка, 3 - начато выполнение команды
uint8_t modem_Do_SendCommand_Confirmed_textData(const textData *command) {
	uint8_t CommandText[100];
	sprintf((char*) CommandText, "%s\n", (char*) &(*command).text);
	uint8_t size = (*command).length + 1;
	return modem_Do_SendCommand_Confirmed(CommandText, size);
}

uint8_t test_modem_Do_SendCommand_Confirmed_GetData_Bool_textData(textData *sendedCommand, textData *responsStartCommand) {
	uint8_t SendCommand_info = modem_Do_SendCommand_Confirmed_textData(sendedCommand);
	if (SendCommand_info == SystemInfo_Message_True) {
		int8_t responsCommand_firstPosition = -1;
		responsCommand_firstPosition = Text_IsFindedIn_FirstPosition((char*) modem_Message_InputBuffer, mS_IB_Size, (char*) (*responsStartCommand).text, (*responsStartCommand).length);

	}
}

#pragma region RunCommand
// Command
// REFERENCE_COMMAND
// Простой запуск команд модема автоматический отправляет команду модему, ожидает результата, и в случае выполнения меняет переменную на 1
// input: uint8_t *commandStarted - переменная для проверки выполняеться ли данная команда в данный момент времени 1 - идет выполнение 0 - выполнение не запущено,
// textData *command - команда которую необходимо выполнить, uint8_t *modem_Status_REF - значение которое будет изменено в случае выполнения команды или получения ошибки 1 -команда выполнена 0 - выполнения не было\ошибка
uint8_t modem_Do_RunCommand(uint8_t *commandStarted, const textData *command, uint8_t *modem_Status_REF) {
	uint8_t confirmed = 0;

	if (*commandStarted == 0 && modem_Work_Status == 1) {
		confirmed = modem_Do_SendCommand_Confirmed_textData(command);
		if (confirmed == 3) {
			*commandStarted = 1;
		}
	} else if (*commandStarted == 1 && modem_Work_Status == 2) {
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

uint8_t modem_Do_RunCommand_Set_Bool(const uint8_t *newState, uint8_t *commandStarted, const textData *commandOff, const textData *commandOn, uint8_t *modem_Status_REF) {
	uint8_t confirmed = 0;
	if (*commandStarted == 0 && modem_Work_Status == 1) {
		if (*newState == 0) {
			confirmed = modem_Do_SendCommand_Confirmed_textData(commandOff);
		} else if (*newState == 1) {
			confirmed = modem_Do_SendCommand_Confirmed_textData(commandOn);
		}

		if (confirmed == 3) {
			*commandStarted = 1;
		}
	} else if (*commandStarted == 1 && modem_Work_Status == 2) {
		if (*newState == 0) {
			confirmed = modem_Do_SendCommand_Confirmed_textData(commandOff);

			if (confirmed == 1) {
				*modem_Status_REF = 1;
				return 1;
			}
		} else if (*newState == 1) {
			confirmed = modem_Do_SendCommand_Confirmed_textData(commandOn);
			if (confirmed == 1) {
				*modem_Status_REF = 0;
				return 1;
			}
		}
	}

	return 0;
}

// ATE
// Устанавливает состояние эхо режима
// input: uint8_t newState - новое состояние эхо режима 1 -включен, 0 - выключен
uint8_t modem_Do_RunCommand_Set_EchoMode(const uint8_t newState) {
	static uint8_t commandStarted = 0;
	return modem_Do_RunCommand_Set_Bool(&newState, &commandStarted, &modem_Command_EchoModeOff, &modem_Command_EchoModeOn, &echoMode);
}

// Включает эхо режим
uint8_t modem_Do_RunCommand_EchoMode_On() {
	return modem_Do_RunCommand_Set_EchoMode(1);
}

// Выключает эхо режим
uint8_t modem_Do_RunCommand_EchoMode_Off() {
	return modem_Do_RunCommand_Set_EchoMode(0);
}

//LFC
uint8_t modem_Do_RunCommand_FlowControl(const uint8_t TE, const uint8_t TA) {
	sprintf((char*) &modem_SendedCommand_OutputBuffer.text, "%s%s:%s", (char*) &modem_Command_LocalFlowControl.text, (char*) &TE, (char*) &TA);
	modem_SendedCommand_OutputBuffer.length = modem_Command_LocalFlowControl.length + 3;
	modem_Do_SendCommand_Confirmed_textData(&modem_SendedCommand_OutputBuffer);
	return 0;
}

/*uint8_t modem_Do_RunCommand_Set_FlowControl(const uint8_t TE, const uint8_t TA) {
 static uint8_t commandStarted = 0;
 return 0;
 }

 uint8_t modem_Do_RunCommand_FlowContol_On() {
 return modem_Do_RunCommand_Set_FlowControl(1);
 }

 uint8_t modem_Do_RunCommand_FlowContol_Off() {
 return modem_Do_RunCommand_Set_FlowControl(0);
 }*/

//uint8_t modem_Do_RunCommand_Get_FlowControl() {
//	static uint8_t temp_localDataFlowControl = 0;
//
//	temp_localDataFlowControl = modem_Do_SendCommand_Confirmed_GetData_Bool(modem_Command_LocalFlowControl_Get_SetedParameters.text, modem_Command_LocalFlowControl_Get_SetedParameters.length, (uint8_t*) "+IFC", 4);
//	if (temp_localDataFlowControl == 0) {
//		localDataFlowControl = 0;
//		return 0;
//	} else if (temp_localDataFlowControl == 1) {
//		localDataFlowControl = 1;
//		return 1;
//	}
//	return temp_localDataFlowControl;
//}
//uint8_t modem_Do_RunCommand_Get_FlowConrol_Available() {
//	return modem_Do_SendCommand_Confirmed(modem_Command_LocalFlowControl_Get_AvailableParameters.text, modem_Command_LocalFlowControl_Get_AvailableParameters.length);
//}
//CLIP
uint8_t modem_Do_RunCommand_CLIP(uint8_t newState) {
	sprintf((char*) modem_SendedCommand_OutputBuffer.text, "%s%d", (char*) modem_Command_LineIdentificationPresentation.text, newState);
	modem_SendedCommand_OutputBuffer.length = modem_Command_LineIdentificationPresentation.length + 1;
	return modem_Do_SendCommand_Confirmed_textData(&modem_SendedCommand_OutputBuffer);
}

//CLTS
uint8_t modem_Do_RunCommand_CLTS(uint8_t newState) {
	sprintf((char*) modem_SendedCommand_OutputBuffer.text, "%s%d", (char*) modem_Command_TimeStamp_TimeRefresh_Network.text, newState);
	modem_SendedCommand_OutputBuffer.length = modem_Command_TimeStamp_TimeRefresh_Network.length + 1;
	return modem_Do_SendCommand_Confirmed_textData(&modem_SendedCommand_OutputBuffer);
}

//CSCLK
uint8_t modem_Do_RunCommand_CSCLK(uint8_t newState) {
	sprintf((char*) modem_SendedCommand_OutputBuffer.text, "%s%d", (char*) modem_Command_SlowClock.text, newState);
	modem_SendedCommand_OutputBuffer.length = modem_Command_SlowClock.length + 1;
	return modem_Do_SendCommand_Confirmed_textData(&modem_SendedCommand_OutputBuffer);
}

//CMGHEX
uint8_t modem_Do_RunCommand_CMGHEX(uint8_t newState) {
	sprintf((char*) modem_SendedCommand_OutputBuffer.text, "%s%d", (char*) modem_Command_EoDSendingNonASCII_SMS.text, newState);
	modem_SendedCommand_OutputBuffer.length = modem_Command_EoDSendingNonASCII_SMS.length + 1;
	return modem_Do_SendCommand_Confirmed_textData(&modem_SendedCommand_OutputBuffer);
}

//HTTPINIT
uint8_t modem_Do_RunCommand_HTTPINIT() {
	return modem_Do_SendCommand_Confirmed_textData(&modem_Command_HTTP_Initialize_Service);
}

//SAPBR
uint8_t modem_Do_RunCommand_SAPBR(uint8_t newState) {
	sprintf((char*) &modem_SendedCommand_OutputBuffer.text, "%s%d", (char*) &modem_Command_SAPBR.text, newState);
	modem_SendedCommand_OutputBuffer.length = modem_Command_SAPBR.length + 1;
	return modem_Do_SendCommand_Confirmed_textData(&modem_SendedCommand_OutputBuffer);
}

//HTTPPARA
uint8_t modem_Do_RunCommand_HTTPPARA(uint8_t *text, uint16_t *textLength) {
	sprintf((char*) &modem_SendedCommand_OutputBuffer.text, "%s%s", (char*) &modem_Command_HTTPPARA.text, (char*) text);
	modem_SendedCommand_OutputBuffer.length = modem_Command_HTTPPARA.length + *textLength;
	return modem_Do_SendCommand_Confirmed_textData(&modem_SendedCommand_OutputBuffer);
}

uint8_t modem_Do_RunCommand_HTTPPARA_textData(textData *command) {
	return modem_Do_RunCommand_HTTPPARA((*command).text, &(*command).length);
}

//================================================================================================================================================

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

//================================================================================================================================================

#pragma region Main space
// Инициализация параметров
modem_actionUsedType_first Init() {
	return 0;
}

// Установка параметров работы
modem_actionUsedType_first Setup() {
	modem_Action = firstFunc;

	return 0;
}

modem_actionUsedType_first testFunc() {
	if (modem_Power == 0) {
		modem_Do_RunCommand_Simple_Test();
	}
	return 0;
}

modem_actionUsedType_first firstFunc() {
	if (modem_Do_RunCommand_Simple_Test()) {
		modem_Action = secondFunc;
	}
	return 0;
}

modem_actionUsedType_first secondFunc() {
	static uint8_t (*secondFunc_action)() = modem_Do_RunCommand_EchoMode_Off;
	static uint8_t secondFunc_Number = 0;

	switch (secondFunc_Number) {
		case 0:
			if (secondFunc_action()) {
				secondFunc_action = modem_Do_RunCommand_FlowControl;
				secondFunc_Number++;
			}
			break;
		case 1:
			if (secondFunc_action(1)) {
				secondFunc_action = modem_Do_RunCommand_CLIP;
				secondFunc_Number++;
			}
			break;
		case 2:
			if (secondFunc_action(1)) {
				secondFunc_action = modem_Do_RunCommand_CLTS;
				secondFunc_Number++;
			}
			break;
		default:
			break;
	}
}

modem_actionUsedType_first firstFunc_Old() {
	uint8_t funcResult = modem_Do_SendCommand_Confirmed((uint8_t*) "AT\n", 3);
	if (funcResult) {
		modem_Action = &secondFunc_Old;
	}
	return funcResult;
}

modem_actionUsedType_first secondFunc_Old() {
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

modem_actionUsedType_first thirdFunc() {
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
modem_actionUsedType_first voidFunc() {
	return 0;
}

void Test() {
	modem_Action();
}

//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================
#pragma region Tested Place DONT'USE
//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================

//Подключиться к модему
//Проверить работу модема
//Отправить проверочный код "AT"

//Провести настройку "Setup"
//Подготовиться к подключению к серверу

//Подключиться к серверу
//Начать отправку данных

//===

//Отправка команд; Для отправки данных необходимо знать: адрес начала текста, длину текста в байтах

//Структура содержащая текст и его длину

//===

//После отправки команды необходимо проверить и вернуть результат работы возможные варианты "OK", "ERROR" в случае если данные не вернуться по истечению времени вернуть "TimeOut"

//Проверка состояния: при проверке состояния модем возвращаяет текст команды и ответ, ответ и нужно будет получить
//В ответе можеты быть одно число, множество чисел и текст

//Продумывание функции, возваращает тип данных и данные
//Для числовых данных возвращает массив и его длину

//===

#pragma region testplace func floder?
//Сокращения доступа
#define testPlace
#define testPlace_modem
#define testPlace_modem_command
#define testPlace_modem_command_textdata
#define testplace_modem_command_textdata_response
#define testPlace_modem_message
#define testPlace_modem_message_input
#define testPlace_modem_message_buffer
#define testPlace_modem_message_action
#define testPlace_modem_message_action_simple
#define testPlace_modem_message_action_complex

#pragma region testPlace Struct
//Структура текста
typedef struct testPlace_modem_textData {
	uint8_t *text;
	size_t length;
} testPlace_modem_textData;

#pragma region testplace modem message buffer
//Буферы
//Буфер приема
#define testplace_modem_message_buffer_input_length 512
uint8_t testplace_modem_message_buffer_input[testplace_modem_message_buffer_input_length] = { 0 };
testPlace_modem_textData testplace_modem_message_buffer_input_textdata = { testplace_modem_message_buffer_input, testplace_modem_message_buffer_input_length };

//Буфер отправки
#define testplace_modem_message_buffer_output_length 512
uint8_t testplace_modem_message_buffer_output[testplace_modem_message_buffer_output_length] = { 0 };
testPlace_modem_textData testplace_modem_message_buffer_output_textdata = { testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length };

#pragma region testplace Modem Commands
testPlace_modem_textData testplace_modem_command_textdata_placeholder = { };

//Список команд доступных для отправки
testPlace_modem_textData testPlace_modem_command_textdata_AT = { (uint8_t*) "AT", 2 };
testPlace_modem_textData testPlace_modem_command_textdata_ATE = { (uint8_t*) "ATE", 3 };

testPlace_modem_textData testPlace_modem_command_textdata_AT_IFC = { (uint8_t*) "AT+IFC", 6 };
testPlace_modem_textData testPlace_modem_command_textdata_AT_CLIP = { (uint8_t*) "AT+CLIP", 7 };
testPlace_modem_textData testPlace_modem_command_textdata_AT_CLTS = { (uint8_t*) "AT+CLTS", 7 };
testPlace_modem_textData testPlace_modem_command_textdata_AT_CSCLK = { (uint8_t*) "AT+CSCLK", 8 };
testPlace_modem_textData testPlace_modem_command_textdata_AT_CMGHEX = { (uint8_t*) "AT+CMGHEX", 9 };

testPlace_modem_textData testPlace_modem_command_textdata_AT_HTTPINIT = { (uint8_t*) "AT+HTTPINIT", 11 };
testPlace_modem_textData testPlace_modem_command_textdata_AT_SAPBR = { (uint8_t*) "AT+SAPBR", 8 };
testPlace_modem_textData testPlace_modem_command_textdata_AT_HTTPPARA = { (uint8_t*) "AT+HTTPPARA", 11 };
testPlace_modem_textData testPlace_modem_command_textdata_AT_HTTPACTION = { (uint8_t*) "AT+HTTPACTION", 13 };
testPlace_modem_textData testPlace_modem_command_textdata_AT_HTTPDATA = { (uint8_t*) "AT+HTTPDATA", 11 };

#pragma region testplace Modem Response Commands
//Список ответов при выполнении команд
testPlace_modem_textData testplace_modem_command_textdata_response_OK = { (uint8_t*) "OK", 2 };
testPlace_modem_textData testplace_modem_command_textdata_response_ERROR = { (uint8_t*) "ERROR", 5 };

testPlace_modem_textData testplace_modem_command_textdata_response_DOWNLOAD = { (uint8_t*) "DOWNLOAD", 8 };

#pragma region testplace modem command result
enum testplace_modem_command_result {
	testplace_modem_command_result_Unknow, testplace_modem_command_result_TimeOut, testplace_modem_command_result_Error, testplace_modem_command_result_Ok, testplace_modem_command_result_work_processing, testplace_modem_command_result_wait_data,
} testplace_modem_command_result;

#pragma region testplace Modem State
enum testplace_modem_state_value {
	testplace_modem_state_value_unknow = -1, testplace_modem_state_value_off, testplace_modem_state_value_on,
} testplace_modem_state_value;

int8_t testplace_modem_state_power = testplace_modem_state_value_unknow;
int8_t testplace_modem_state_busy = testplace_modem_state_value_unknow;

int8_t testplace_modem_state_ATE = testplace_modem_state_value_unknow;

int8_t testplace_modem_state_AT_IFC[2] = { testplace_modem_state_value_unknow, testplace_modem_state_value_unknow };
int8_t testplace_modem_state_AT_CLIP = testplace_modem_state_value_unknow;
int8_t testplace_modem_state_AT_CLTS = testplace_modem_state_value_unknow;
int8_t testplace_modem_state_AT_CSCLK = testplace_modem_state_value_unknow;
int8_t testplace_modem_state_AT_CMGHEX = testplace_modem_state_value_unknow;

int8_t testplace_modem_state_AT_HTTPINIT = testplace_modem_state_value_unknow;

#pragma region testplace Modem Func Command
//Функция отправки команд
//На вход получает комманду которую необходимо отправить
//Ничего не возвращает
void testplace_modem_message_action_simple_sendcommand(testPlace_modem_textData *command) {
	sprintf((char*) testplace_modem_message_buffer_output, "%s%c", (char*) (*command).text, '\n');
	HAL_UART_Transmit_IT(&huart1, testplace_modem_message_buffer_output, (*command).length + 1);
}

//Функция запуска ожидания текста на входе
//Запускает ожидание HAL_UART
void testplace_modem_message_action_simple_waittext() {
	HAL_UART_Receive_IT(&huart1, testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length);
}
testPlace_modem_message_buffer

//Проверяет начатоли ожидание текста, в ином случае запускает ожидание
//Если ожидание начато, запускает таймер и проверяет текст на содержание сообщения "OK" или "ERROR"
//Если в тексте содержатся искомые сообщения, останавливает ожидание, останавливает и обнуляет таймер, заполняет буфер нулями, возвращает соответсвующий ответ
//Если текст не содержиться, проверяет таймер
//Если время вышло делает действие аналогичное нахождению сообщения

static SoftTimer testPlace_modem_message_input_timeOutTimer;
uint32_t testPlace_modem_message_input_timeOutTimer_delay = 10000;
uint8_t testPlace_modem_message_action_complex_find_command_result() {
	//func
	uint8_t return_result_end(uint8_t result) {
		Timer_ResetTimer(&testPlace_modem_message_input_timeOutTimer);
		Array_uint8_t_Fill(testplace_modem_message_buffer_input, testplace_modem_message_buffer_input_length, '\0');
		HAL_UART_AbortReceive_IT(&huart1);
		return result;
	}

	//body
	if (huart1.RxState == HAL_UART_STATE_BUSY_RX) {
		if (!Timer_RunAlways_GetStatus(&testPlace_modem_message_input_timeOutTimer, testPlace_modem_message_input_timeOutTimer_delay)) {
			if (Text_IsFindedIn((char*) testplace_modem_message_buffer_input, testplace_modem_message_buffer_input_length, (char*) testplace_modem_command_textdata_response_OK.text, testplace_modem_command_textdata_response_OK.length)) {
				return return_result_end(testplace_modem_command_result_Ok);
			} else if ((Text_IsFindedIn((char*) testplace_modem_message_buffer_input, testplace_modem_message_buffer_input_length, (char*) testplace_modem_command_textdata_response_ERROR.text, testplace_modem_command_textdata_response_ERROR.length))) {
				return return_result_end(testplace_modem_command_result_Error);
			} else if (Text_IsFindedIn((char*) testplace_modem_message_buffer_input, testplace_modem_message_buffer_input_length, (char*) testplace_modem_command_textdata_response_DOWNLOAD.text, testplace_modem_command_textdata_response_DOWNLOAD.length)) {
				return return_result_end(testplace_modem_command_result_wait_data);
			}
			return testplace_modem_command_result_work_processing;
		} else {
			return return_result_end(testplace_modem_command_result_TimeOut);
		}
	} else {
		testplace_modem_message_action_simple_waittext();
		return testplace_modem_command_result_work_processing;
	}
	return return_result_end(testplace_modem_command_result_Unknow);
}

//Функция отправляет команду модему
//Проверяет доступность порта, если порт не занят отправляет команду
void testplace_modem_message_action_simple_rightsendcommand(testPlace_modem_textData *command) {
	if (huart1.RxState == HAL_UART_STATE_READY && huart1.gState == HAL_UART_STATE_READY) {
		testplace_modem_message_action_simple_sendcommand(command);
	}
}

uint8_t testplace_moedm_message_action_complex_sendcommand_confirmed(testPlace_modem_textData *command) {
	testplace_modem_message_action_simple_rightsendcommand(command);
	return testPlace_modem_message_action_complex_find_command_result();
}

uint8_t testPlace_modem_message_action_simple_sendcommand_AT() {
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testPlace_modem_command_textdata_AT);
}

uint8_t testPlace_modem_message_action_simple_runcommand_simpletest() {
#define testplace_modem_message_action_simple_usedaction testPlace_modem_message_action_simple_sendcommand_AT
#define testplace_modem_state_usedstate testplace_modem_state_power
	static uint8_t _is_worked = testplace_modem_state_value_off;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if ((testplace_modem_state_busy == testplace_modem_state_value_off || testplace_modem_state_busy == testplace_modem_state_value_unknow) && _is_worked == testplace_modem_state_value_off) {
		result = testplace_modem_message_action_simple_usedaction();
		_is_worked = testplace_modem_state_value_on;
		testplace_modem_state_busy = testplace_modem_state_value_on;
	} else if (testplace_modem_state_busy == testplace_modem_state_value_on && _is_worked == testplace_modem_state_value_on) {
		result = testplace_modem_message_action_simple_usedaction();
	}

	if (result == testplace_modem_command_result_Ok || result == testplace_modem_command_result_Error) {
		testplace_modem_state_usedstate = testplace_modem_state_value_on;
		work_stop();

	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_ATEn(uint8_t newstate) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_ATE
	sprintf((char*) testplace_modem_message_buffer_output, "%s%d", (char*) testplace_modem_command_textdata_usedcommand.text, newstate);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 1;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_ATE_set(uint8_t newstate) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_ATEn
#define testplace_modem_state_usedstate testplace_modem_state_ATE
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
	}

	if (result == testplace_modem_command_result_Ok) {
		testplace_modem_state_usedstate = newstate;
		work_stop();

	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_AT_IFCnn(uint8_t TE, uint8_t TA) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_IFC
	sprintf((char*) testplace_modem_message_buffer_output, "%s=%d,%d", (char*) testplace_modem_command_textdata_usedcommand.text, TE, TA);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 4;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_IFC_set(uint8_t TE, uint8_t TA) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_IFCnn
#define testplace_modem_state_usedstate testplace_modem_state_AT_IFC
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(TE, TA);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(TE, TA);
	}

	if (result == testplace_modem_command_result_Ok) {
		testplace_modem_state_usedstate[0] = TE;
		testplace_modem_state_usedstate[1] = TA;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate[0] = testplace_modem_state_value_unknow;
		testplace_modem_state_usedstate[1] = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_AT_CLIPn(uint8_t newstate) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_CLIP
	sprintf((char*) testplace_modem_message_buffer_output, "%s=%d", (char*) testplace_modem_command_textdata_usedcommand.text, newstate);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 2;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_CLIP_set(uint8_t newstate) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_CLIPn
#define testplace_modem_state_usedstate testplace_modem_state_AT_CLIP
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
	}

	if (result == testplace_modem_command_result_Ok) {
		testplace_modem_state_usedstate = newstate;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_AT_CLTSn(uint8_t newstate) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_CLTS
	sprintf((char*) testplace_modem_message_buffer_output, "%s=%d", (char*) testplace_modem_command_textdata_usedcommand.text, newstate);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 2;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_CLTS_set(uint8_t newstate) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_CLTSn
#define testplace_modem_state_usedstate testplace_modem_state_AT_CLTS
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
	}

	if (result == testplace_modem_command_result_Ok) {
		testplace_modem_state_usedstate = newstate;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_AT_CSCLKn(uint8_t newstate) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_CLTS
	sprintf((char*) testplace_modem_message_buffer_output, "%s=%d", (char*) testplace_modem_command_textdata_usedcommand.text, newstate);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 2;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_CSCLK_set(uint8_t newstate) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_CSCLKn
#define testplace_modem_state_usedstate testplace_modem_state_AT_CSCLK
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
	}

	if (result == testplace_modem_command_result_Ok) {
		testplace_modem_state_usedstate = newstate;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_AT_CMGHEXn(uint8_t newstate) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_CMGHEX
	sprintf((char*) testplace_modem_message_buffer_output, "%s=%d", (char*) testplace_modem_command_textdata_usedcommand.text, newstate);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 2;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_CMGHEX_set(uint8_t newstate) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_CMGHEXn
#define testplace_modem_state_usedstate testplace_modem_state_AT_CMGHEX
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
	}

	if (result == testplace_modem_command_result_Ok) {
		testplace_modem_state_usedstate = newstate;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_AT_HTTPINIT() {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_HTTPINIT
	sprintf((char*) testplace_modem_message_buffer_output, "%s", (char*) testplace_modem_command_textdata_usedcommand.text);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 2;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_HTTPINIT_set() {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_HTTPINIT
#define testplace_modem_state_usedstate testplace_modem_state_AT_HTTPINIT
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction();
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction();
	}

	if (result == testplace_modem_command_result_Ok) {
		testplace_modem_state_usedstate = 1;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_AT_SAPBR(uint8_t cmd_type, uint8_t cid, testPlace_modem_textData *parametrs) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_SAPBR
	sprintf((char*) testplace_modem_message_buffer_output, "%s=%d,%d%s", (char*) testplace_modem_command_textdata_usedcommand.text);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 2;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_SAPBR_set(uint8_t cmd_type, uint8_t cid, testPlace_modem_textData *parametrs) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_SAPBR
//#define testplace_modem_state_usedstate testplace_modem_state_AT_SAPBR
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(cmd_type, cid, parametrs);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(cmd_type, cid, parametrs);
	}

	if (result == testplace_modem_command_result_Ok) {
		//testplace_modem_state_usedstate = 1;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

uint8_t testplace_modem_message_action_simple_sendcommand_AT_HTTPPARA(testPlace_modem_textData *tag, testPlace_modem_textData *value) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_HTTPPARA
	sprintf((char*) testplace_modem_message_buffer_output, "%s=\"%s\",\"%s\"", (char*) testplace_modem_command_textdata_usedcommand.text, (*tag).text, (*value).text);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 6 + (*tag).length + (*value).length;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_HTTPPARA_set(testPlace_modem_textData *tag, testPlace_modem_textData *value) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_HTTPPARA
//#define testplace_modem_state_usedstate testplace_modem_state_AT_HTTPINIT
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(tag, value);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(tag, value);
	}

	if (result == testplace_modem_command_result_Ok) {
		//testplace_modem_state_usedstate = 1;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		//testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

// TODO: AT+HTTPACTION -+
uint8_t testplace_modem_message_action_simple_sendcommand_AT_HTTPACTION(uint8_t newstate) {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_HTTPACTION
	sprintf((char*) testplace_modem_message_buffer_output, "%s=%d", (char*) testplace_modem_command_textdata_usedcommand.text, newstate);
	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 2;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_HTTPACTION_set(uint8_t newstate) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_HTTPACTION
//#define testplace_modem_state_usedstate testplace_modem_state_AT_CMGHEX
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(newstate);
	}

	if (result == testplace_modem_command_result_Ok) {
		//testplace_modem_state_usedstate = newstate;
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		//testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

// TODO: AT+HTTPDATA -+
uint8_t testplace_modem_message_action_simple_sendcommand_AT_HTTPDATA() {
#define testplace_modem_command_textdata_usedcommand testPlace_modem_command_textdata_AT_HTTPPARA

	sprintf((char*) testplace_modem_message_buffer_output, "%s=%u,%u", (char*) testplace_modem_command_textdata_usedcommand.text, length, delay);

	uint8_t size = 0;
	do {
		length /= 10;
		++size;
	} while (length);
	uint8_t size2 = 0;
	do {
		delay /= 10;
		++size2;
	} while (delay);

	testplace_modem_message_buffer_output_textdata.length = testplace_modem_command_textdata_usedcommand.length + 6 + size + (*value).size2;
	return testplace_moedm_message_action_complex_sendcommand_confirmed(&testplace_modem_message_buffer_output_textdata);
}

uint8_t testplace_modem_message_action_simple_runcommand_AT_HTTPDATA_set(uint32_t length, uint32_t delay) {
#define testplace_modem_message_action_simple_usedaction testplace_modem_message_action_simple_sendcommand_AT_HTTPDATA
//#define testplace_modem_state_usedstate testplace_modem_state_AT_HTTPINIT
	static uint8_t _is_worked = 0;
	uint8_t result = testplace_modem_command_result_Unknow;

	void work_stop() {
		_is_worked = testplace_modem_state_value_off;
		testplace_modem_state_busy = testplace_modem_state_value_off;
		Array_uint8_t_Fill(testplace_modem_message_buffer_output, testplace_modem_message_buffer_output_length, 0);
	}

	if (testplace_modem_state_busy == 0 && _is_worked == 0) {
		result = testplace_modem_message_action_simple_usedaction(length, delay);
		_is_worked = 1;
		testplace_modem_state_busy = 1;
	} else if (testplace_modem_state_busy == 1 && _is_worked == 1) {
		result = testplace_modem_message_action_simple_usedaction(length, delay);
	}

	if (result == testplace_modem_command_result_wait_data) {
		work_stop();
	} else if (result != testplace_modem_command_result_Unknow && result != testplace_modem_command_result_work_processing) {
		//testplace_modem_state_usedstate = testplace_modem_state_value_unknow;
		work_stop();
	}
	return result;
}

#pragma regon Modem Func
uint8_t testplace_modem_setup(uint8_t reset_it) {
	if (testplace_modem_state_power == 1) {
		static uint8_t funcNumber = 0;

		if (reset_it) {
			funcNumber = 0;
		}

		switch (funcNumber) {
			case 0:
				if (testplace_modem_message_action_simple_runcommand_ATE_set(0) != testplace_modem_command_result_work_processing) {
					funcNumber++;
				}
				break;
			case 1:
				if (testplace_modem_message_action_simple_runcommand_AT_IFC_set(1, 1) != testplace_modem_command_result_work_processing) {
					funcNumber++;
				}
				break;
			case 2:
				if (testplace_modem_message_action_simple_runcommand_AT_CLIP_set(1) != testplace_modem_command_result_work_processing) {
					funcNumber++;
				}
				break;
			case 3:
				if (testplace_modem_message_action_simple_runcommand_AT_CLTS_set(1) != testplace_modem_command_result_work_processing) {
					funcNumber++;
				}
				break;
			case 4:
				if (testplace_modem_message_action_simple_runcommand_AT_CSCLK_set(0) != testplace_modem_command_result_work_processing) {
					funcNumber++;
				}
				break;
			case 5:
				if (testplace_modem_message_action_simple_runcommand_AT_CMGHEX_set(1) != testplace_modem_command_result_work_processing) {
					funcNumber++;
				}
				break;
			default:
				return 1;
				break;
		}
	} else {
		testPlace_modem_message_action_simple_runcommand_simpletest();
	}
	return 0;
}
// TODO: ATE0 -+
// TODO: AT+IFC=1,1 -+
// TODO: AT+CLIP=1 -+
// TODO: AT+CLTS=1 -+
// TODO: AT+CSCLK=0 -+
// TODO: AT+CMGHEX=1 -+
// TODO: AT+HTTPINIT -+
// TODO: AT+SAPBR=3,1,"CONTYPE","GPRS" -+
// TODO: AT+SAPBR=3,1,"APN","internet.mts.ru" -+
// TODO: AT+SAPBR=1,1 -+
// TODO: AT+HTTPPARA="URL","http://urv.iot.turtton.ru/api/log/ep\" -+
// TODO: Setup -+

// TODO: SendData
uint8_t testPlace_modem_message_action_complex_send_data(testPlace_modem_textData *data) {
	uint8_t is_wait_data = 0;
	if (testplace_modem_message_action_simple_runcommand_AT_HTTPDATA_set((*data).length, 1000) == testplace_modem_command_result_wait_data) {
		is_wait_data = 1;
	}

	if (is_wait_data) {
		testplace_modem_message_action_simple_rightsendcommand(data);
	}
}
#pragma region !!!!Hard Code!!!!
#pragma region Wialon

static const unsigned short crc16_table[256] = { 0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1,
		0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 0x4400, 0x84C1,
		0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 };
unsigned short crc16(const void *data, unsigned data_size) {
	if (!data || !data_size)
		return 0;
	unsigned short crc = 0;
	unsigned char *buf = (unsigned char*) data;
	while (data_size--)
		crc = (crc >> 8) ^ crc16_table[(unsigned char) crc ^ *buf++];
	return crc;
}

const uint8_t testplace_login_data[] = { 0x24, 0x24,         // head
		0x00,           // packet id (login) -> packet type (login)
		0x00, 0x01,         // packet number
		0x00, 0x14,         // length
		0x01,           // protocol version
		0x44,           // pwd and login are string type
		0x38, 0x36, 0x33, 0x30, 0x35, 0x31, 0x30, 0x36, 0x37, 0x31, 0x31, 0x37, 0x38, 0x37, 0x38, 0x00, // login
		0x30, 0x00           // pswd string
		};
unsigned short testplace_crc_local = crc16(testplace_login_data, 27);

//"AT+HTTPPARA=\"URL\",\"http://urv.iot.turtton.ru/api/log/ep\"\n"
testPlace_modem_textData testplace_tag_URL = { "URL", 3 };
testPlace_modem_textData testplace_url_wialon = { "nl.gpsgsm.org:22022", 19 };

//if (modem_Do_SendCommand_Confirmed((uint8_t*) "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\n", 30)) {
//				++thirdFunc_ActionNumber;
//			}
//			break;
//		case 2:
//			if (modem_Do_SendCommand_Confirmed((uint8_t*) "AT+SAPBR=3,1,\"APN\",\"internet.mts.ru\"\n", 37)) {
//				++thirdFunc_ActionNumber;
//			}
//			break;
//		case 3:
//			if (modem_Do_SendCommand_Confirmed((uint8_t*) "AT+SAPBR=1,1\n", 13)) {

testPlace_modem_textData testplace_textdata_contype_grps = { ",\"CONTYPE\",\"GPRS\"", 17 };
testPlace_modem_textData testplace_textdata_apn_mts = { ",\"APN\",\"internet.mts.ru\"", 26 };
testPlace_modem_textData testplace_textdata_void = { "", 0 };

uint8_t testplace_modem_prepare_to_server_work(uint8_t reset_it) {
	static uint8_t func_number = 0;
	if (reset_it) {
		func_number = 0;
	}

	switch (testplace_modem_message_action_simple_runcommand_AT_HTTPINIT_set()) {
		case 0:
			if (testplace_modem_message_action_simple_runcommand_AT_HTTPINIT_set()) {
				func_number++;
			}
			break;
		case 2:
			if (testplace_modem_message_action_simple_runcommand_AT_SAPBR_set(3, 1, &testplace_textdata_contype_grps)) {
				func_number++;
			}
			break;
		case 3:
			if (testplace_modem_message_action_simple_runcommand_AT_SAPBR_set(3, 1, &testplace_textdata_apn_mts)) {
				func_number++;
			}
			break;
		case 4:
			if (testplace_modem_message_action_simple_runcommand_AT_SAPBR_set(1, 1, &testplace_textdata_void)) {
				func_number++;
			}
			break;
		case 5:
			if (testplace_modem_message_action_simple_runcommand_AT_HTTPPARA_set(&testplace_tag_URL, &testplace_url_wialon)) {
				func_number++;
			}
			break;
		default:
			break;
	}
}

uint8_t testplace_Wialon_Login(uint8_t *login, uint8_t *password) {
	// TODO: AT+HTTPDATA=33,10000
	// testplace_data_login
	// TODO: AT+HTTPACTION=1
}

uint8_t testplace_Wialon_Data_Send() {
	// TODO: AT+HTTPDATA=33,10000
	// testplace_data_data
	// TODO: AT+HTTPACTION=1
}

// TODO: Wialon login
// TODO: Wialon send data
