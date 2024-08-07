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
#define testplace_modem_command_response
#define testPlace_modem_message
#define testPlace_modem_message_buffer
#define testPlace_modem_message_action
#define testPlace_modem_message_action_simple
#define testPlace_modem_message_action_complex

#pragma region testplace modem message buffer
//Буферы
//Буфер приема
#define testplace_modem_message_buffer_input_length 512
uint8_t testplace_modem_message_buffer_input[testplace_modem_message_buffer_input_length] = { 0 };

//Буфер отправки
#define testplace_modem_message_buffer_outpu_length 512
uint8_t testplace_modem_message_buffer_output[testplace_modem_message_buffer_outpu_length] = { 0 };

//Структура текста
#pragma region testPlace Struct
typedef struct testPlace_modem_textData {
	uint8_t *text;
	size_t length;
} testPlace_modem_textData;

#pragma region testplace Modem Commands
//Список команд доступных для отправки
testPlace_modem_textData testPlace_modem_command_AT = { (uint8_t*) "AT", 2 };

#pragma region testplace Modem Commands
//Список ответов при выполнении команд
testPlace_modem_textData testplace_modem_command_response_OK = { (uint8_t*) "OK", 2 };
testPlace_modem_textData testplace_modem_command_response_ERROR = { (uint8_t*) "ERROR", 5 };

#pragma region testplace Modem Func
//Функция отправки команд
//На вход получает комманду которую необходимо отправить
//Ничего не возвращает
void testplace_modem_message_action_simple_sendcommand(testPlace_modem_textData *command) {
	sprintf((char*) testplace_modem_message_buffer_output, "\s\c", (char*) (*command).text, '\n');
	HAL_UART_Transmit_IT(&huart1, testplace_modem_message_buffer_output, (*command).length + 1);
}

//Функция запуска ожидания текста на входе
//Запускает ожидание HAL_UART
void testplace_modem_message_action_simple_waittext() {
	HAL_UART_Receive_IT(&huart1, testplace_modem_message_buffer_output, testplace_modem_message_buffer_outpu_length);
}
testPlace_modem_message_buffer

//функция получения результата выполнения команды
enum testplace_modem_command_result {
	testplace_modem_command_result_Unknow, testplace_modem_command_result_TimeOut, testplace_modem_command_result_Error, testplace_modem_command_result_Ok,
} testplace_modem_command_result;

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
			if (Text_IsFindedIn((char*) testplace_modem_message_buffer_input, testplace_modem_message_buffer_input_length, (char*) testplace_modem_command_response_OK.text, testplace_modem_command_response_OK.length)) {
				return return_result_end(testplace_modem_command_result_Ok);
			} else if ((Text_IsFindedIn((char*) testplace_modem_message_buffer_input, testplace_modem_message_buffer_input_length, (char*) testplace_modem_command_response_ERROR.text, testplace_modem_command_response_ERROR.length))) {
				return return_result_end(testplace_modem_command_result_Error);
			}
		} else {
			return return_result_end(testplace_modem_command_result_TimeOut);
		}
	} else {
		testplace_modem_message_action_simple_waittext();
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
	testplace_moedm_message_action_complex_sendcommand_confirmed(&testPlace_modem_command_AT);
}
