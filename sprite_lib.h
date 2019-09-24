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
#include "stm32f769i_discovery_lcd.h"
#include "math.h"
#include "stdlib.h"


//========== Definiciones: Direccion del Buffer ==========
#define FRAME_BUFFER	((uint32_t)0xC0000000)
#define RENDER_BUFFER	((uint32_t)0xC0400000)

//========== Tipos de HitBox ==========
typedef enum {
	RectHitBox = 1,
	CircHitbox
} Sprite_Type_HitBox;


//========== Definiciones: Estructuras ==========

//Estructura para el hitbox de una instancia
typedef struct _Sprite_Type_Hitbox
{
	uint8_t		type;	//Tipo de hitbox
	int16_t		x,y;    //(x,y) inicial del hitbox respecto al tamaño de subimagen
	uint16_t	w,h;	//Ancho y alto del hitbox
	uint8_t		r;		//Radio del hitbox
} Sprite_Type_Hitbox;

//Estructura para las propiedades de la instancia
typedef struct _Sprite_Type_Instance
{
	uint8_t		parent;		//ID del objeto
	int8_t 		layer;		//Layer de la instancia

	struct _Sprite_Type_Instance	*ins_prev;
	struct _Sprite_Type_Instance	*ins_next;

	uint16_t	xsubimg;	//Posicion de la subimagen actual en la columna
	uint16_t	ysubimg;	//Columna de la subimagen actual
	uint16_t	asubimg;	//Subimagen inicial para la animacion
	uint16_t	bsubimg;	//Subimagen final para la animacion
	int8_t		tsubimg;	//Tasa total de fotogramas para la animacion
	uint8_t		fsubimg;	//Fotograma actual de la animacion

	int16_t x;			//Pos x actual
	int16_t	y;			//Pos y actual
	int16_t	xstart; 	//x inicial
	int16_t	ystart; 	//y inicial
	int16_t	xprev;		//Pos x anterior
	int16_t	yprev;		//Pos y anterior
	int8_t	vx;			//Velocidad en el eje x
	int8_t	vy;			//Velocidad en el eje y

	Sprite_Type_Hitbox hitbox;	//Hitbox de la instancia

	BOOL		cte;		//La instancia permanece constante entre Rooms
	BOOL		vis;		//Visibilidad del sprite
	BOOL		step;		//Disponible actualizacion de la instancia
	BOOL		destroy;	//La instancia sera destruida
	BOOL		anim;		//Sprite de la instancia esta siendo animado

	int16_t		var[MAX_VAR];	//Variables internas [(0) ~ (MAX_VAR-1)]

	//Puntero a la funcion Task de la instancia
	void (*taskIns)(struct _Sprite_Type_Instance *pins);

	//Puntero a la funcion Draw de la instancia
	void (*drawIns)(struct _Sprite_Type_Instance *pins);

} Sprite_Type_Instance;

typedef struct _Sprite_List_Instance
{
	int8_t layer;
	Sprite_Type_Instance *ins;
	struct _Sprite_List_Instance *next;
	struct _Sprite_List_Instance *prev;
} Sprite_List_Instance;

//Estructura para las propiedades del objeto
typedef struct _Sprite_Type_Object
{
	uint8_t		id;			//ID del objeto
	uint8_t		nins;		//Numero de instancias actuales

	//Puntero al siguiente objeto
	struct _Sprite_Type_Object	*next;

	//Puntero a las instancias hijas
	Sprite_Type_Instance	*ins;

	Sprite_Image	*image; //Puntero de la Imagen del objeto
	uint16_t 	width;		//Ancho de la subimagen
	uint16_t	height; 	//Alto del subimagen
	uint16_t	xsubimg;	//Numero de subimagenes por columna
	uint16_t	ysubimg;	//Numero de columnas de subimagenes

	Sprite_Type_Hitbox hitbox;	//Hitbox del objeto

	int16_t		var[MAX_VAR];	//Variables internas [(0) ~ (MAX_VAR-1)]

	//Puntero a la funcion Task del objeto
	void (*taskObj)(Sprite_Type_Instance* pins);

	//Puntero a la funcion Draw de la instancia
	void (*drawObj)(Sprite_Type_Instance *pins);

} Sprite_Type_Object;


//Estructura para una Pantalla
typedef struct _Sprite_Type_Screen
{
	BOOL		Activated;	//Si la Screen esta activada se dibujara sobre ella

	uint16_t	xScreen;	//Coordenada x de la Screen en la pantalla fisica
	uint16_t	yScreen;	//Coordenada y de la Screen en la pantalla fisica
	uint16_t	width;		//Ancho de la Screen
	uint16_t	height;		//Alto de la Screen

	int16_t		xCur;		//Posicion x actual de la Screen en la Room
	int16_t		yCur;		//Posicion y actual de la Screen en la Room
	int16_t		xNext;		//Posicion x siguiente de la Screen en la Room
	int16_t		yNext;		//Posicion y siguiente de la Screen en la Room
	int16_t		xPrev;		//Posicion x anterior de la Screen en la Room
	int16_t		yPrev;		//Posicion y anterior de la Screen en la Room

} Sprite_Type_Screen;


//========== Funciones Privadas: Prototipos ==========

//Funciones del DMA2D
void DMA2D_ClearBuffer(uint32_t *pDst);
void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst);
void DMA2D_DrawImage(uint32_t *pImg, uint32_t *pDst, uint16_t width);
void DMA2D_FillRect(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint32_t col);

//Configuracion Principal
void Step_Config(uint16_t step);
void Refresh_Config(BOOL refresh);
void Background_Config(uint32_t color);
void Text_Config(sFONT* font, uint32_t color);

//Creacion de Pantalla
void Screen_Create(uint8_t ID, uint16_t xscreen, uint16_t yscreen, uint16_t wscreen, uint16_t hscreen, int16_t xi, int16_t yi);

//Funciones de Objetos
BOOL Object_Create(uint8_t ID, Sprite_Type_Object *obj, void (*taskobj)(Sprite_Type_Instance* ins), Sprite_Image *imagen, uint16_t wi, uint16_t hi, uint8_t xsub, uint8_t ysub);
BOOL Object_SetHitbox(uint8_t par, int16_t x, int16_t y, uint16_t w, uint16_t h);
static void Object_Add(Sprite_Type_Object *obj);
Sprite_Type_Object* Object_Get(uint8_t par);

//Funciones de Instancias
Sprite_Type_Instance* Instance_Create(uint8_t par, int8_t lay, uint8_t xsub, uint8_t ysub, int16_t xi, int16_t yi, BOOL visible);
static void Instance_Add(Sprite_Type_Instance *ins);
BOOL Instance_SetAnim(Sprite_Type_Instance* ins, uint8_t ra, uint8_t rb, uint8_t ry, int8_t ti);
BOOL Instance_Destroy(Sprite_Type_Instance* ins);
void Instance_Clean(void);

//Funciones Principales
void Step_Sprites(void);
void Frame_Sprites(void);
void Step_Control(void);
void Task_Sprite(Sprite_Type_Instance* ins);
static void Anim_Sprite(Sprite_Type_Instance* ins);

//Funciones de Dibujo
static void Draw_Sprite(Sprite_Type_Instance *ins, int16_t xi, int16_t yi);
static void Draw_ImagePartial(uint16_t left, uint16_t top, Sprite_Image *image, uint16_t xoffset, uint16_t yoffset, uint16_t width, uint16_t height);
void Draw_StringAt(uint16_t Xpos, uint16_t Ypos, uint8_t *Text);
static void Draw_Char(uint16_t Xpos, uint16_t Ypos, const uint8_t *c);

//Funciones utiles
BOOL RNG_Init(void);
int16_t Rand_Get(int16_t i1 , int16_t i2);
uint16_t Point_GetAngle(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
uint16_t Point_GetDistance(int16_t x1,int16_t y1, int16_t x2, int16_t y2);
void Move_Direction(Sprite_Type_Instance *ins, uint16_t dir, int16_t dist);

//Funciones de Colision
BOOL Collision_RectangleInstance(Sprite_Type_Instance* ins, uint16_t xa, uint16_t ya, uint16_t xb, uint16_t yb, int16_t xi, int16_t yi);
BOOL Collision_HitboxInstance(Sprite_Type_Instance* ins1, Sprite_Type_Instance* ins2, int16_t xi, int16_t yi);

#endif /* INC_SPRITE_LIB_H_ */
