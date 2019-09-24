/*
 * sprite_graphics.h
 *
 *  Created on: 04-06-2018
 *      Author: Giordano
 */

#ifndef INC_SPRITE_GRAPHICS_H_
#define INC_SPRITE_GRAPHICS_H_

#include <stdint.h>

//Estructura para una imagen
typedef struct _Sprite_Image
{
	uint16_t width;
	uint16_t height;
	uint32_t *data;
} Sprite_Image;

#endif /* INC_SPRITE_GRAPHICS_H_ */
