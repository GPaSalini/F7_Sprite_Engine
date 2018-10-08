/*
 * sprite_lib.h
 *
 *  Created on: 21-12-2017
 *      Author: Giordano
 */

#ifndef INC_SPRITE_LIB_H_
#define INC_SPRITE_LIB_H_

//========== Incluciones ==========
#include "sprite_config.h"
#include "sprite_graphics.h"
#include "stm32f7xx_hal.h"
#include "Graphics.h"
#include "stm32f769i_discovery_lcd.h"
#include "math.h"


//========== Definiciones: Direccion del Buffer ==========
#define FRAME_BUFFER	((uint32_t)0xC0000000)
#define RENDER_BUFFER	((uint32_t)0xC0200000)

//========== Tipos de HitBox ==========
typedef enum {
	RectHitBox = 1,
	CircHitbox
} Sprite_Type_HitBox;


//========== Definiciones: Estructuras ==========

//Estructura para las propiedades de la instancia
typedef struct _Sprite_Type_Instance
{
	int8_t		pos;		//Posicion de la instancia
	uint8_t		parent;		//ID del objeto
	uint8_t 	layer;		//Layer de la instancia

	uint16_t	xsubimg;	//Posicion de la subimagen actual en la columna
	uint16_t	ysubimg;	//Columna de la subimagen actual
	uint16_t	asubimg;	//Subimagen inicial para la animacion
	uint16_t	bsubimg;	//Subimagen final para la animacion
	int8_t		tsubimg;	//Tasa total de fotogramas para la animacion
	uint8_t		fsubimg;	//Fotograma actual de la animacion

	uint16_t 	x;			//Pos x actual
	uint16_t	y;			//Pos y actual
	uint16_t	xstart; 	//x inicial
	uint16_t	ystart; 	//y inicial
	uint16_t	xprev;		//Pos x anterior
	uint16_t	yprev;		//Pos y anterior
	int8_t		vx;			//Velocidad en el eje x
	int8_t		vy;			//Velocidad en el eje y

	BOOL		cte;		//La instancia permanece constante entre Rooms
	BOOL		vis;		//Visibilidad del sprite
	BOOL		step;		//Disponible actualizacion de la instancia
	BOOL		anim;		//Sprite de la instancia esta siendo animado

	int16_t		var[MAX_VAR];	//Variables internas [(0) ~ (MAX_VAR-1)]

	//Puntero a la funcion Task de la instancia
	void (*taskIns)(uint8_t par, int8_t pos);

} Sprite_Type_Instance;

//Estructura para las propiedades del objeto
typedef struct _Sprite_Type_Object
{
	uint8_t		id;			//ID del objeto
	void		*next;		//Puntero al siguiente objeto
	void		*ins;		//Puntero a la estructura de instancias asociada
	uint8_t		maxins;		//Maximo de instancias
	uint8_t		nins;		//Numero de instancias actuales

	void		*image;		//Puntero de la Imagen del objeto
	uint16_t 	width;		//Ancho de la subimagen
	uint16_t	height; 	//Alto del subimagen
	uint16_t	xsubimg;	//Numero de subimagenes por columna
	uint16_t	ysubimg;	//Numero de columnas de subimagenes

	uint16_t	xhitbox;	//X inicial del hitbox
	uint16_t	yhitbox;	//y inicial del hitbox
	uint16_t	whitbox;	//Ancho del hitbox
	uint16_t	hhitbox;	//Alto del hitbox
	uint8_t		rhitbox;	//Radio del hitbox
	uint8_t		thitbox;	//Tipo de hitbox: rectangular/circular

	int16_t		var[MAX_VAR];	//Variables internas [(0) ~ (MAX_VAR-1)]

	//Puntero a la funcion Task del objeto
	void (*taskObj)(uint8_t par, int8_t pos);

} Sprite_Type_Object;


//Estructura para las propiedades del Tileset
typedef struct _Sprite_Type_Tileset
{
	uint8_t		layer;		//ID de la capa del Tileset
	uint8_t 	room;		//Numero de donde estara el Tileset
	void		*next;		//Puntero al siguiente Tileset

	void		*image;		//Puntero de la Imagen del Tileset
	uint16_t	wgrid;		//Ancho de casillas
	uint16_t	hgrid;		//Alto de casillas
	uint16_t	xi;			//Posicion x inicial de la capa de Tiles
	uint16_t	yi;			//Posicion y inicial de la capa de Tiles
	uint16_t	width;		//Ancho de la capa de Tileset
	uint16_t	height;		//Alto de la capa de Tileset
	uint16_t	xtiles;		//Numero de Tiles por columna (Tileset)
	uint16_t	ytiles;		//Numero de columnas del Tileset
	uint16_t	xroom;		//Numero de casillas por columna (Room)
	uint16_t	yroom;		//Numero de columnas de Tiles en la Room

	//Matriz del mapa de datos
	uint16_t	maxdata;
	uint8_t		*data;

	BOOL		visible;	//Visibilidad del Tileset

}Sprite_Type_Tileset;

//Estructura para una Pantalla
typedef struct _Sprite_Type_Screen
{
	BOOL		Activated;	//Si la Screen esta activada se dibujara sobre ella

	uint16_t	xScreen;	//Coordenada x de la Screen en la pantalla
	uint16_t	yScreen;	//Coordenada y de la Screen en la pantalla
	uint16_t	width;		//Ancho de la Screen
	uint16_t	height;		//Alto de la Screen

	uint16_t	xCur;		//Posicion x actual de la Screen en la Room
	uint16_t	yCur;		//Posicion y actual de la Screen en la Room
	uint16_t	xNext;		//Posicion x siguiente de la Screen en la Room
	uint16_t	yNext;		//Posicion y siguiente de la Screen en la Room
	uint16_t	xPrev;		//Posicion x anterior de la Screen en la Room
	uint16_t	yPrev;		//Posicion y anterior de la Screen en la Room

} Sprite_Type_Screen;


//========== Funciones Privadas: Prototipos ==========

//Funciones del DMA2D
void DMA2D_Init(void);
void DMA2D_ClearBuffer(uint32_t *pDst);
void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst);
void DMA2D_DrawImage(uint32_t *pImg, uint32_t *pDst, uint16_t width);
void DMA2D_FillRect(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint32_t col);

//Configuracion Principal
void Step_Config(uint16_t step);
void Graph_Config(BOOL refresh, uint32_t bkcolor);

//Creacion de Pantalla
void Screen_Create(uint8_t ID, uint16_t xscreen, uint16_t yscreen, uint16_t wscreen, uint16_t hscreen, uint16_t xi, uint16_t yi);

//Funciones de Objetos
BOOL Object_Create(uint8_t ID, Sprite_Type_Object *obj, Sprite_Type_Instance *ins, uint8_t max, void (*taskobj)(uint8_t par, int8_t pos), void *imagen, uint16_t wi, uint16_t hi, uint8_t xsub, uint8_t ysub);
BOOL Object_SetHitbox(uint8_t par, uint16_t xhb, uint16_t yhb, uint16_t whb, uint16_t hhb);
void Object_Add(Sprite_Type_Object *obj);
Sprite_Type_Object* Object_Get(uint8_t par);

//Funciones de Instancias
int8_t Instance_Create(uint8_t par, int8_t lay, uint8_t xsub, uint8_t ysub, uint16_t xi, uint16_t yi, BOOL visible);
BOOL Instance_SetAnim(uint8_t par, int8_t pos, uint8_t ra, uint8_t rb, uint8_t ry, uint8_t ti);
BOOL Instance_Clean(uint8_t par, int8_t pos);
BOOL Instance_Free(uint8_t par, int8_t pos);

//Funciones para Tileset
void Tileset_Create(uint8_t layer, uint8_t nroom, Sprite_Type_Tileset *tile, void *imagen, uint16_t wgrid, uint16_t hgrid, uint16_t xi, uint16_t yi, uint16_t width, uint16_t height);
BOOL Tileset_SetMap(uint8_t layer, uint8_t data, uint16_t xt, uint16_t yt,uint16_t size);
void Tileset_Add(Sprite_Type_Tileset *tile);
BOOL Tileset_Free(Sprite_Type_Tileset *tile);
Sprite_Type_Tileset* Tileset_Get(uint8_t layer);

//Cambio de habitacion
BOOL Room_Goto(uint8_t room);

//Funciones Principales
void Step_Sprites(void);
void Task_Sprite(uint8_t par, int8_t pos);
void Anim_Sprite(uint8_t par, int8_t pos);

//Funciones de Dibujo
void Draw_Sprite(uint8_t par, int8_t pos, uint16_t xi, uint16_t yi);
void Draw_Tile(Sprite_Type_Tileset* tile, uint8_t dpos);
void Draw_ImagePartial(uint16_t left, uint16_t top, Sprite_Image *image, uint16_t xoffset, uint16_t yoffset, uint16_t width, uint16_t height);

//Funciones utiles
int16_t Random_intGet(int16_t i1 , int16_t i2);
uint16_t Point_GetAngle(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
uint16_t Point_GetDistance(int16_t x1,int16_t y1, int16_t x2, int16_t y2);
void Move_Direction(uint8_t par, int8_t pos, uint16_t dir, int16_t dist);

//Funciones de Colision
BOOL Collision_RectangleInstance(uint8_t par, int8_t pos, uint16_t xa, uint16_t ya, uint16_t xb, uint16_t yb, int16_t xi, int16_t yi);
BOOL Collision_SpriteInstance(uint8_t par1, int8_t pos1,uint8_t par2, int8_t pos2, int16_t xi, int16_t yi);
BOOL Collision_HitboxInstance(uint8_t par1, int8_t pos1,uint8_t par2, int8_t pos2, int16_t xi, int16_t yi);

#endif /* INC_SPRITE_LIB_H_ */
