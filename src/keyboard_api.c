/*
 * keyboard_api.c
 *
 *  Created on: 26-12-2017
 *      Author: Giordano
 */

#include "keyboard_api.h"

//Variables Globales
uint8_t host_state;
Keys_Pressed output;

//Constantes del teclado
static  const  int8_t  HID_KEYBRD_Key[] = {
  '\0',  '`',  '1',  '2',  '3',  '4',  '5',  '6',
  '7',  '8',  '9',  '0',  '-',  '=',  '\0', '\r',
  '\t',  'q',  'w',  'e',  'r',  't',  'y',  'u',
  'i',  'o',  'p',  '[',  ']',  '\\',
  '\0',  'a',  's',  'd',  'f',  'g',  'h',  'j',
  'k',  'l',  ';',  '\'', '\0', '\n',
  '\0',  '\0', 'z',  'x',  'c',  'v',  'b',  'n',
  'm',  ',',  '.',  '/',  '\0', '\0',
  '\0',  '\0', '\0', ' ',  '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '\0', '\0', '\0', '\r', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '7',  '4',  '1',
  '\0',  '/',  '8',  '5',  '2',
  '0',   '*',  '9',  '6',  '3',
  '.',   '-',  '+',  '\0', '\n', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0'
};

static  const  int8_t  HID_KEYBRD_ShiftKey[] = {
  '\0', '~',  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
  '_',  '+',  '\0', '\0', '\0', 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',
  'I',  'O',  'P',  '{',  '}',  '|',  '\0', 'A',  'S',  'D',  'F',  'G',
  'H',  'J',  'K',  'L',  ':',  '"',  '\0', '\n', '\0', '\0', 'Z',  'X',
  'C',  'V',  'B',  'N',  'M',  '<',  '>',  '?',  '\0', '\0',  '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0',    '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};

static  const  uint8_t  HID_KEYBRD_Codes[] = {
  0,     0,    0,    0,   31,   50,   48,   33,
  19,   34,   35,   36,   24,   37,   38,   39,       /* 0x00 - 0x0F */
  52,    51,   25,   26,   17,   20,   32,   21,
  23,   49,   18,   47,   22,   46,    2,    3,       /* 0x10 - 0x1F */
  4,    5,    6,    7,    8,    9,   10,   11,
  43,  110,   15,   16,   61,   12,   13,   27,       /* 0x20 - 0x2F */
  28,   29,   42,   40,   41,    1,   53,   54,
  55,   30,  112,  113,  114,  115,  116,  117,       /* 0x30 - 0x3F */
  118,  119,  120,  121,  122,  123,  124,  125,
  126,   75,   80,   85,   76,   81,   86,   89,       /* 0x40 - 0x4F */
  79,   84,   83,   90,   95,  100,  105,  106,
  108,   93,   98,  103,   92,   97,  102,   91,       /* 0x50 - 0x5F */
  96,  101,   99,  104,   45,  129,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x60 - 0x6F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x70 - 0x7F */
  0,    0,    0,    0,    0,  107,    0,   56,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x80 - 0x8F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x90 - 0x9F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xA0 - 0xAF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xB0 - 0xBF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xC0 - 0xCF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xD0 - 0xDF */
  58,   44,   60,  127,   64,   57,   62,  128        /* 0xE0 - 0xE7 */
};

//Guarda las teclas presionadas
void USBH_HID_GetKeyCode(HID_KEYBD_Info_TypeDef *info)
{
 	if((info->lshift) || (info->rshift))
		for (uint8_t i=0;i<6;i++)
		{
			output.ascii[i] =  HID_KEYBRD_ShiftKey[HID_KEYBRD_Codes[info->keys[i]]];
			output.keys[i] = info->keys[i];
		}
  	else
  		for (uint8_t i=0;i<6;i++)
		{
			output.ascii[i] = HID_KEYBRD_Key[HID_KEYBRD_Codes[info->keys[i]]];
			output.keys[i] = info->keys[i];
		}
}

//Obtiene las teclas presionadas
void getKeyboardInput(USBH_HandleTypeDef *phost)
{
	if(host_state == HOST_USER_CLASS_ACTIVE)
	{
		HID_KEYBD_Info_TypeDef *kb_info = USBH_HID_GetKeybdInfo(phost);
		if(kb_info != NULL)
		{
			for (uint8_t i=0;i<6;i++)
			{
				output.ascii[i] = 0;
				output.keys[i] = 0;
			}
			USBH_HID_GetKeyCode(kb_info);
		}
	}
}

//Revisa si se presiona la tecla con el codigo ASCII indicado
uint8_t Keyboard_checkASCII(char code)
{
	if(host_state == HOST_USER_CLASS_ACTIVE)
		for (uint8_t i=0;i<6;i++)
			if (output.ascii[i]==code)
				return TRUE;

	return FALSE;
}

//Revisa si se presiona la tecla indicada
uint8_t Keyboard_checkKEY(char code)
{
	if(host_state == HOST_USER_CLASS_ACTIVE)
		for (uint8_t i=0;i<6;i++)
			if (output.keys[i]==code)
				return TRUE;

	return FALSE;
}

//Retorna el primer ASCII presionado del teclado
uint8_t Keyboard_getASCII(void)
{
	if(host_state == HOST_USER_CLASS_ACTIVE)
		for (uint8_t i=0;i<6;i++)
			if (output.ascii[i]!=0)
				return output.ascii[i];

	return 0;
}
