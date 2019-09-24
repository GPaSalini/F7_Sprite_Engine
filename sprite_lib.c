/*
 * sprite_lib.c
 *
 *  Created on: 21-12-2017
 *      Author: Giordano
 */

#include "sprite_lib.h"

//========== Variables Globales ==========

uint32_t C_Background;
uint32_t C_Text;
BOOL RefreshBuffer;
BOOL UpdateBuffer = FALSE;
uint16_t maxStep;
sFONT *Sprite_Font = NULL;

Sprite_Type_Screen Screen[MAX_SCREEN];
Sprite_Type_Object *Object_List = NULL;
Sprite_List_Instance *Instance_List = NULL;
uint16_t Count_Instance = 0;

uint32_t tickfirst;
uint32_t ticklast;
uint16_t ticktime;
int16_t ticktimer;
int16_t framerate;
uint16_t freebytes;
char debug[10] = {"\0"};

RNG_HandleTypeDef rng_handle;
RNG_TypeDef rng_instance;

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

	state = HAL_DMA2D_Init(&hDma2dHandler);
	if (state == HAL_OK)
	{
		state = HAL_DMA2D_Start(&hDma2dHandler,C_Background,(uint32_t)pDst,BSP_LCD_GetXSize(),BSP_LCD_GetYSize());
		state = HAL_DMA2D_PollForTransfer(&hDma2dHandler,100);
	}
}


//Copia el buffer render al buffer de pantalla mediante DMA2D en modo M2M con PFC
void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst)
{
	HAL_StatusTypeDef state;
	DMA2D_HandleTypeDef hDma2dHandler;

	hDma2dHandler.Init.Mode         = DMA2D_M2M_PFC;
	hDma2dHandler.Init.ColorMode    = DMA2D_OUTPUT_ARGB4444;
	hDma2dHandler.Init.OutputOffset = 0x0;
	hDma2dHandler.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
	hDma2dHandler.Init.RedBlueSwap   = DMA2D_RB_REGULAR;

	hDma2dHandler.Instance = DMA2D;

	state = HAL_DMA2D_Init(&hDma2dHandler);
	if (state == HAL_OK)
	{
		state = HAL_DMA2D_Start(&hDma2dHandler,(uint32_t)pSrc,(uint32_t)pDst,BSP_LCD_GetXSize(),BSP_LCD_GetYSize());
		state = HAL_DMA2D_PollForTransfer(&hDma2dHandler,100);
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

	state = HAL_DMA2D_Init(&hDma2dHandler);
	if (state == HAL_OK)
	{
		state = HAL_DMA2D_BlendingStart(&hDma2dHandler,(uint32_t)pImg,(uint32_t)pDst,(uint32_t)pDst,width,1);
		state = HAL_DMA2D_PollForTransfer(&hDma2dHandler,100);
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

	state = HAL_DMA2D_Init(&hDma2dHandler);
	if (state == HAL_OK)
	{
		uint32_t Xaddress = (uint32_t)RENDER_BUFFER + 4*(BSP_LCD_GetXSize()*y1 + x1);
		state = HAL_DMA2D_Start(&hDma2dHandler,col,(uint32_t)Xaddress,w,h);
		state = HAL_DMA2D_PollForTransfer(&hDma2dHandler,100);
	}
}


//Configura los step que ocurren hasta 1 segundo (fps)
void Step_Config(uint16_t step)
{
	maxStep = step;
	ticktime = 1000/maxStep;
}

//Configura refresco del buffer render
void Refresh_Config(BOOL refresh)
{
	RefreshBuffer = refresh;
}

//Configura el color de fondo
void Background_Config(uint32_t color)
{
	C_Background = color;
	BSP_LCD_SetBackColor(C_Background);
}

//Configura una fuente y color de texto
void Text_Config(sFONT* font, uint32_t color)
{
	Sprite_Font = font;
	C_Text = color;
}


/* Descripcion: Funcion que crea una pantalla dentro de la habitacion actual.
 * -ID: id unica de la pantalla.
 * -xscreen,yscreen: coordenadas de la pantalla.
 * -wscreen,hscreen: ancho y alto de la pantalla.
 * -xi,yi: posicion de la pantalla en la room.
 * Retorno: Nada.
 * NOTA: La pantalla viene desactivada por defecto.
 */
void Screen_Create(uint8_t ID, uint16_t xscreen, uint16_t yscreen, uint16_t wscreen, uint16_t hscreen, int16_t xi, int16_t yi)
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
 * -*taskobj: Puntero del Task.
 * -*imagen: imagen del objeto.
 * -wi,hi: ancho y alto del sprite.
 * -xsub,ysub: numero de subimagenes por columna y columna de subimagenes.
 * Retorno: TRUE/FALSE.
 */
BOOL Object_Create(uint8_t ID, Sprite_Type_Object *obj, void (*taskobj)(Sprite_Type_Instance* ins), Sprite_Image *imagen, uint16_t wi, uint16_t hi, uint8_t xsub, uint8_t ysub)
{
	if (obj==NULL) return FALSE;

	//Configura las propiedades del Sprite
	obj->id = ID;
	obj->ins = NULL;
	obj->nins = 0;
	obj->image = imagen;
	obj->width = wi;
	obj->height = hi;
	obj->xsubimg = xsub;
	obj->ysubimg = ysub;
	obj->hitbox.type = RectHitBox;
	obj->hitbox.x = 0;
	obj->hitbox.y = 0;
	obj->hitbox.w = wi;
	obj->hitbox.h = hi;
	obj->hitbox.r = 0;

	//Funcion asociada al objeto
	obj->taskObj = taskobj;

	//Funcion de dibujo
	obj->drawObj = NULL;

	//Vacia las variables internas
	for (uint8_t i=0; i<MAX_VAR ;i++)
		obj->var[i] = 0;

	//Enlista el objeto actual
	Object_Add(obj);

	return TRUE;
}


//Configura el hitbox de un objeto con un x,y respecto a la imagen del sprite y un ancho/alto de hitbox.
BOOL Object_SetHitbox(uint8_t par, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
	//Puntero del objeto padre
	Sprite_Type_Object* obj = Object_Get(par);
	if (obj==NULL) return FALSE;

	//Revisa que el hitbox sea mayor a cero
	if (w==0 || h==0) return FALSE;

	//Configura el hitbox rectangular
	obj->hitbox.x = x;
	obj->hitbox.y = y;
	obj->hitbox.w = w;
	obj->hitbox.h = h;

	return TRUE;
}


/* Descripcion: Agrega objetos a la lista de puntero de objetos.
 * NOTA: Es llamada por Object_Create().
 */
static void Object_Add(Sprite_Type_Object *obj)
{
	Sprite_Type_Object *obj_nxt = Object_List;

	if (obj_nxt==NULL) Object_List = obj;
	else
	{
		obj_nxt = Object_List;
		while (obj_nxt->next!=NULL)
			obj_nxt = obj_nxt->next;
		obj_nxt->next = obj;
	}
	obj->next = NULL;
}


//Busca un objeto por su id y retorna su puntero
Sprite_Type_Object* Object_Get(uint8_t par)
{
	Sprite_Type_Object* obj = Object_List;
	while (obj!=NULL)
	{
		if (obj->id==par)
			return obj;
		obj = obj->next;
	}
	return NULL;
}


/* Descripcion: Funcion que crea una instancia configurandola.
 * -par: id del objeto padre.
 * -lay: layer de la instancia
 * -xsub,ysub: subimagen inicial del sprite de la instancia.
 * -xi,yi: posicion inicial de la instancia.
 * -visible: visibilidad inicial de la instancia.
 * Retorno: Puntero de la instancia creada.
 */
Sprite_Type_Instance* Instance_Create(uint8_t par, int8_t lay, uint8_t xsub, uint8_t ysub, int16_t xi, int16_t yi, BOOL visible)
{
	//Falla si no queda espacio para la instancia
	if (Count_Instance >= MAX_INS) return NULL;

	//Objeto asociado
	Sprite_Type_Object *obj = Object_Get(par);
	if (obj==NULL) return NULL;

	//Reserva de memoria para la instancia
	Sprite_Type_Instance *ins = (Sprite_Type_Instance*)pvPortMalloc(sizeof(Sprite_Type_Instance));
	if (ins==NULL) return NULL;

	//Configura las propiedades de la instancia
	ins->parent = par;
	ins->layer = lay;

	ins->ins_prev = NULL;
	ins->ins_next = NULL;

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
	ins->destroy = FALSE;
	ins->anim = FALSE;

	ins->taskIns = NULL;
	ins->drawIns = NULL;

	//Configura las variables internas
	for (uint8_t i=0; i<MAX_VAR ;i++)
		ins->var[i] = obj->var[i];

	//Añade la instancia a su objeto padre
	Sprite_Type_Instance *ins_count = obj->ins;
	if (ins_count==NULL)
		obj->ins = ins;
	else
	{
		while (ins_count->ins_next!=NULL)
			ins_count = ins_count->ins_next;
		ins_count->ins_next = ins;
		ins->ins_prev = ins_count;
	}
	obj->nins++;
	Count_Instance++;

	//Añade la instancia a la lista
	Instance_Add(ins);

	return ins;
}

/* Descripcion: Agrega la instancia a la lista de instancias.
 * NOTA: Es llamada por Instance_Create().
 */
static void Instance_Add(Sprite_Type_Instance *ins)
{
	Sprite_List_Instance *Instance_Next = NULL;
	Sprite_List_Instance *Instance_Count = Instance_List;
	Sprite_List_Instance *Instance_Prev = NULL;
	int8_t lay = ins->layer;

	Sprite_List_Instance *list = (Sprite_List_Instance*)pvPortMalloc(sizeof(Sprite_List_Instance));
	if (list==NULL) return;
	list->ins = ins;
	list->layer = ins->layer;
	list->next = NULL;
	list->prev = NULL;

	if (Instance_Count!=NULL)
	{
		//Avanza hasta el final del layer
		while (Instance_Count->layer<=lay && Instance_Count!=NULL)
		{
			Instance_Prev = Instance_Count;
			Instance_Count = Instance_Count->next;
		}

		if (Instance_Prev==NULL)
		{
			Instance_List = list;
			list->next = Instance_Count;
			Instance_Count->prev = list;
		}
		else
		{
			if (Instance_Count==NULL)
			{
				Instance_Prev->next = list;
				list->prev = Instance_Prev;
			}
			else
			{
				list->next = Instance_Count;
				Instance_Count->prev = list;
				list->prev = Instance_Prev;
				Instance_Prev->next = list;
			}
		}
	}
	else Instance_List = list;
}


/* Descripcion: Configura la animacion de un sprite.
 * -*ins: puntero de la instancia.
 * -ra,rb: rango [a,b] de subimagenes para la animacion.
 * -ry: columna para el rango de animacion.
 * -ti: tasa de animacion.
 * Retorno: TRUE/FALSE.
 */
BOOL Instance_SetAnim(Sprite_Type_Instance* ins, uint8_t ra, uint8_t rb, uint8_t ry, int8_t ti)
{
	if (ins==NULL) return FALSE;

	//Puntero de la instancia y objeto
	Sprite_Type_Object* obj = Object_Get(ins->parent);

	//Evita tener un rango mayor al permitido
	if (ra >= obj->xsubimg || rb >= obj->xsubimg || ra > rb) return FALSE;
	if (ry >= obj->ysubimg) return FALSE;

	//Configura la animacion
	ins->anim = TRUE;
	ins->asubimg = ra;
	ins->bsubimg = rb;
	ins->ysubimg = ry;
	ins->tsubimg = ti;
	ins->fsubimg = 0;

	//Primera subimagen
	if (ti>0) ins->xsubimg = ra;
	if (ti<0) ins->xsubimg = rb;

	return TRUE;
}


/* Descripcion: Destruye una instancia eliminandola tanto de su objeto padre como de la lista.
 * -*ins: puntero de la instancia.
 * Retorno: TRUE/FALSE.
 */
BOOL Instance_Destroy(Sprite_Type_Instance* ins)
{
	if (ins==NULL) return FALSE;

	//Puntero del objeto y la instancia
	Sprite_Type_Object* obj = Object_Get(ins->parent);

	//Descuenta la instancia
	Count_Instance--;
	obj->nins--;

	//Desvincula de la lista de objetos
	Sprite_Type_Instance *ins_aux = obj->ins;
	Sprite_Type_Instance *ins_prev = NULL;
	BOOL ok = FALSE;
	while (ins_aux->ins_next!=NULL && !ok)
	{
		if (ins_aux==ins) ok = TRUE;
		else
		{
			ins_prev = ins_aux;
			ins_aux = ins_aux->ins_next;
		}
	}

	//Reenlaza las instancias
	if (ins_aux==ins)
	{
		if (ins_prev==NULL)
			obj->ins = ins->ins_next;
		else
		{
			ins_prev->ins_next = ins->ins_next;
			if (ins->ins_next!=NULL)
				ins->ins_next->ins_prev = ins_prev;
		}
	}
	else return FALSE;

	//Desvincula de la lista de instancias
	if (Instance_List!=NULL)
	{
		Sprite_List_Instance *Instance_Count = Instance_List;
		Sprite_List_Instance *Instance_Prev = NULL;
		Sprite_List_Instance *Instance_Next = NULL;

		while (Instance_Count->ins!=ins && Instance_Count!=NULL)
			Instance_Count = Instance_Count->next;

		if (Instance_Count!=NULL)
		{
			if (Instance_Count->prev!=NULL)
				Instance_Prev = Instance_Count->prev;
			else Instance_Prev = NULL;
			Instance_Next = Instance_Count->next;

			if (Instance_Prev!=NULL)
				Instance_Prev->next = Instance_Next;
			else Instance_List = Instance_Next;
			Instance_Next->prev = Instance_Prev;
		}
		vPortFree(Instance_Count);
		Instance_Count = NULL;
	}

	vPortFree(ins);
	ins = NULL;

    return TRUE;
}


//Destruye todas las instancias no constantes
void Instance_Clean(void)
{
	Sprite_List_Instance *list = Instance_List;
	Sprite_Type_Instance *ins = NULL;
	while (list!=NULL)
	{
		ins = list->ins;
		if (ins->cte==FALSE)
			Instance_Destroy(ins);
		list = list->next;
	}
}


/* Descripcion: Lo que se realiza durante cada Step del Sprite Engine.
 * NOTA: Debe ser incluido en el loop del programa.
 */
void Step_Sprites(void)
{
	//Obtiene el Systick inicial
	tickfirst = HAL_GetTick();

	//Revisa las instancias en la lista para que se procesen
	Sprite_List_Instance *list = Instance_List;
	Sprite_Type_Instance *ins = NULL;
	while (list!=NULL)
	{
		ins = list->ins;
		if (ins->step)
			Task_Sprite(ins);
		list = list->next;
		if (ins->destroy)
			Instance_Destroy(ins);
	}
}

/* Descripcion: Dibuja en doble buffer cuando se indica.
 * NOTA: Debe ser incluido en el loop del programa.
 */
void Frame_Sprites(void)
{
	if (!RefreshBuffer && !UpdateBuffer) return;

	//Limpia el Buffer Render con el color de fondo
	DMA2D_ClearBuffer((uint32_t *)RENDER_BUFFER);

	//Actualiza la posicion de las pantallas
	for (uint8_t iscreen=0; iscreen<MAX_SCREEN ;iscreen++)
	{
		if (Screen[iscreen].Activated)
		{
			Screen[iscreen].xPrev = Screen[iscreen].xCur;
			Screen[iscreen].xCur = Screen[iscreen].xNext;

			Screen[iscreen].yPrev = Screen[iscreen].yCur;
			Screen[iscreen].yCur = Screen[iscreen].yNext;
		}
	}

	//Revisa las instancias en la lista para dibujarlos
	Sprite_List_Instance *list = Instance_List;
	Sprite_Type_Instance *ins = NULL;
	while (list!=NULL)
	{
		ins = list->ins;
		if (ins->vis)
			Anim_Sprite(ins);
		if (Object_Get(ins->parent)->drawObj!=NULL)
			Object_Get(ins->parent)->drawObj(ins);
		if (ins->drawIns!=NULL)
			ins->drawIns(ins);
		list = list->next;
	}

	Step_Control();

	//Doble Buffer
	DMA2D_CopyBuffer((uint32_t *)RENDER_BUFFER,(uint32_t *)FRAME_BUFFER);

	UpdateBuffer = FALSE;
}

void Step_Control(void)
{
	//Tiempo Muerto y FPS
	ticklast = HAL_GetTick();
	ticktimer = ticktime - (ticklast - tickfirst);
	if (ticktimer>0)
		osDelay(ticktimer);
	framerate = 1000 / (ticktimer + ticklast - tickfirst);

	#if(DEBUG_MEM==1)
	//Memoria disponible
	freebytes = xPortGetFreeHeapSize();
	itoa(freebytes,&debug,10);
	BSP_LCD_DisplayStringAt(704,40,&debug,LEFT_MODE);
	#endif

	#if(DEBUG_INS==1)
	//Numero de instancias
	itoa(Count_Instance,&debug,10);
	BSP_LCD_DisplayStringAt(704,64,&debug,LEFT_MODE);
	#endif

	#if(DEBUG_FPS==1)
	//Delta ticks
	itoa(ticktimer,&debug,10);
	BSP_LCD_DisplayStringAt(704,16,&debug,LEFT_MODE);
	#endif
}


/* Descripcion: Tareas que realiza un Sprite al ser procesado.
 * NOTA: Es llamada por Step_Sprites().
 */
void Task_Sprite(Sprite_Type_Instance* ins)
{
	Sprite_Type_Object* obj = Object_Get(ins->parent);

	//Movimiento (nativo) de la instancia
	ins->xprev = ins->x;
	ins->yprev = ins->y;
	ins->x += ins->vx;
	ins->y += ins->vy;

	//Tareas a realizar como objeto
	if (obj->taskObj!=NULL) obj->taskObj(ins);

	//Tareas a realizar como instancia
	if (ins->taskIns!=NULL) ins->taskIns(ins);
}


/* Descripcion: actualiza la animacion de una instancia, para luego dibujarla.
 * NOTA: Es llamada por Step_Sprite().
 */
static void Anim_Sprite(Sprite_Type_Instance* ins)
{
	if (ins->anim)
	{
		//Si el sprite esta animado, avanza el fotograma
		if (ins->tsubimg>0)
		{
			if (ins->fsubimg < ins->tsubimg) ins->fsubimg++;
			else ins->fsubimg = 0;
		}
		else if (ins->tsubimg<0)
		{
			if (ins->fsubimg < -(ins->tsubimg)) ins->fsubimg++;
			else ins->fsubimg = 0;
		}


		//Cambia la subimagen si corresponde
		if (ins->fsubimg == ins->tsubimg)
		{
			if (ins->tsubimg>0)
			{
				if (ins->xsubimg < ins->bsubimg) ins->xsubimg++;
				else ins->xsubimg = ins->asubimg;
			}
			else if (ins->tsubimg<0)
			{
				if (ins->xsubimg > ins->asubimg) ins->xsubimg--;
				else ins->xsubimg = ins->bsubimg;
			}
		}
	}

	//Dibuja el sprite
	Draw_Sprite(ins,ins->x,ins->y);
}


/* Descripcion: Prepara el dibujo del sprite en la pantalla fisica.
 * -*ins: puntero de la instancia.
 * -xi,yi: posicion para dibujar.
 * NOTA: Es llamada por Anim_Sprite().
 */
static void Draw_Sprite(Sprite_Type_Instance *ins, int16_t xi, int16_t yi)
{
	Sprite_Type_Object* obj = Object_Get(ins->parent);

	//Dibuja sobre las screens solo si estan activadas
	for (uint8_t iscreen=0; iscreen<MAX_SCREEN; iscreen++)
	{
		if (Screen[iscreen].Activated)
		{
			//Toma las coordenadas del sprite en la Room convirtiendolas en las coordenadas de la Screen
			int16_t a = xi - Screen[iscreen].xCur;
			int16_t b = yi - Screen[iscreen].yCur;
			int16_t c = a + obj->width;
			int16_t d = b + obj->height;

			//Offset del spritesheet
			int16_t ofx = ins->xsubimg * obj->width;
			int16_t ofy = ins->ysubimg * obj->height;

			//Rectangulo de la Screen
			uint16_t o = 0;
			uint16_t p = 0;
			uint16_t q = Screen[iscreen].width;
			uint16_t r = Screen[iscreen].height;

			//Verifica que el sprite este dentro del rectangulo de la Screen
			BOOL coll = FALSE;
			if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b <= p && b <= r) && (d >= p && d >= r))) coll = TRUE;
			else if (((c >= o && c >= q) && (a <= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) coll = TRUE;
			else if (((c >= o && c <= q) || (a >= o && a <= q)) && ((b >= p && b <= r) || (d >= p && d <= r))) coll = TRUE;

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

				if (a<s)
				{
					ofx -= (a - Screen[iscreen].xScreen);
					a = s;
				}
				if (b<t)
				{
					ofy -= (b - Screen[iscreen].yScreen);
					b = t;
				}
				if (c>u) c = u;
				if (d>v) d = v;

				//Cambia coordenada x2,y2 por ancho y alto arreglado
				c -= a;
				d -= b;

				//Dibuja la porcion del sprite dentro de la screen
				if (c>0 && d>0) Draw_ImagePartial(a,b,obj->image,ofx,ofy,c,d);
			}
		}
	}
}


//Dibuja una imagen BMP con cierto ancho/alto y cierto x/y offset en x,y con formato ARGB-8888-32bpp.
static void Draw_ImagePartial(uint16_t left, uint16_t top, Sprite_Image *image, uint16_t xoffset, uint16_t yoffset, uint16_t width, uint16_t height)
{

	uint32_t 	*imagePtr;
    uint16_t	sizeX, sizeY;
    uint16_t	x, y, xc, yc;
    uint32_t	addressOffset = 0;
    uint32_t 	Xaddress;

    //Mueve el puntero a la direccion de la imagen
    imagePtr = image->data;

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
	for (y=0; y<height ;y++)
	{
		DMA2D_DrawImage((uint32_t *) imagePtr, (uint32_t *) Xaddress, width);
		Xaddress += BSP_LCD_GetXSize()*4;
		imagePtr += sizeX;
	}
}

//Dibuja una string en la posicion Xpos,Ypos
void Draw_StringAt(uint16_t Xpos, uint16_t Ypos, uint8_t *Text)
{
	uint16_t refcolumn = Xpos, i = 0;
	uint32_t size = 0, xsize = 0;
	uint8_t *ptr = Text;
	uint8_t *Ascii;

	//Tamaño del texto
	while (*ptr++) size ++ ;

	//Numero de caracteres por linea
	xsize = (BSP_LCD_GetXSize()/Sprite_Font->Width);

	//Verificacion si la columna esta fuera de pantalla
	if ((refcolumn < 1) || (refcolumn >= 0x8000))
		return;

	//Dibuja caracter por caracter
	while ((*Text != 0) & (((BSP_LCD_GetXSize() - (i*Sprite_Font->Width)) & 0xFFFF) >= Sprite_Font->Width))
	{
		Ascii = &Sprite_Font->table[(*Text-' ') * Sprite_Font->Height * ((Sprite_Font->Width)/8)];
		Draw_Char(refcolumn,Ypos,Ascii);
		refcolumn += Sprite_Font->Width;

		Text++;
		i++;
	}
}

//Dibuja el caracter en la posicion indicada
static void Draw_Char(uint16_t Xpos, uint16_t Ypos, const uint8_t *c)
{
	uint32_t i = 0, j = 0;
	uint16_t height, width;
	uint8_t  *pchar;
	uint32_t line;

	height = Sprite_Font->Height;
	width  = Sprite_Font->Width;

	for (i = 0; i < height; i++)
	{
		pchar = ((uint8_t *)c + (width/8) * i);
//		line = (pchar[0]<< 24) | (pchar[1]<< 16) | (pchar[2]<< 8) | pchar[3];
		line = pchar[(width/8)-1];
		for (uint8_t l=(width/8)-1 ;l>0; l--)
			line |= pchar[((width-8)/8)-l] << l*8;

		for (j = 0; j < width; j++)
		{
			if(line & (1 << (width- j)))
				BSP_LCD_DrawPixel((Xpos + j),Ypos,C_Text);
		}
		Ypos++;
	}
}

//Inicializa Random Number Generator del STM
BOOL RNG_Init(void)
{
	HAL_StatusTypeDef status;

	__HAL_RCC_RNG_CLK_ENABLE();
	status = HAL_RNG_Init(&rng_handle);
	if (status!=HAL_OK) return FALSE;
	rng_handle.Instance = RNG;

	return TRUE;
}

//Funcion que retorna un numero entero al azar entre i1 y i2 (-32 767 ~ 32 768).
int16_t Rand_Get(int16_t ri, int16_t rf)
{
	if (ri >= rf) return 0;

	int16_t range,irandom;

	range = rf - ri + 1;
	#ifdef	USE_RNG
	HAL_RNG_GenerateRandomNumber(&rng_handle,&irandom);
	irandom += ri;
	#else
	irandom = ri + rand()%range;
	#endif
	return irandom;
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


//Funcion que mueve una instancia dada una direccion y distancia
void Move_Direction(Sprite_Type_Instance *ins, uint16_t dir, int16_t dist)
{
	if (ins == NULL) return;

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
 * -*ins: puntero de la instancia.
 * -xa,ya,xb,yb: posiciones del rectangulo. (xa debe ser mayor a xb, lo mismo para ya con yb).
 * -xi,yi: adelanto en x y/o y de la instancia.
 * Retorno: TRUE si colisionan.
 */
BOOL Collision_RectangleInstance(Sprite_Type_Instance* ins, uint16_t xa, uint16_t ya, uint16_t xb, uint16_t yb, int16_t xi, int16_t yi)
{
	if (ins == NULL) return FALSE;

	//Valores de posicion para obj
	uint16_t a = ins->x + xi;
	uint16_t b = ins->y + yi;
	uint16_t c = a + Object_Get(ins->parent)->width -1;
	uint16_t d = b + Object_Get(ins->parent)->height -1;

	//Valores de posicion para la zona rectangular
	uint16_t o = xa;
	uint16_t p = ya;
	uint16_t q = xb;
	uint16_t r = yb;

	//Retorna FALSE si el rectangulo esta mal formado
	if (o>=q || p>=r) return FALSE;

	//Revisa colisiones
	if (o>=a && o<=c)
	{
		if (p>=b && p<=d) return TRUE;
		else if (r>=b && r<=d) return TRUE;
	}
	else if (q>=a && q<=c)
	{
		if (p>=b && p<=d) return TRUE;
		else if (r>=b && r<=d) return TRUE;
	}

	if (a>=o && a<=q)
	{
		if (b>=p && b<=r) return TRUE;
		if (d>=p && d<=r) return TRUE;
	}
	else if (c>=o && c<=q)
	{
		if (b>=p && b<=r) return TRUE;
		if (d>=p && d<=r) return TRUE;
	}

	//No hubo colision
    return FALSE;
}


/* Descripcion: Revisa colisiones respecto al hitbox de dos instancias.
 * -*ins: punteros de la instancia 1 y 2.
 * -xi,yi: adelanto en x y/o y de la instancia 1.
 * Retorno: TRUE si colisionan.
 */
BOOL Collision_HitboxInstance(Sprite_Type_Instance* ins1, Sprite_Type_Instance* ins2, int16_t xi, int16_t yi)
{
	if (ins1==NULL || ins2==NULL) return FALSE;

	//Valores de posicion para spr1
	uint16_t a = ins1->x + Object_Get(ins1->parent)->hitbox.x + xi;
	uint16_t b = ins1->y + Object_Get(ins1->parent)->hitbox.y + yi;
	uint16_t c = a + Object_Get(ins1->parent)->hitbox.w -1;
	uint16_t d = b + Object_Get(ins1->parent)->hitbox.h -1;

	//Valores de posicion para spr2
	uint16_t o = ins2->x + Object_Get(ins2->parent)->hitbox.x;
	uint16_t p = ins2->y + Object_Get(ins2->parent)->hitbox.y;
	uint16_t q = o + Object_Get(ins2->parent)->hitbox.w -1;
	uint16_t r = p + Object_Get(ins2->parent)->hitbox.h -1;

	//Revisa colisiones
	if (o>=a && o<=c)
	{
		if (p>=b && p<=d) return TRUE;
		else if (r>=b && r<=d) return TRUE;
	}
	else if (q>=a && q<=c)
	{
		if (p>=b && p<=d) return TRUE;
		else if (r>=b && r<=d) return TRUE;
	}

	if (a>=o && a<=q)
	{
		if (b>=p && b<=r) return TRUE;
		if (d>=p && d<=r) return TRUE;
	}
	else if (c>=o && c<=q)
	{
		if (b>=p && b<=r) return TRUE;
		if (d>=p && d<=r) return TRUE;
	}

	return FALSE;
}
