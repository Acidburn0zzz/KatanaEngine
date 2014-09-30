/*	Copyright (C) 2011-2014 OldTimes Software
*/
#ifndef __ENGINEGAME__
#define __ENGINEGAME__

#include "quakedef.h"

extern ModuleExport_t	*Game;

/*	Global					*/
void Game_Initialize(void);

/*	Server-side functions	*/
void Server_Flare(vec3_t org,float r,float g,float b,float a,float scale,char *texture);

/*	Client-side functions	*/
void Client_ParseFlareEffect(void);

#endif