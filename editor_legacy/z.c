#include "qe3.h"

#define	PAGEFLIPS	2

z_t		z;

void Z_Init (void)
{
	z.origin[0] = 0;
	z.origin[1] = 20;
	z.origin[2] = 46;

	z.scale = 1;
}

/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

static	int	cursorx, cursory;

void Z_MouseDown (int x, int y, int buttons)
{
	vec3_t	org,dir,vup,vright;
	brush_t	*b;

	Sys_GetCursorPos (&cursorx, &cursory);

	vup[0] = 0; 
	vup[1] = 0; 
	vup[2] = 1/z.scale;

	Math_VectorCopy(z.origin,org);
	org[2] += (y - (z.height/2))/z.scale;
	org[1] = -8192;

	b = selected_brushes.next;
	if (b != &selected_brushes)
		org[0] = (b->mins[0] + b->maxs[0])/2;

	dir[0] = 0; dir[1] = 1; dir[2] = 0;

	// [7/8/2012] Simplified ~hogsy
	Math_VectorClear(vright);

	// LBUTTON = manipulate selection
	// shift-LBUTTON = select
	// middle button = grab texture
	// ctrl-middle button = set entire brush to texture
	// ctrl-shift-middle button = set single face to texture
	if ( (buttons == MK_LBUTTON)
		|| (buttons == (MK_LBUTTON | MK_SHIFT))
		|| (buttons == MK_MBUTTON)
		|| (buttons == (MK_MBUTTON|MK_SHIFT|MK_CONTROL)) )
	{
		Drag_Begin (x, y, buttons,
			vright, vup,
			org, dir);
		return;
	}

	// control mbutton = move camera
	if ((buttons == (MK_CONTROL|MK_MBUTTON) ) || (buttons == (MK_CONTROL|MK_LBUTTON)))
	{	
		camera.origin[2] = org[2] ;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z);
	}
}

void Z_MouseMoved (int x, int y, int buttons)
{
	if (!buttons)
		return;

	if(buttons == MK_LBUTTON)
	{
		Drag_MouseMoved(x,y,buttons);
		Sys_UpdateWindows (W_Z|W_CAMERA);
		return;
	}

	// rbutton = drag z origin
	if (buttons == MK_RBUTTON)
	{
		Sys_GetCursorPos (&x, &y);
		if ( y != cursory)
		{
			z.origin[2] += y-cursory;
			Sys_SetCursorPos (cursorx, cursory);
			Sys_UpdateWindows (W_Z);
		}
		return;
	}
		// control mbutton = move camera
	if ((buttons == (MK_CONTROL|MK_MBUTTON) ) || (buttons == (MK_CONTROL|MK_LBUTTON)))
	{	
		camera.origin[2] = (y - (z.height/2))/z.scale;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z);
	}
}


/*
============================================================================

DRAWING

============================================================================
*/

void Z_DrawGrid (void)
{
	float	zz, zb, ze;
	int		w, h;
	char	text[32];

	w = z.width/2/z.scale;
	h = z.height/2/z.scale;

	zb = z.origin[2] - h;
	if (zb < region_mins[2])
		zb = region_mins[2];
	zb = 64.0f*floorf (zb/64);

	ze = z.origin[2] + h;
	if (ze > region_maxs[2])
		ze = region_maxs[2];
	ze = 64.0f*ceilf(ze/64);

	// draw major blocks

	glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR]);

	glBegin (GL_LINES);

	glVertex2f (0, zb);
	glVertex2f (0, ze);

	for (zz=zb ; zz<ze ; zz+=64)
	{
		glVertex2f (-w, zz);
		glVertex2f (w, zz);
	}

	glEnd ();

	// draw minor blocks
	if(g_qeglobals.bShowGrid && g_qeglobals.d_gridsize*z.scale >= 4)
	{
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR]);

		glBegin (GL_LINES);
		for (zz=zb ; zz<ze ; zz+=g_qeglobals.d_gridsize)
		{
			if ( ! ((int)zz & 63) )
				continue;
			glVertex2f (-w, zz);
			glVertex2f (w, zz);
		}
		glEnd ();
	}

	// draw coordinate text if needed

	// [8/8/2012] Changed to green so we show better against our background ~hogsy
	glColor4f(0,1.0f,0,0);

	for (zz=zb ; zz<ze ; zz+=64)
	{
		glRasterPos2f (-w+1, zz);
		sprintf(text, "%i",(int)zz);
		glCallLists (strlen(text), GL_UNSIGNED_BYTE, text);
	}
}

#define CAM_HEIGHT		48 // height of main part
#define CAM_GIZMO		8	// height of the gizmo

void Z_Draw (void)
{
    brush_t		*brush;
	float		w,h,x,y;
	double		start=0, end;
	qtexture_t	*q;
	float		top, bottom;
	vec3_t		org_top, org_bottom, dir_up, dir_down;
	int			xCam = z.width/3;

	if (!active_brushes.next)
		return;	// not valid yet

	if (z.timing)
		start = Sys_DoubleTime ();

	//
	// clear
	//
	glViewport(0, 0, z.width, z.height);

	glClearColor (
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][0],
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][1],
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][2],
		0);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 

	glMatrixMode(GL_PROJECTION);

    glLoadIdentity ();
	w = z.width/2 / z.scale;
	h = z.height/2 / z.scale;
	glOrtho (-w, w, z.origin[2]-h, z.origin[2]+h, -8, 8);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	//
	// now draw the grid
	//
	Z_DrawGrid ();

	//
	// draw stuff
	//
	glDisable(GL_CULL_FACE);

	glShadeModel (GL_FLAT);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	Math_VectorClear(dir_up);
	Math_VectorClear(dir_down);

	// draw filled interiors and edges
	dir_up[2]	= 1;
	dir_down[2] = -1;

	Math_VectorCopy(z.origin,org_top);
	org_top[2] = 4096.0f;
	Math_VectorCopy(z.origin,org_bottom);
	org_bottom[2] = -4096.0f;

	for (brush = active_brushes.next ; brush != &active_brushes ; brush=brush->next)
	{
		if (brush->mins[0] >= z.origin[0]
			|| brush->maxs[0] <= z.origin[0]
			|| brush->mins[1] >= z.origin[1]
			|| brush->maxs[1] <= z.origin[1])
			continue;

		if (!Brush_Ray (org_top, dir_down, brush, &top))
			continue;
		top = org_top[2] - top;
		if (!Brush_Ray (org_bottom, dir_up, brush, &bottom))
			continue;
		bottom = org_bottom[2] + bottom;

		q = Texture_ForName (brush->brush_faces->texdef.name);
		glColor3f (q->color[0], q->color[1], q->color[2]);
		glBegin (GL_QUADS);
		glVertex2f(-xCam,bottom);
		glVertex2f(xCam,bottom);
		glVertex2f(xCam,top);
		glVertex2f(-xCam,top);
		glEnd ();

		glColor3f (1,1,1);
		glBegin (GL_LINE_LOOP);
		glVertex2f (-xCam, bottom);
		glVertex2f (xCam, bottom);
		glVertex2f (xCam, top);
		glVertex2f (-xCam, top);
		glEnd ();
	}

	//
	// now draw selected brushes
	//
	for (brush = selected_brushes.next ; brush != &selected_brushes ; brush=brush->next)
	{
		if ( !(brush->mins[0] >= z.origin[0]
			|| brush->maxs[0] <= z.origin[0]
			|| brush->mins[1] >= z.origin[1]
			|| brush->maxs[1] <= z.origin[1]) )
		{
			if (Brush_Ray (org_top, dir_down, brush, &top))
			{
				top = org_top[2] - top;
				if (Brush_Ray (org_bottom, dir_up, brush, &bottom))
				{
					bottom = org_bottom[2] + bottom;

					q = Texture_ForName (brush->brush_faces->texdef.name);
					glColor3f (q->color[0], q->color[1], q->color[2]);
					glBegin (GL_QUADS);
					glVertex2f (-xCam, bottom);
					glVertex2f (xCam, bottom);
					glVertex2f (xCam, top);
					glVertex2f (-xCam, top);
					glEnd ();
				}
			}
		}

		glColor3f (1,0,0);
		glBegin (GL_LINE_LOOP);
		glVertex2f (-xCam, brush->mins[2]);
		glVertex2f (xCam, brush->mins[2]);
		glVertex2f (xCam, brush->maxs[2]);
		glVertex2f (-xCam, brush->maxs[2]);
		glEnd ();
	}

	/*	Camera
	*/
	x = 0;
	y = camera.origin[2];

	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glColor3f(0,1.0f,1.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(x-xCam,y,0);
	glVertex3f(x,y+CAM_GIZMO,0);
	glVertex3f(x+xCam,y,0);
	glVertex3f(x,y-CAM_GIZMO,0);
	glVertex3f(x-xCam,y,0);
	glVertex3f(x+xCam,y,0);
	glVertex3f(x+xCam,y-CAM_HEIGHT,0);
	glVertex3f(x-xCam,y-CAM_HEIGHT,0);
	glVertex3f(x-xCam,y,0);
	glEnd();

	glDisable(GL_LINE_SMOOTH);

	glColor4f(0,1.0f,1.0f,0.25f);
	glBegin(GL_QUADS);
	glVertex3f(x-xCam,y,0);
	glVertex3f(x,y+CAM_GIZMO,0);
	glVertex3f(x+xCam,y,0);
	glVertex3f(x,y-CAM_GIZMO,0);
	glVertex3f(x-xCam,y,0);
	glVertex3f(x+xCam,y,0);
	glVertex3f(x+xCam,y-CAM_HEIGHT,0);
	glVertex3f(x-xCam,y-CAM_HEIGHT,0);
	glVertex3f(x-xCam,y,0);
	glEnd();

	glDisable(GL_BLEND);

    glFinish();

	if (z.timing)
	{
		end = Sys_DoubleTime ();
		Console_Print(FALSE,"z: %i ms\n", (int)(1000*(end-start)));
	}
}

