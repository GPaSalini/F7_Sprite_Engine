/*
 * keyboard_api.h
 *
 *  Created on: 26-12-2017
 *      Author: Giordano
 */

#ifndef INC_KEYBOARD_API_H_
#define INC_KEYBOARD_API_H_

#include "usbh_hid.h"
#include "usbh_core.h"

//Estructura para las teclas presionadas
typedef struct _Keys_Pressed
	{
	uint8_t	ascii[6];	//Letras presionadas
	uint8_t keys[6];	//Botones presionados
} Keys_Pressed;

//Funciones Privadas: Prototipos
void USBH_HID_GetKeyCode(HID_KEYBD_Info_TypeDef *info);
void getKeyboardInput(USBH_HandleTypeDef *phost);
uint8_t Keyboard_checkASCII(char code);
uint8_t Keyboard_checkKEY(char code);
uint8_t Keyboard_getASCII(void);

#endif /* INC_KEYBOARD_API_H_ */
