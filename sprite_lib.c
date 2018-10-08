/*
 * sprite_lib.c
 *
 *  Created on: 21-12-2017
 *      Author: Giordano
 */

#include "sprite_lib.h"

//========== Variables Privadas ==========
uint32_t C_Background;
BOOL RefreshBuffer;

uint16_t maxStep;
Sprite_Type_Object *Object_List = NULL;
Sprite_Type_Tileset *Tileset_List = NULL;
uint16_t Count_Instance = 0;

Sprite_Type_Screen Screen[MAX_SCREEN];
uint16_t wRoom[MAX_ROOM];
uint16_t hRoom[MAX_ROOM];
uint8_t Room_Active = 0;

uint32_t tickfirst;
uint32_t ticklast;
uint16_t tickfrec;
int16_t ticktimer;
uint8_t framerate;


//========== Funciones ==========

//Limpia el buffer render con el color de fondo mediante R2M del DMA2D
void DMA2D_ClearBuffer(uint32_t *pDst)
{
	HAL_StatusTypeDef state;
	DMA2D_HandleTypeDef hDma2dHandler;

	hDma2dHandler.Init.Mode         = DMA2D_R2M;
	hDma2dHandler.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
	hDma2dHandler.Init.OutputOffset = 0x0;
	hDma2dHandler.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
	hDma2dHandler.Init.RedBlueSwap   = DMA2D_RB_REGULAR;

	hDma2dHandler.Instance = DMA2D;

	if(HAL_DMA2D_Init(&hDma2dHandler) == HAL_OK)
	{
		state = HAL_DMA2D_Start(&hDma2dHandler, C_Background, (uint32_t)pDst, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
		if (state == HAL_OK) {HAL_DMA2D_PollForTransfer(&hDma2dHandler, 10);}
	}
}


//Copia el buffer render al buffer de pantalla mediante Interrupcion y M2M del DMA2D
void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst)
{
	HAL_StatusTypeDef state;
	DMA2D_HandleTypeDef hDma2dHandler;

	hDma2dHandler.Init.Mode         = DMA2D_M2M;
	hDma2dHandler.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
	hDma2dHandler.Init.OutputOffset = 0x0;
	hDma2dHandler.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
	hDma2dHandler.Init.RedBlueSwap   = DMA2D_RB_REGULAR;

	hDma2dHandler.Instance = DMA2D;

	if(HAL_DMA2D_Init(&hDma2dHandler) == HAL_OK)
	{
		state = HAL_DMA2D_Start(&hDma2dHandler, (uint32_t)pSrc, (uint32_t)pDst, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
		if (state == HAL_OK) {HAL_DMA2D_PollForTransfer(&hDma2dHandler, 10);}
	}
}

//Dibuja una linea de imagen al buffer render aplicando transparencias mediante Blending del DMA2D
void DMA2D_DrawImage(uint32_t *pImg, uint32_t *pDst, uint16_t width)
{
	HAL_StatusTypeDef state;
	DMA2D_HandleTypeDef hDma2dHandler;

	hDma2dHandler.Init.Mode         = DMA2D_M2M_BLEND;
	hDma2dHandler.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
	hDma2dHandler.Init.OutputOffset = 0x0;
	hDma2dHandler.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
	hDma2dHandler.Init.RedBlueSwap   = DMA2D_RB_REGULAR;

	hDma2dHandler.Instance = DMA2D;

	if(HAL_DMA2D_Init(&hDma2dHandler) == HAL_OK)
	{
		state = HAL_DMA2D_BlendingStart(&hDma2dHandler,(uint32_t)pImg,(uint32_t)pDst,(uint32_t)pDst,width,1);
		if (state == HAL_OK) {HAL_DMA2D_PollForTransfer(&hDma2dHandler, 10);}
	}
}


//Dibuja un rectangulo en x1,y1 relleno de un color
void DMA2D_FillRect(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint32_t col)
{
	HAL_StatusTypeDef state;
	DMA2D_HandleTypeDef hDma2dHandler;

	hDma2dHandler.Init.Mode         = DMA2D_R2M;
	hDma2dHandler.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
	hDma2dHandler.Init.OutputOffset = BSP_LCD_GetXSize() - w;
	hDma2dHandler.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
	hDma2dHandler.Init.RedBlueSwap   = DMA2D_RB_REGULAR;

	hDma2dHandler.Instance = DMA2D;

	if(HAL_DMA2D_Init(&hDma2dHandler) == HAL_OK)
	{
		uint32_t Xaddress = (uint32_t)RENDER_BUFFER + 4*(BSP_LCD_GetXSize()*y1 + x1);
		state = HAL_DMA2D_Start(&hDma2dHandler, col, (uint32_t)Xaddress, w, h);
		if (state == HAL_OK) {HAL_DMA2D_PollForTransfer(&hDma2dHandler,10);}
	}
}


//Configura los step que ocurren hasta 1 segundo (fps)
void Step_Config(uint16_t step)
{
	maxStep = step;
	tickfrec = 1000/maxStep;
}

//Configura refresco del buffer render y color de fondo
void Graph_Config(BOOL refresh, uint32_t bkcolor)
{

	//Refresco del Buffer Render
	RefreshBuffer = refresh;

	//Color de fondo del Layer
	C_Background = bkcolor;
	BSP_LCD_SelectLayer(0);
	BSP_LCD_SetBackColor(C_Background);
}

/* Descripcion: Funcion que crea una pantalla dentro de la habitacion actual.
 * -ID: id unica de la pantalla.
 * -xscreen,yscreen: coordenadas de la pantalla.
 * -wscreen,hscreen: ancho y alto de la pantalla.
 * -xi,yi: posicion de la pantalla en la room.
 * Retorno: Nada.
 * *La pantalla viene desactivada por defecto.
 */
void Screen_Create(uint8_t ID, uint16_t xscreen, uint16_t yscreen, uint16_t wscreen, uint16_t hscreen, uint16_t xi, uint16_t yi)
{
	Screen[ID].Activated = FALSE;

	Screen[ID].xScreen = xscreen;
	Screen[ID].yScreen = yscreen;
	Screen[ID].width = wscreen;
	Screen[ID].height = hscreen;

	Screen[ID].xCur = xi;
	Screen[ID].yCur = yi;
	Screen[ID].xNext = xi;
	Screen[ID].yNext = yi;
	Screen[ID].xPrev = xi;
	Screen[ID].yPrev = yi;
}


/* Descripcion: Funcion que crea un objeto configurandolo.
 * -ID: id unica del objeto.
 * -*obj: Puntero del objeto.
 * -*ins: Puntero de la instancia asociada.
 * -max: Maximo de instancias.
 * -*imagen: imagen del objeto.
 * -*taskobj: Puntero del Task.
 * -wi,hi: ancho y alto del sprite.
 * -xsub,ysub: numero de subimagenes por columna y columna de subimagenes.
 * Retorno: Puntero del Sprite_Type_Object.
 */
BOOL Object_Create(uint8_t ID, Sprite_Type_Object *obj, Sprite_Type_Instance *ins, uint8_t max, void (*taskobj)(uint8_t par, int8_t pos), void *imagen, uint16_t wi, uint16_t hi, uint8_t xsub, uint8_t ysub)
{
	//Revisa que exista la estructura de instancia
	if (ins==NULL) {return FALSE;}

	//Configura las propiedades del Sprite
	obj->id = ID;
	obj->maxins = max;
	obj->nins = 0;
	obj->image = imagen;
	obj->width = wi;
	obj->height = hi;
	obj->xsubimg = xsub;
	obj->ysubimg = ysub;
	obj->xhitbox = 0;
	obj->yhitbox = 0;
	obj->whitbox = wi;
	obj->hhitbox = hi;
	obj->rhitbox = 1;
	obj->thitbox = RectHitBox;

	//Estructura de instancia y funcion asociada al objeto
	obj->ins = ins;
	obj->taskObj = taskobj;

	//Vacia las variables internas
	int8_t i = 0;
	while (i < MAX_VAR)
	{
		obj->var[i] = 0;
		i++;
	}

	//Enlista el objeto actual
	Object_Add(obj);

	//Vacia las instancias
	Sprite_Type_Instance* pins = ins;
	i = max-1;
	while (i>=0)
	{
		Instance_Clean(ID,i);
		i--;
	}

	return TRUE;
}


//Configura el hitbox de un objeto con un x,y respecto a la imagen del sprite y un ancho/alto de hitbox.
BOOL Object_SetHitbox(uint8_t par, uint16_t xhb, uint16_t yhb, uint16_t whb, uint16_t hhb)
{
	//Puntero del objeto padre
	Sprite_Type_Object* obj = Object_Get(par);

	//Revisa que el hitbox no se escape del ancho/alto del sprite y que sea mayor a cero
	if ((whb==0) || (hhb==0) || (whb>obj->width) || (hhb>obj->height)) {return FALSE;}

	//Configura el hitbox rectangular
	obj->xhitbox = xhb;
	obj->yhitbox = yhb;
	obj->whitbox = whb;
	obj->hhitbox = hhb;

	return TRUE;
}


/* Descripcion: Agrega objetos a la lista de puntero de objetos.
 * *Es llamada por Object_Create.
 */
void Object_Add(Sprite_Type_Object *obj)
{
	Sprite_Type_Object *pNext;

	if (Object_List==NULL) {Object_List = obj;}
	else {
		pNext = Object_List;
		while (pNext->next!=NULL) {pNext = (Sprite_Type_Object *)pNext->next;}
		pNext->next = (void *)obj;
	}
	obj->next = NULL;
}


//Busca un objeto por su id y retorna su puntero
Sprite_Type_Object* Object_Get(uint8_t par)
{
	Sprite_Type_Object* obj = Object_List;
	while (obj!=NULL)
	{
		if (obj->id==par) {return obj;}
		obj = (Sprite_Type_Object *)obj->next;
	}

	return NULL;
}


/* Descripcion: Funcion que crea una instancia configurandola.
 * -par: id del objeto padre.
 * -lay: layer de la instancia
 * -xsub,ysub: subimagen inicial del sprite de la instancia.
 * -xi,yi: posicion inicial de la instancia.
 * -visible: visibilidad inicial de la instancia.
 * Retorno: posicion en la matriz del objeto padre.
 */
int8_t Instance_Create(uint8_t par, int8_t lay, uint8_t xsub, uint8_t ysub, uint16_t xi, uint16_t yi, BOOL visible)
{
	//Objeto asociado
	Sprite_Type_Object* obj = Object_Get(par);

	//Falla si no queda espacio para la instancia
	if (obj->nins >= obj->maxins) {return -1;}
	if (Count_Instance >= MAX_INS) {return -1;}

	//Busca un espacio para la instancia
	Sprite_Type_Instance* ins = (Sprite_Type_Instance*) obj->ins;
	Sprite_Type_Instance* pins = (Sprite_Type_Instance*) obj->ins;
	int8_t ii = -1;
	uint8_t i = 0;
	while (pins < ins + obj->maxins && ii==-1)
	{
		if (pins->pos == -1) {ii = i;}
		pins++;
		i++;
	}
	if (ii==-1) {return -1;}
	else {ins += ii;}

	//Configura las propiedades de la instancia
	ins->pos = ii;
	ins->parent = par;
	ins->layer = lay;

	ins->xsubimg = xsub;
	ins->ysubimg = ysub;
	ins->asubimg = 0;
	ins->bsubimg = 0;
	ins->tsubimg = 0;
	ins->fsubimg = 0;

	ins->x = xi;
	ins->y = yi;
	ins->xstart = xi;
	ins->ystart = yi;
	ins->xprev = xi;
	ins->yprev = yi;
	ins->vx = 0;
	ins->vy = 0;

	ins->cte = FALSE;
	ins->vis = visible;
	ins->step = TRUE;
	ins->anim = FALSE;

	ins->taskIns = NULL;

	//Configura las variables internas
	i = 0;
	while (i < MAX_VAR)
	{
		ins->var[i] = obj->var[i];
		i++;
	}

	//Cuenta la instancia añadida
	obj->nins++;
	Count_Instance++;

	return ii;
}


/* Descripcion: Configura la animacion de un sprite.
 * -par: ID del objeto padre.
 * -ins: posicion de la instancia en la matriz del objeto padre.
 * -ra,rb: rango inferior y superior de subimagenes para la animacion.
 * -ry: columna para el rango de animacion.
 * -ti: tasa de animacion.
 * Retorno: TRUE si la animacion fue un exito.
 */
BOOL Instance_SetAnim(uint8_t par, int8_t pos, uint8_t ra, uint8_t rb, uint8_t ry, uint8_t ti)
{
	//Puntero de la instancia y objeto
	Sprite_Type_Object* obj = Object_Get(par);
	Sprite_Type_Instance* ins = (Sprite_Type_Instance*) obj->ins;

	//Revisa si la instancia existe
	ins += pos;
	if (ins->pos ==-1) {return FALSE;}

	//Evita tener un rango mayor al permitido
	if ((ra >= obj->xsubimg) || (rb >= obj->xsubimg)) {return FALSE;}
	if (ry >= obj->ysubimg) {return FALSE;}

	//Configura la animacion
	ins->anim = TRUE;
	ins->asubimg = ra;
	ins->bsubimg = rb;
	ins->ysubimg = ry;
	ins->tsubimg = ti;
	ins->fsubimg = 0;

	//Primera subimagen
	if (ti>0) {ins->xsubimg = ra;}
	if (ti<0) {ins->xsubimg = rb;}

	return TRUE;
}


/* Descripcion: Limpia el arreglo de instancia de un objeto.
 * -par: id del objeto.
 * -pos: posicion en el arreglo de instancias.
 */
BOOL Instance_Clean(uint8_t par, int8_t pos)
{
	//Puntero del objeto y la instancia
	Sprite_Type_Object* obj = Object_Get(par);
	Sprite_Type_Instance* ins = obj->ins;
	ins += pos;

	//Revisa que no se escape del rango
	if (pos>obj->maxins) {return FALSE;}

    //Limpia la instancia
    ins->pos = -1;
    ins->parent = 0;
    ins->layer = 0;

    ins->xsubimg = 0;
    ins->ysubimg = 0;
    ins->asubimg = 0;
    ins->bsubimg = 0;
    ins->tsubimg = 0;
    ins->fsubimg = 0;

    ins->x = 0;
    ins->y = 0;
    ins->xstart = 0;
    ins->ystart = 0;
    ins->xprev = 0;
    ins->yprev = 0;
    ins->vx = 0;
    ins->vy = 0;

    ins->cte = FALSE;
    ins->vis = FALSE;
    ins->step = FALSE;
    ins->anim = FALSE;

    ins->taskIns = NULL;

	uint8_t i = 0;
	while (i < MAX_VAR)
	{
		ins->var[i] = 0;
		i++;
	}

    return TRUE;
}


/* Descripcion: Libera una instancia vaciando su configuracion.
 * -par: id del objeto.
 * -pos: posicion en el arreglo de instancias.
 */
BOOL Instance_Free(uint8_t par, int8_t pos)
{
	//Puntero del objeto y la instancia
	Sprite_Type_Object* obj = Object_Get(par);
	Sprite_Type_Instance* ins = obj->ins;

	//Revisa que exista y no se salga del rango
	if (pos==-1) {return FALSE;}
	if (pos>obj->maxins) {return FALSE;}

	//Instancia
	ins += pos;

	//Descuenta la instancia
	Count_Instance--;
	obj->nins--;

    //Vacia la instancia eliminada
    ins->pos = -1;
    ins->parent = 0;
    ins->layer = 0;

    ins->xsubimg = 0;
    ins->ysubimg = 0;
    ins->asubimg = 0;
    ins->bsubimg = 0;
    ins->tsubimg = 0;
    ins->fsubimg = 0;

    ins->x = 0;
    ins->y = 0;
    ins->xstart = 0;
    ins->ystart = 0;
    ins->xprev = 0;
    ins->yprev = 0;
    ins->vx = 0;
    ins->vy = 0;

    ins->cte = FALSE;
    ins->vis = FALSE;
    ins->step = FALSE;
    ins->anim = FALSE;

    ins->taskIns = NULL;

	uint8_t i = 0;
	while (i < MAX_VAR)
	{
		ins->var[i] = 0;
		i++;
	}

    return TRUE;
}


/* Descripcion: Crea o sobreescribe un Tileset.
 * -layer: Capa para dibujar Tileset, mas baja primero.
 * -nroom: Numero de habitacion donde se dibujara el Tileset.
 * -*tile Puntero del Tileset.
 * -*imagen: Imagen del Tileset.
 * -wgrid,hgrid: Ancho/Alto de las casillas de Tiles.
 * -xi,yi: Posicion inicial de la capa de Tiles.
 * -width,height: Ancho/Alto para la capa de Tilesest.
 * Retorno: Nada.
 */
void Tileset_Create(uint8_t layer, uint8_t nroom, Sprite_Type_Tileset *tile, void *imagen, uint16_t wgrid, uint16_t hgrid, uint16_t xi, uint16_t yi, uint16_t width, uint16_t height)
{
	//Configura las propiedades del Sprite
	tile->layer = layer;
	tile->room = nroom;
	tile->image = imagen;
	tile->wgrid = wgrid;
	tile->hgrid = hgrid;
	tile->xi = xi;
	tile->yi = yi;
	tile->width = width;
	tile->height = height;
	tile->visible = FALSE;

	// Puntero a la informacion del tamaño
	FLASH_WORD *imagePtr;
	imagePtr = (FLASH_WORD *)((IMAGE_FLASH *)imagen)->address + 1;

	// Lee el tamaño de la imagen
	uint16_t sizeY, sizeX;
	sizeY = *imagePtr;
	imagePtr++;
	sizeX = *imagePtr;

	//Numero de casillas para Tileset y para Room
	tile->xtiles = sizeX / wgrid;
	tile->ytiles = sizeY / hgrid;
	tile->xroom = tile->width / wgrid;
	tile->yroom = tile->height / hgrid;

	//Buffer de datos
	tile->maxdata = tile->xroom * tile->yroom;
	tile->data = (uint8_t *) malloc(tile->maxdata);

	//Vacia los valores del Mapa de datos del Tileset
	memset(tile->data,0,tile->maxdata);

	//Enlista el objeto actual
	Tileset_Add(tile);
}


/* Descripcion: Configura un rango de valores para cierta capa de Tileset.
 * -layer: Capa del Tileset.
 * -data: Valor para las posiciones.
 * -xt,yt: Posicion inicial (casillas) para el ingreso de datos.
 * -size: Tamaño del ingreso de datos.
 * Retorno: Nada.
 */
BOOL Tileset_SetMap(uint8_t layer, uint8_t data, uint16_t xt, uint16_t yt, uint16_t size)
	{

	//Obtiene el puntero del Tileset
	Sprite_Type_Tileset* tile = Tileset_Get(layer);

	//Revisa que no se exceda ningun maximo
	if (xt >= tile->xroom) {return FALSE;}
	if (yt >= tile->yroom) {return FALSE;}

	uint16_t iPos = (yt * tile->xroom) + xt;
	uint16_t fPos = iPos + size;
	if (iPos > tile->maxdata || fPos > tile->maxdata) {return FALSE;}

	//Rellena con los valores dados la region del Mapa de datos del Tileset
	uint16_t i = iPos;
	while (i < fPos)
		{
		*(__IO uint8_t*) (tile->data + i) = data;
		i++;
	}

	return TRUE;
}


/* Descripcion: Agrega un Tileset a la lista de puntero de Tileset.
 * *Es llamada por Tileset_Create.
 */
void Tileset_Add(Sprite_Type_Tileset *tile)
	{
	Sprite_Type_Tileset *pNext;

	if (Tileset_List==NULL)
		{
		Tileset_List = tile;
	}
	else {
		pNext = Tileset_List;
		while (pNext->next!=NULL)
			{
			pNext = (Sprite_Type_Tileset *)pNext->next;
		}
		pNext->next = (void *)tile;
	}
	tile->next = NULL;
}


//Libera un Tileset para reconfigurarlo, retornando true si lo libero correctamente
BOOL Tileset_Free(Sprite_Type_Tileset *tile)
	{

	if (Tileset_List==NULL) {return FALSE;}

	//Arregla la lista de Tileset, eliminandolo de la lista
	if(tile == Tileset_List) {Tileset_List = (Sprite_Type_Tileset *)tile->next;}
	else {
		Sprite_Type_Tileset *curr;
		Sprite_Type_Tileset *prev;

		curr = (Sprite_Type_Tileset*)Tileset_List->next;
		prev = (Sprite_Type_Tileset*)Tileset_List;

		//Lo busca si no es NULL
		while(curr)
			{
			if(curr == tile) {break;}

			prev = (Sprite_Type_Tileset*)curr;
			curr = (Sprite_Type_Tileset*)curr->next;
		}

		//Cancela si es NULL
		if(!curr) {return FALSE;}

		prev->next = curr->next;
	}

	//Vacia el Tile
	tile->layer = 0;
	tile->room = 0;
	tile->image = NULL;
	tile->wgrid = 0;
	tile->hgrid = 0;
	tile->xi = 0;
	tile->yi = 0;
	tile->width = 0;
	tile->height = 0;
	tile->xtiles = 0;
	tile->ytiles = 0;
	tile->xroom = 0;
	tile->yroom = 0;
	tile->maxdata = 0;
	tile->visible = FALSE;

	//Libera la memoria del buffer del Mapa de Tiles
	free(tile->data);
	tile->data = NULL;

	return TRUE;
}


//Busca un tileset por su id y retorna su puntero
Sprite_Type_Tileset* Tileset_Get(uint8_t layer)
	{
	Sprite_Type_Tileset* tile = Tileset_List;

	while (tile!=NULL)
		{
		if (tile->layer==layer) {return tile;}
		tile = (Sprite_Type_Tileset *)tile->next;
	}

	return NULL;
}


//Cambia de Room
BOOL Room_Goto(uint8_t room)
{
	//Revisa que la habitacion exista
	if (room>=MAX_ROOM) {return FALSE;}

	//Revisa que no este en la misma habitacion
	if (Room_Active==room) {return FALSE;}

	//Limpia las instancias de la room
	Sprite_Type_Object* obj = Object_List;
	Sprite_Type_Instance* ins;
	int8_t max;

	while (obj!=NULL)
	{
		ins = (Sprite_Type_Instance*)obj->ins;
		max = obj->maxins -1;

		//Borra la instancia
		while (max>=0)
		{
			if (((ins+max)->pos!=-1) && ((ins+max)->cte==FALSE)) {Instance_Free(obj->id,max);}
			max--;
		}

		//Siguiente objeto
		obj = (Sprite_Type_Object*) obj->next;
	}

	//Cambia de Room
	Room_Active = room;

	return TRUE;
}


/* Descripcion: Lo que se realiza durante cada Step del Sprite Engine.
 * NOTA:Debe ser incluido en el loop del programa.
 */
void Step_Sprites(void)
{
	//Obtiene el Systick inicial
	tickfirst = HAL_GetTick();

	//Limpia el Buffer Render con el color de fondo
	if (RefreshBuffer==TRUE) {DMA2D_ClearBuffer((uint32_t *)RENDER_BUFFER);}

	//Corrige la posicion de las pantallas
	uint8_t iscreen = 0;
	while (iscreen<MAX_SCREEN)
	{
		if (Screen[iscreen].Activated)
		{
			Screen[iscreen].xPrev = Screen[iscreen].xCur;
			Screen[iscreen].xCur = Screen[iscreen].xNext;

			Screen[iscreen].yPrev = Screen[iscreen].yCur;
			Screen[iscreen].yCur = Screen[iscreen].yNext;
		}
		iscreen++;
	}

	//Revisa los Tileset enlistados para que se procesen en la Room actual
	Sprite_Type_Tileset* tile = Tileset_List;
	while (tile!=NULL)
	{
		if (tile->room == Room_Active)
		{
			uint16_t i = 0;
			while (i<tile->maxdata)
			{
				uint8_t data = *(tile->data + i);
				if (data!=0) {Draw_Tile(tile,i);}
				i++;
			}
		}

		//Pasa al siguiente Tileset
		tile = (Sprite_Type_Tileset *)tile->next;
	}

	//Revisa los objetos en la lista para que se procesen
	Sprite_Type_Object* obj = Object_List;
	Sprite_Type_Instance* ins;
	int8_t max;
	while (obj!=NULL)
	{
		ins = (Sprite_Type_Instance*)obj->ins;
		max = obj->maxins -1;

		//Procesa las instancias
		while (max>=0)
		{
			if ((ins+max)->step)
			{
				Task_Sprite(obj->id,max);
				if ((ins+max)->vis) {Anim_Sprite(obj->id,max);}
			}
			max--;
		}

		//Siguiente objeto
		obj = (Sprite_Type_Object*) obj->next;
	}

	//Copia el Render Buffer al Frame Buffer (Pantalla)
	DMA2D_CopyBuffer((uint32_t *)RENDER_BUFFER, (uint32_t *)FRAME_BUFFER);

	//Tiempo Muerto y FPS
	ticklast = HAL_GetTick();
	ticktimer = tickfrec - (ticklast - tickfirst);
	if (ticktimer<0) {ticktimer = 0;}
	if (ticktimer>0) {osDelay(ticktimer);}
	framerate = 1000 / (ticktimer + ticklast - tickfirst);
}


/* Descripcion: Tareas que realiza un Sprite al ser procesado.
 * *Es llamada por Step_Sprites().
 */
void Task_Sprite(uint8_t par, int8_t pos)
{
	Sprite_Type_Object* obj = Object_Get(par);
	Sprite_Type_Instance* ins = obj->ins;
	ins += pos;

	//Movimiento (nativo) de la instancia
	ins->xprev = ins->x;
	ins->yprev = ins->y;
	ins->x += ins->vx;
	ins->y += ins->vy;

	//Tareas a realizar como objeto
	if (obj->taskObj!=NULL) {obj->taskObj(par,pos);}

	//Tareas a realizar como instancia
	if (ins->taskIns!=NULL) {ins->taskIns(par,pos);}
}


/* Descripcion: actualiza la animacion de una instancia, para luego dibujarla.
 * *Es llamada por Step_Sprite().
 */
void Anim_Sprite(uint8_t par, int8_t pos)
{
	Sprite_Type_Object* obj = Object_Get(par);
	Sprite_Type_Instance* ins = obj->ins;
	ins += pos;

	if (ins->anim)
	{
		//Si el sprite esta animado, avanza el fotograma
		if (ins->tsubimg>0)
		{
			if (ins->fsubimg < ins->tsubimg) {ins->fsubimg++;}
			else {ins->fsubimg = 0;}
		}
		else if (ins->tsubimg<0)
		{
			if (ins->fsubimg < -(ins->tsubimg)) {ins->fsubimg++;}
			else {ins->fsubimg = 0;}
		}


		//Cambia la subimagen si corresponde
		if (ins->fsubimg == ins->tsubimg)
		{
			if (ins->tsubimg>0)
			{
				if (ins->xsubimg < ins->bsubimg) {ins->xsubimg++;}
				else {ins->xsubimg = ins->asubimg;}
			}
			else if (ins->tsubimg<0)
			{
				if (ins->xsubimg > ins->asubimg) {ins->xsubimg--;}
				else {ins->xsubimg = ins->bsubimg;}
			}
		}
	}

	//Dibuja el sprite
	Draw_Sprite(par,pos,ins->x,ins->y);
}


/* Descripcion: Dibuja el sprite de una instancia.
 * -obj: puntero del objeto padre.
 * -ins: posicion de la instancia en la matriz del objeto padre.
 * -xi,yi: posicion para dibujar.
 * Retorno: No.
 * *Es llamada por Anim_Sprite().
 */
void Draw_Sprite(uint8_t par, int8_t pos, uint16_t xi, uint16_t yi)
{
	Sprite_Type_Object* obj = Object_Get(par);
	Sprite_Type_Instance* ins = obj->ins;
	ins += pos;

	//Dibuja sobre las screens solo si estan activadas
	uint8_t iscreen = 0;
	while (iscreen<MAX_SCREEN)
	{
		if (Screen[iscreen].Activated)
		{
			//Toma las coordenadas del sprite en la Room convirtiendolas en las coordenadas de la Screen
			int16_t a = xi - Screen[iscreen].xCur;
			int16_t b = yi - Screen[iscreen].yCur;
			int16_t c = a + obj->width;
			int16_t d = b + obj->height;
			int16_t ofx = ins->xsubimg * obj->width;
			int16_t ofy = ins->ysubimg * obj->height;

			//Rectangulo de la Screen
			uint16_t o = 0;
			uint16_t p = 0;
			uint16_t q = Screen[iscreen].width;
			uint16_t r = Screen[iscreen].height;

			//Verifica que el sprite este dentro del rectangulo de la Screen
			BOOL coll = FALSE;
			if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b <= p && b <= r) && (d >= p && d >= r))) {coll = TRUE;}
			else if (((c >= o && c >= q) && (a <= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {coll = TRUE;}
			else if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {coll = TRUE;}

			//Arregla la imagen para dibujarla en la pantalla si esta dentro de la Screen
			if (coll)
			{
				uint16_t s = 0 + Screen[iscreen].xScreen;
				uint16_t t = 0 + Screen[iscreen].yScreen;
				uint16_t u = q + Screen[iscreen].xScreen;
				uint16_t v = r + Screen[iscreen].yScreen;

				a += Screen[iscreen].xScreen;
				b += Screen[iscreen].yScreen;
				c += Screen[iscreen].xScreen;
				d += Screen[iscreen].yScreen;

				if (a<s) {ofx -= (a - Screen[iscreen].xScreen); a = s;}
				if (b<t) {ofy -= (b - Screen[iscreen].yScreen); b = t;}
				if (c>u) {c = u;}
				if (d>v) {d = v;}

				//Cambia coordenada x2,y2 por ancho y alto arreglado
				c -= a;
				d -= b;

				//Dibuja la porcion del sprite dentro de la screen
				if (c>1 && d>1) {Draw_ImagePartial(a,b,obj->image,ofx,ofy,c,d);}
			}
		}

		//Siguiente
		iscreen++;
	}
}


/* Descripcion: Dibuja un Tile.
 * -tile: Puntero del Tileset.
 * -dpos: Posicion del dato en el buffer del Tileset.
 * Retorno: Nada.
 * *Es llamada por Step_Sprites().
 */
void Draw_Tile(Sprite_Type_Tileset* tile, uint8_t dpos)
	{
	uint8_t data = *(tile->data + dpos) - 1;
	uint16_t xt = data;
	uint16_t yt = 0;
	uint16_t xr = dpos;
	uint16_t yr = 0;

	//Obtiene la posicion xt,yt (tile del tileset) desde dpos
	while (xt >= tile->xtiles)
		{
		xt -= tile->xtiles;
		yt++;
	}

	//Obtiene la posicion xr,yr (tile de la room) desde dpos
	while (xr >= tile->xroom)
		{
		xr -= tile->xroom;
		yr++;
	}

	uint8_t iscreen = 0;
	//Dibuja sobre las screens solo si estan activadas y el layer es visible
	while (iscreen<MAX_SCREEN)
		{
		if (Screen[iscreen].Activated && tile->visible)
			{
			//Toma las coordenadas del Tile en la Room convirtiendolas en las coordenadas de la Screen
			int16_t a = tile->xi - Screen[iscreen].xCur + (xr*tile->wgrid);
			int16_t b = tile->yi - Screen[iscreen].yCur + (yr*tile->hgrid);
			int16_t c = a + tile->wgrid;
			int16_t d = b + tile->hgrid;
			int16_t ofx = xt * tile->wgrid;
			int16_t ofy = yt * tile->hgrid;

			//Rectangulo de la Screen
			uint16_t o = 0;
			uint16_t p = 0;
			uint16_t q = Screen[iscreen].width;
			uint16_t r = Screen[iscreen].height;

			//Verifica que el Tile este dentro del rectangulo de la Screen
			BOOL coll = FALSE;
			if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b <= p && b <= r) && (d >= p && d >= r))) {coll = TRUE;}
			else if (((c >= o && c >= q) && (a <= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {coll = TRUE;}
			else if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {coll = TRUE;}

			//Arregla la imagen para dibujarla en la pantalla si esta dentro de la Screen
			if (coll)
				{
				uint16_t s = 0 + Screen[iscreen].xScreen;
				uint16_t t = 0 + Screen[iscreen].yScreen;
				uint16_t u = q + Screen[iscreen].xScreen;
				uint16_t v = r + Screen[iscreen].yScreen;

				a += Screen[iscreen].xScreen;
				b += Screen[iscreen].yScreen;
				c += Screen[iscreen].xScreen;
				d += Screen[iscreen].yScreen;

				if (a<s) {ofx -= (a - Screen[iscreen].xScreen); a = s;}
				if (b<t) {ofy -= (b - Screen[iscreen].yScreen); b = t;}
				if (c>u) {c = u;}
				if (d>v) {d = v;}

				//Cambia coordenada x2,y2 por ancho y alto arreglado
				c -= a;
				d -= b;

				//Dibuja la porcion de tile en la screen
				if (c>1 && d>1) {Draw_ImagePartial(a,b,tile->image,ofx,ofy,c,d);}
			}
		}
		iscreen++;
	}
}


//Dibuja una imagen BMP con cierto ancho/alto y cierto x/y offset en x,y con formato ARGB-8888-32bpp.
void Draw_ImagePartial(uint16_t left, uint16_t top, Sprite_Image *image, uint16_t xoffset, uint16_t yoffset, uint16_t width, uint16_t height)
{

	uint32_t 			*imagePtr;
    uint16_t            sizeX, sizeY;
    uint16_t			x, y, xc, yc;
    uint16_t            addressOffset = 0;
    uint32_t 			Xaddress;

    //Mueve el puntero a la direccion de la imagen
    imagePtr = (uint32_t *)((Sprite_Image *)image)->data;

    //Tamaño de la imagen
    sizeY = image->height;
    sizeX = image->width;

    //Offset y dimensiones
    if(width != 0)
    {
        imagePtr += xoffset + yoffset*(sizeX);
        addressOffset = sizeX - width;
        Xaddress = (uint32_t)RENDER_BUFFER + 4*(BSP_LCD_GetXSize()*top + left);
    }

	//Pasa la imagen al Buffer Render linea por linea
	for (y = 0; y < height; y++)
	{
		DMA2D_DrawImage((uint32_t *) imagePtr, (uint32_t *) Xaddress, width);
		Xaddress += BSP_LCD_GetXSize()*4;
		imagePtr += sizeX;
	}
}


//Funcion que retorna un numero entero al azar entre i1 y i2 (-32 767 ~ 32 768).
int16_t Random_intGet(int16_t i1 , int16_t i2)
{
	int16_t ret;

	//Verifica que el valor i1 sea menor a i2
	if (i1 >= i2) {return 0;}

	//Calcula el rango y obtiene un valor random
	int16_t range = i2 - i1 + 1;
	int16_t irandom = rand()%range;

	//Transforma el valor random a un valor dentro del rango (i1 ~ i2).
	if (irandom <i1) {while (irandom<i1) {irandom += range;}}
	else if (irandom >i2) {while (irandom<i2) {irandom -= range;}}

	//Retorna el valor random
	ret = irandom;
	return ret;
}


//Funcion que retorna el angulo entre dos puntos respecto al eje X con la circunferencia goniometrica
uint16_t Point_GetAngle(int16_t xi, int16_t yi, int16_t xo, int16_t yo)
{
	float angle;
	uint8_t k;
	uint16_t xd,yd;

	//Obtiene la constante k: numero de cuartos de vuelta
	if ((yo-yi)>=0 && (xo-xi)>0) {k = 0;} //Primer cuadrante
	else if ( (yo-yi)>0 && (xo-xi)<=0) {k = 1;} //Segundo cuadrante
	else if ( (yo-yi)<=0 && (xo-xi)<0) {k = 2;} //Tercer cuadrante
	else if ( (yo-yi)<0 && (xo-xi)>=0) {k = 3;} //Cuarto cuadrante cuadrante
	else if ( (yo-yi)==0 && (xo-xi)==0 ) {k = 0;}

	//Mueve el triangulo al primer cuadrante
	if (k==3)
	{
		xd = yi - yo;
		yd = xo - xi;
	}
	else if (k==2)
	{
		xd = xi - xo;
		yd = yi - yo;
	}
	else if (k==1)
	{
		xd = yo - yi;
		yd = xi - xo;
	}
	else {
		xd = xo - xi;
		yd = yo - yi;
	}

	//Obtiene el angulo
	if (xd!=0)
	{
		angle = atan2(yd,xd);
		angle = angle * 180 / 3.14159;
	}
	else {angle = 90;}

	//Corrige el cuadrante del angulo
	angle += k*90;

	//Retorna el angulo
	uint16_t ret = angle;
	return ret;
}


//Funcion que retorna la distancia entre dos puntos
uint16_t Point_GetDistance(int16_t x1,int16_t y1, int16_t x2, int16_t y2)
{
	float magn;

	magn = ((x2-x1)*(x2-x1)) + ((y2-y1)*(y2-y1));
	magn = sqrt(magn);

	return magn;
}


//Funcion que mueve cierta instancia segun una direccion y una distancia
void Move_Direction(uint8_t par, int8_t pos, uint16_t dir, int16_t dist)
{
	//Puntero del objeto e instancia
	Sprite_Type_Object* obj = Object_Get(par);
	Sprite_Type_Instance* ins = obj->ins;
	ins += pos;

	uint16_t xi = ins->x;
	uint16_t yi = ins->y;

	//Corrige la direccion
	if (dir<0) {dir += 360;}
	if (dir>=360) {dir -= 360;}

	float dir_rad = dir * 3.14159 / 180;

	int16_t xa = cos(dir_rad)*dist;
	int16_t ya = sin(dir_rad)*dist;

	ins->x = xi + xa;
	ins->y = yi - ya;
}


/* Descripcion: Revisa si una instancia colisiona una zona rectangular/cuadrada.
 * -objeto padre de la instancia hija.
 * -posiciones del rectangulo. (xa debe ser mayor a xb, igual para ya con yb)
 * -xi,yi: Adelanto en x y/o y de la instancia 1.
 * Retorno: TRUE si colisionan.
 */
BOOL Collision_RectangleInstance(uint8_t par, int8_t pos, uint16_t xa, uint16_t ya, uint16_t xb, uint16_t yb, int16_t xi, int16_t yi)
{
	//Puntero de la instancia y objeto
	Sprite_Type_Object* obj = Object_Get(par);
	Sprite_Type_Instance* ins = (Sprite_Type_Instance*) obj->ins;
	ins += pos;

	//Valores de posicion para obj
	uint16_t a = ins->x + xi;
	uint16_t b = ins->y + yi;
	uint16_t c = a + obj->width -1;
	uint16_t d = b + obj->height -1;

	//Valores de posicion para la zona rectangular
	uint16_t o = xa;
	uint16_t p = ya;
	uint16_t q = xb;
	uint16_t r = yb;

	//Retorna FALSE si el rectangulo esta mal formado
	if (o>=q || p>=r) {return FALSE;}

	//Revisa colisiones cuadradas
	if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b <= p && b <= r) && (d >= p && d >= r))) {return TRUE;} //obj1 contiene al obj2 (eje:y)
	if (((c >= o && c >= q) && (a <= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {return TRUE;} //obj1 contiene al obj2 (eje:x)
	if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {return TRUE;} //obj1 colision de esquina con obj2

	//No hubo colision
    return FALSE;
}


/* Descripcion: Revisa colisiones entre una instancia y otra.
 * -objetos 1 y 2.
 * -posicion de la instancia en la matriz padre 1 y 2.
 * -xi,yi: Adelanto en x y/o y de la instancia 1.
 * Retorno: TRUE si colisionan.
 */
BOOL Collision_SpriteInstance(uint8_t par1, int8_t pos1,uint8_t par2, int8_t pos2, int16_t xi, int16_t yi)
{
	//Puntero de las instancias y objetos
	Sprite_Type_Object* obj1 = Object_Get(par1);
	Sprite_Type_Instance* ins1 = (Sprite_Type_Instance*) obj1->ins;
	ins1 += pos1;
	Sprite_Type_Object* obj2 = Object_Get(par2);
	Sprite_Type_Instance* ins2 = (Sprite_Type_Instance*) obj2->ins;
	ins2 += pos2;

	//Valores de posicion para obj1
	uint16_t a = ins1->x + xi;
	uint16_t b = ins1->y + yi;
	uint16_t c = a + obj1->width -1;
	uint16_t d = b + obj1->height -1;

	//Valores de posicion para obj2
	uint16_t o = ins2->x;
	uint16_t p = ins2->y;
	uint16_t q = o + obj2->width -1;
	uint16_t r = p + obj2->height -1;

	//Revisa colisiones cuadradas
	if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b <= p && b <= r) && (d >= p && d >= r))) {return TRUE;} //obj1 contiene al obj2 (eje:y)
	if (((c >= o && c >= q) && (a <= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {return TRUE;} //obj1 contiene al obj2 (eje:x)
	if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {return TRUE;} //obj1 colision de esquina con obj2

	//No hubo colision
    return FALSE;
}


/* Descripcion: Revisa colisiones respecto al hitbox de dos instancias.
 * -objetos 1 y 2.
 * -posicion de la instancia en la matriz padre 1 y 2.
 * -xi,yi: Adelanto en x y/o y de la instancia 1.
 * Retorno: TRUE si colisionan.
 */
BOOL Collision_HitboxInstance(uint8_t par1, int8_t pos1,uint8_t par2, int8_t pos2, int16_t xi, int16_t yi)
{
	//Puntero de las instancias y objetos
	Sprite_Type_Object* obj1 = Object_Get(par1);
	Sprite_Type_Instance* ins1 = (Sprite_Type_Instance*) obj1->ins;
	ins1 += pos1;
	Sprite_Type_Object* obj2 = Object_Get(par2);
	Sprite_Type_Instance* ins2 = (Sprite_Type_Instance*) obj2->ins;
	ins2 += pos2;

	//Valores de posicion para spr1
	uint16_t a = ins1->x + obj1->xhitbox + xi;
	uint16_t b = ins1->y + obj1->yhitbox + yi;
	uint16_t c = a + obj1->whitbox -1;
	uint16_t d = b + obj1->hhitbox -1;

	//Valores de posicion para spr2
	uint16_t o = ins2->x + obj2->xhitbox;
	uint16_t p = ins2->y + obj2->yhitbox;
	uint16_t q = o + obj2->whitbox -1;
	uint16_t r = p + obj2->hhitbox -1;

	//Revisa colisiones cuadradas
	if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b <= p && b <= r) && (d >= p && d >= r))) {return TRUE;} //obj1 contiene al obj2 (eje:y)
	if (((c >= o && c >= q) && (a <= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {return TRUE;} //obj1 contiene al obj2 (eje:x)
	if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) {return TRUE;} //obj1 colision de esquina con obj2

	return FALSE;
}
