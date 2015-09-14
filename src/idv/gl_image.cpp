/* =============================================================================
	File: gl_image.cpp

 =============================================================================== */
 //!!! todo - need to find a replacement library to the Apple Graphics Library for Linux in here

#include <algorithm>
#include <iomanip>
#include <FL/Fl.H>

#ifdef WIN32
#define _GDI32_
#include <GL/gl.h>
#include "wglext.h"
#else
#include <GL/gl.h>
//#include <AGL/agl.h>
#endif
//#include "glext.h"

#include "utility.h"
#include "scene/movie.h"
#include "scene/font.h"

#include "world.h"
#include "gl_draw.h"
#include "settings.h"
#include "zip/compress.h"

//----[ static global variables ]----//

static GLFont	*g_glFont10 = NULL;
static GLFont	*g_glFont12 = NULL;
static GLFont	*g_glFont14 = NULL;
static GLFont	*g_glFont36 = NULL;
static GLFont	*g_glFont = NULL;

/* =============================================================================
 =============================================================================== */

void CDraw::setGLFont(int size)
{
	if (size >= 36)
		g_glFont = g_glFont36;
	else if (size >= 14)
		g_glFont = g_glFont14;
	else if (size >= 12)
		g_glFont = (g_Set.m_ShowLargeText ? g_glFont14 : g_glFont12);
	else
		g_glFont = (g_Set.m_ShowLargeText ? g_glFont12 : g_glFont10);
}

/* =============================================================================
 =============================================================================== */
void CDraw::glDrawText(string strText, color clr, int size, float x, float y, float z)
{
	if (!strText.size())
		return;

	setGLFont(size);
	glEnable(GL_TEXTURE_2D);
	g_glFont->Begin();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glColor4f(.3, .3, .3, .8);
	g_glFont->TextOut(strText, (int) x + 1, (int) y - 1, (int) z);
	glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr));
	g_glFont->TextOut(strText, (int) x, (int) y, (int) z);
	glDisable(GL_TEXTURE_2D);
}

/* =============================================================================
 =============================================================================== */
void CDraw::glDrawText(string strText, color clr, float x, float y)
{
	glDrawText(strText, clr, 10, x, y, 0);
}

/* =============================================================================
 =============================================================================== */
void CDraw::drawScaledText(string strText, color clr, Vec3f pos, float scale)
{
	glColor4ub(clrR(clr), clrG(clr), clrB(clr), clrA(clr));
	glEnable(GL_TEXTURE_2D);
	g_glFont36->Begin();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glMatrixMode(GL_MODELVIEW); //  Set our matrix to our model view matrix

	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);

	float	modelview[16];
	int		i, j;
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

	// undo all rotations - beware all scaling is lost, translation is preserved
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
		{
			if (i == j)
				modelview[i * 4 + j] = 1.0;
			else
				modelview[i * 4 + j] = 0.0;
		}

	glLoadMatrixf(modelview);

	scale *= 0.5;
	glScalef(scale, scale, scale);
	g_glFont36->TextOut(strText, 0, 0, 0);
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

/* =============================================================================
 =============================================================================== */
void CDraw::initText()
{
	string	strFontPath = g_Env.m_SystemPath + "/fonts/";
	g_glFont10 = new GLFont();
	g_glFont10->Create((strFontPath + "arial10.glf").c_str());
	g_glFont12 = new GLFont();
	g_glFont12->Create((strFontPath + "arial12.glf").c_str());
	g_glFont14 = new GLFont();
	g_glFont14->Create((strFontPath + "arial14.glf").c_str());
	g_glFont36 = new GLFont();
	g_glFont36->Create((strFontPath + "arial36.glf").c_str());
}

/* =============================================================================
 =============================================================================== */
void CDraw::delText()
{
	delete g_glFont10;
	g_glFont10 = NULL;
	delete g_glFont12;
	g_glFont12 = NULL;
	delete g_glFont14;
	g_glFont14 = NULL;
	delete g_glFont36;
	g_glFont36 = NULL;
}

struct t_name
{
	string	pName;
	Vec3f	vPos;
	Vec3f	vPosUR;
	color	vClr;
	int		iSize;
};

bool operator < (const struct t_name &a, const struct t_name &b)
{
	return a.vPos[2] < b.vPos[2];
}

/* =============================================================================
 =============================================================================== */
void CDraw::draw3DText(string strText, Vec3f pos, color clr, int size, bool bFlush)
{
	static vector<t_name>	names;
	if (!bFlush)
	{
		t_name	t;
		t.pName = strText;
		t.vClr = clr;
		t.iSize = size;

		Vec3f	pos2d = get2DPoint(pos);
		if (pos2d[2] > 1.0 || pos2d[2] < 0.0)
			return;
		t.vPos = pos2d;

		float	dx, dy;
		setGLFont(size);
		g_glFont->getExtent(strText.c_str(), dx, dy);
		t.vPosUR = t.vPos + Vec3f(dx, dy, 0);
		names.push_back(t);
	}
	else
	{
		// sort list by depth;
		sort(names.begin(), names.end());

		enterOrtho2D();
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);

		for (int i = 0; i < names.size(); i++)
		{
			bool	bHidden = false;
			Vec3f	vCenter = names[i].vPos + (names[i].vPosUR - names[i].vPos) / 2;
			for (int j = 0; j < i && !bHidden; j++)	//  check if behind other name
			{
				if (	vCenter[0] < names[j].vPosUR[0]
					&&	vCenter[0] > names[j].vPos[0]
					&&	vCenter[1] < names[j].vPosUR[1]
					&&	vCenter[1] > names[j].vPos[1]) 
					bHidden = true;
			}

			if (!bHidden)
			{
				glDrawText
				(
					names[i].pName,
					names[i].vClr,
					names[i].iSize,
					names[i].vPos[0],
					getHeight() - names[i].vPos[1],
					0
				);
			}
		}

		names.clear();

		glPopAttrib();
		exitOrtho2D();
	}
}

/* =============================================================================
    SAVE IMAGE must be called in to create and then release before returning to
    drawing pc window
 =============================================================================== */
bool CDraw::setImageBuffer(int ww, int hh, bool bEntry)
{
	static int	wOld = 0;
	static int	hOld = 0;

	if (bEntry)
	{
		GLint	vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		wOld = vp[2];
		hOld = vp[3];
		if (ww == -1) ww = wOld;
		if (hh == -1) hh = hOld;
	}

	if (bEntry)
	{
		if (ww == wOld && hh == hOld)
			return true;

		if (ww <= wOld && hh <= hOld)
		{
			resize_glwindow(ww, hh);
			return true;
		}
	}

#ifdef WIN32
	static HDC							hDC = NULL;
	static HGLRC						hRC = NULL;
	static HPBUFFERARB					pbuffer = NULL;
	static HDC							hpbufdc = NULL;
	static HGLRC						hpbufglrc = NULL;

	static PFNWGLCREATEPBUFFERARBPROC	wglCreatePbufferARB = NULL;
	static PFNWGLGETPBUFFERDCARBPROC	wglGetPbufferDCARB = NULL;
	static PFNWGLDESTROYPBUFFERARBPROC	wglDestroyPbufferARB = NULL;
#else
	/*
	static AGLContext					aglConOld = NULL;
	static AGLPbuffer					pbuffer = NULL;
	static AGLContext					aglConNew = NULL;
	*/
#endif
	if (bEntry)
	{
		cout << "Resizing the opengl context before rendering.";

#ifdef WIN32
		if (wglCreatePbufferARB == NULL)
		{
			wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC) wglGetProcAddress("wglCreatePbufferARB");
			wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC) wglGetProcAddress("wglGetPbufferDCARB");
			wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC) wglGetProcAddress("wglDestroyPbufferARB");
			if (wglCreatePbufferARB == NULL || wglGetPbufferDCARB == NULL || wglDestroyPbufferARB == NULL)
				return false;
		}

		hDC = wglGetCurrentDC();
		hRC = wglGetCurrentContext();

		PIXELFORMATDESCRIPTOR	pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),	//  size of this pfd
			1,	//  version number
			PFD_SUPPORT_OPENGL |	//  support OpenGL
			PFD_DOUBLEBUFFER,		//  double buffered
			PFD_TYPE_RGBA,			//  RGBA type
			24, //  24-bit color depth
			0,
			0,
			0,
			0,
			0,
			0,	//  color bits ignored
			0,	//  no alpha buffer
			0,	//  shift bit ignored
			0,	//  no accumulation buffer
			0,
			0,
			0,
			0,	//  accum bits ignored
			32, //  32-bit z-buffer
			0,	//  no stencil buffer
			0,	//  no auxiliary buffer
			PFD_MAIN_PLANE, //  main layer
			0,		//  reserved
			0,
			0,
			0		//  layer masks ignored
		};
		int						iPixelFormat = ChoosePixelFormat(hDC, &pfd);
		if (iPixelFormat == 0 && g_Env.m_Verbose)
		{
			cout << ("ERROR: No compatible image formats found for card");
			return false;
		}

		const int	pbufferAttributes[] =
		{
			WGL_TEXTURE_FORMAT_ARB,
			WGL_TEXTURE_RGBA_ARB,
			WGL_TEXTURE_TARGET_ARB,
			WGL_TEXTURE_2D_ARB,
			0
		};

		pbuffer = wglCreatePbufferARB(hDC, iPixelFormat, ww, hh, pbufferAttributes);
		if (pbuffer == NULL)
		{
			ostringstream	s;
			s << "Failed to create buffer for image of size: " << ww << " x " << hh;
			cout << (s.str());
			return false;
		}

#else
		/*
		aglConOld = aglGetCurrentContext();
		pbuffer = NULL;

		GLint			attrib[] =
		{
			AGL_RGBA,
			AGL_RED_SIZE,
			8,
			AGL_BLUE_SIZE,
			8,
			AGL_GREEN_SIZE,
			8,
			AGL_ALPHA_SIZE,
			8,
			AGL_DEPTH_SIZE,
			32,
			AGL_NO_RECOVERY,
			AGL_NONE
		};

		AGLPixelFormat	aglPixFmt = aglChoosePixelFormat(NULL, 0, attrib);
		if (!aglPixFmt)
		{
			cout << ("Failed to chose pixel format");
			goto clean_up;
		}

		aglConNew = aglCreateContext(aglPixFmt, NULL);
		aglDestroyPixelFormat(aglPixFmt);
		if (!aglConNew)
		{
			cout << ("Failed to create context");
			goto clean_up;
		}

		GLboolean	bRet = aglCreatePBuffer(ww, hh, GL_TEXTURE_RECTANGLE_EXT, GL_RGBA, 0, &pbuffer);
		if (pbuffer == NULL || !bRet)
		{
			ostringstream	s;
			s << "Failed to create buffer for image of size: " << ww << " x " << hh;
			cout << (s.str());
			return false;
		}
		*/
#endif
		editCompass().clear();
		clearWatermark();
		delText();
		g_World.unloadMaterials();

#ifdef WIN32
		hpbufdc = wglGetPbufferDCARB(pbuffer);
		if (hpbufdc == NULL)
		{
			cout << ("Failed to set PBuffer");
			goto clean_up;
		}

		hpbufglrc = wglCreateContext(hpbufdc);
		if (hpbufglrc == NULL)
		{
			cout << ("Failed to create new context");
			goto clean_up;
		}

		wglMakeCurrent(hpbufdc, hpbufglrc);
#else
		/*
		bRet = aglSetPBuffer(aglConNew, pbuffer, 0, 0, 0);
		if (!bRet)
		{
			cout << ("Failed to set PBuffer");
			goto clean_up;
		}

		bRet = aglSetCurrentContext(aglConNew);
		if (!bRet)
		{
			cout << ("Failed to set current context");
			goto clean_up;
		}
		*/
#endif

		//  reinitialize OpenGL for this GLRC
		initState();
		resize_glwindow(ww, hh);

		//  Reload all the textures for this context
		editCompass().draw(true);
		initWatermark();
		initText();
		g_World.reloadMaterials();

		return true;
	}

	//  Otherwise we're returning to normal onscreen buffer
	//if (!pbuffer)	//  didn't create offscreen buffer
	//{
		resize_glwindow(wOld, hOld);
		return true;
	//}

	//  clear out materials from this context
	editCompass().clear();
	clearWatermark();
	delText();
	g_World.unloadMaterials();

	//  go back to normal onscreen context
clean_up:
	cout << ("Returning to onscreen context.");
#ifdef WIN32
	wglMakeCurrent(hDC, hRC);
	if (pbuffer) wglDestroyPbufferARB(pbuffer);
	pbuffer = NULL;
	if (hpbufglrc) wglDeleteContext(hpbufglrc);
	hpbufglrc = NULL;
#else
	/*
	aglSetCurrentContext(aglConOld);
	if (pbuffer) aglDestroyPBuffer(pbuffer);
	pbuffer = NULL;
	if (aglConNew) aglDestroyContext(aglConNew);
	aglConNew = NULL;
	*/
#endif
	resize_glwindow(wOld, hOld);

	editCompass().draw(true);
	initWatermark();
	initText();
	g_World.reloadMaterials();

	g_Set.m_NeedRedraw = true;
	return true;
}

/* =============================================================================
 =============================================================================== */
void CDraw::getGLFrame(unsigned char *imageBuffer, int ww, int hh)
{
	int iOldTop = getTopOffset();
	int iOldBot = getBotOffset();
	setScreenOffsets(0, 0);

	g_World.updateTerrain();
	drawGL();
	glFinish();
	glReadBuffer(GL_BACK);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, ww, hh, GL_RGB, GL_UNSIGNED_BYTE, imageBuffer);

	setScreenOffsets(iOldTop, iOldBot);
}

/* =============================================================================
 =============================================================================== */
bool CDraw::saveGLFrame(string fileName)
{
	GLint	vp[4];

	glGetIntegerv(GL_VIEWPORT, vp);

	int				ww = vp[2];
	int				hh = vp[3];

	unsigned char	*imageBuffer = NULL;
	if (!mem_alloc(imageBuffer, 3 * ww * hh))
		return false;

	bool	bOldLocater = g_Set.m_ShowLocater;
	g_Set.m_ShowLocater = false;

	bool	bOldLighting = g_Set.m_ShowLighting;
	g_Set.m_ShowLighting = false;

	bool	bOldTimeline = g_Set.m_ShowTimeline;
	g_Set.m_ShowTimeline = false;

	if (getExt(fileName) == "") setExt(fileName, ".bmp");

	g_Set.m_Printing = true;
	g_World.getTerrain().setTileScale(1.0);
	getGLFrame(imageBuffer, ww, hh);
	g_Set.m_Printing = false;
	::saveImage(fileName, imageBuffer, ww, hh, m_Quality);

	delete[] imageBuffer;
	g_Set.m_ShowLocater = bOldLocater;
	g_Set.m_ShowLighting = bOldLighting;
	g_Set.m_ShowTimeline = bOldTimeline;
	g_Set.m_NeedRedraw = true;

	return true;
}

void	progress_dlg(string caption);
void	progress_update(string pText, double val);
void	progress_end();
void	progress_cancel_btn(bool b);

/* =============================================================================
 =============================================================================== */

int get_show_state()
{
	return(g_Set.m_ShowTerrain ? 0x1 : 0) 
		| (g_Set.m_ShowData ? 0x2 : 0) 
		| (g_Set.m_ShowNames ? 0x4 : 0) 
		| (g_Set.m_ShowObjects ? 0x8 : 0) 
		| (g_Set.m_ShowLines ? 0x10 : 0);
}

/* =============================================================================
 =============================================================================== */
void set_show_state(int iState)
{
	g_Set.m_ShowTerrain = (iState & 0x1);
	g_Set.m_ShowData = (iState & 0x2);
	g_Set.m_ShowNames = (iState & 0x4);
	g_Set.m_ShowObjects = (iState & 0x8);
	g_Set.m_ShowLines = (iState & 0x10);
}

/* =============================================================================
 =============================================================================== */
bool CDraw::saveLayeredFrame(string fileName)
{
	static string	pExt[] = { "trn", "feat", "data", "names", "obj", "line" };

	int				iPos = (int) fileName.rfind('.');
	string			strBase = fileName.substr(0, iPos);
	string			strExt = fileName.substr(iPos);

	int				iOldState = get_show_state();
	int				iNewState = 0x1;
	bool			bRet = true;
	for (int i = 0; i < 6; i++, iNewState <<= 1)
	{
		set_show_state(iNewState);

		string	fname = strBase + "_" + pExt[i] + strExt;
		bRet &= saveGLFrame(fileName);
	}

	set_show_state(iOldState);
	return bRet;
}

/* =============================================================================
 =============================================================================== */
void CDraw::initVisSettings(CVisScript &VisScript)
{
	if (VisScript.m_Height <= 0 || VisScript.m_Width <= 0 || VisScript.m_CurScreenSize)
	{
		VisScript.m_Height = getHeight();
		VisScript.m_Width = getWidth();
	}

	m_Quality = min(100, max(1, VisScript.m_Quality));
	g_Set.m_WaterMark = VisScript.m_WaterMark;
	g_Set.m_TimeStamp = VisScript.m_TimeStamp;
}

/* =============================================================================
 =============================================================================== */
bool CDraw::saveImage(string fileName, CVisScript VisScript)
{
	initVisSettings(VisScript);

	if (!setImageBuffer(VisScript.m_Width, VisScript.m_Height, true)) 
		return false;

	int iViews = VisScript.m_Views.size();

	if (!g_Set.m_ServerMode && iViews > 1)
	{
		//progress_cancel_btn(true);
		//progress_dlg("Saving Images");
		//progress_cancel_btn(false);
	}

	g_Set.m_Printing = true;
	g_World.getTerrain().setTileScale(1.0);
	g_Set.m_NeedRedraw = false;
	for (int v = 0; v < max(iViews, 1); v++)
	{
		if (g_Env.m_SystemInterrupt)
			break;
		//if (!g_Set.m_ServerMode && iViews > 1)
		//	progress_update(VisScript.m_Views[v], (double) v / (double) iViews);
		//Fl::check();

		double	dStart = 0, dFinish = 0;
		if (VisScript.m_Views.size() > 0)
			g_World.setNewView(VisScript.m_Views[v], dStart, dFinish, VisScript.m_UpdateCamera);

		string	outFile = fileName;

		if (iViews > 1)
		{
			ostringstream	s;
			s << remExt(fileName) << setfill('0') << setw(4) << v << getExt(fileName);
			outFile = s.str();
		}

		if (VisScript.m_ImageLayers)
			saveLayeredFrame(outFile);
		else
			saveGLFrame(outFile);
	}

	g_Set.m_Printing = false;
	//if (!g_Set.m_ServerMode && iViews > 1) progress_end();
	if (!setImageBuffer(VisScript.m_Width, VisScript.m_Height, false)) 
		return false;

	g_Set.m_NeedRedraw = true;
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CDraw::saveMovie(string szFileName, CVisScript VisScript)
{
	unsigned char	*pMovieBuffer = NULL;
	CMovie			*pNewMoviePtr = NULL;
	bool			bRet = false;
	double			time_inc;
	double			dTime;

	initVisSettings(VisScript);

	double	dStart = g_World.getTimeLine().getSelStart();
	double	dFinish = g_World.getTimeLine().getSelFinish();

	int		iViews = VisScript.m_Views.size();
	int		iTotFrames = 0;

	bool	bOldLocater = g_Set.m_ShowLocater;
	g_Set.m_ShowLocater = false;

	bool	bOldLighting = g_Set.m_ShowLighting;
	g_Set.m_ShowLighting = false;

	bool	bOldTimeline = g_Set.m_ShowTimeline;
	g_Set.m_ShowTimeline = false;

	if (!setImageBuffer(VisScript.m_Width, VisScript.m_Height, true)) 
		goto clean_up;
	if (!mem_alloc(pMovieBuffer, 3 * VisScript.m_Width * VisScript.m_Height))
		goto clean_up;

	if (!VisScript.m_MovieJPEGS)
	{
		//pNewMoviePtr = new CMovie();
		//if (pNewMoviePtr->StartNewMovie(szFileName.c_str(),
		//	VisScript.m_Width,VisScript.m_Height,	VisScript.m_MovieSpeed,	VisScript.m_Quality) != 0)
		//	goto clean_up;
	}

	if (!g_Set.m_ServerMode)
	{
		//progress_cancel_btn(true);
		//progress_dlg("Saving Movie");
		//progress_cancel_btn(false);
	}

	g_Set.m_Printing = true;
	g_World.getTerrain().setTileScale(1.0);
	for (int v = 0; v < max(iViews, 1); v++)
	{
		Vec3d	p_pos, d_pos;
		double	p_elev, p_azim, p_dolly, d_elev, d_azim, d_dolly;

		if (g_Env.m_SystemInterrupt)
			break;

		if (iViews > 0)
		{
			//progress_update("Loading " + VisScript.m_Views[v], 0);

			if (VisScript.m_MovieTrans_Fade && v > 0)
				initFadeTimer();
			getCameraView(p_pos, p_elev, p_azim, p_dolly);
			g_World.setNewView(VisScript.m_Views[v], dStart, dFinish, VisScript.m_UpdateCamera);
			getCameraView(d_pos, d_elev, d_azim, d_dolly);
			d_pos -= p_pos;
			d_elev -= p_elev;
			d_azim -= p_azim;
			d_dolly -= p_dolly;
			if (VisScript.m_MovieTrans_Fade && v > 0) 
				startFadeTimer(VisScript.m_MovieTransTime);

			//Fl::check();
		}

		g_Set.m_ShowLocater = false;
		g_Set.m_ShowLighting = false;
		g_Set.m_ShowTimeline = false;

		int iTransition = 0;
		int iTransitionFrames = VisScript.m_MovieTransTime * VisScript.m_MovieSpeed;
		if (v > 0 && VisScript.m_MovieTransition)
			iTransition = iTransitionFrames;

		int iViewFrames = VisScript.m_MovieTime * VisScript.m_MovieSpeed + iTransition;
		time_inc = (dFinish - dStart) / (double) (iViewFrames - 1);
		dTime = dStart;

		double	dPeak = len(d_pos * Vec3d(1, 1, 0)) * .3;
		double	dRate = (double) VisScript.m_RotateSpeed / (double) VisScript.m_MovieSpeed;

		for (int i = 0; i < iViewFrames; iTotFrames++, i++)
		{
			if (g_Env.m_SystemInterrupt)
				break;

			if (!g_Set.m_ServerMode)
			{
				ostringstream	s;
				if (iViews > 0) 
					s << VisScript.m_Views[v] << ": ";
				s << "Frame " << i + 1;
				//progress_update(s.str(), (double) (i + 1) / (double) iViewFrames);
			}

			//Fl::check();

			if (iTransition == 0)
			{
				g_World.getTimeLine().setTime(dTime);
				g_World.updateTime();
				dTime += time_inc;
			}
			else
			{
				iTransition--;

				double	dInc = (double) (iTransitionFrames - iTransition) / (double) iTransitionFrames;
				if (VisScript.m_MovieTrans_Fade && v > 0) 
					setFadeTimer(dInc * VisScript.m_MovieTransTime);
				dInc = VisScript.m_MovieTrans_Camera ? dInc : 1.0;
				getCamera().moveCamera
					(
						getCameraFromGPS(p_pos + dInc * d_pos),
						p_elev + dInc * d_elev,
						p_azim + dInc * d_azim,
						p_dolly + dInc * d_dolly + dPeak * sin(vl_pi * dInc),
						0
					);
			}

			getGLFrame(pMovieBuffer, VisScript.m_Width, VisScript.m_Height);
			//if (!VisScript.m_MovieJPEGS)
			//	pNewMoviePtr->AppendFrame(pMovieBuffer);
			//else
			//{
				ostringstream	s;
				s << remExt(szFileName) << setfill('0') << setw(4) << iTotFrames << ".jpg";
				::saveImage(s.str(), pMovieBuffer, VisScript.m_Width, VisScript.m_Height, VisScript.m_Quality);
			//}

			if (VisScript.m_MovieRotate) 
				getCamera().setAzimuth(getCamera().getAzimuth() + (dRate * vl_pi / 360.0));
			if (VisScript.m_WorldRotate) 
				setLookAtGPS(getLookAtGPS() + Vec3d(0, dRate * 0.1, 0));
		}
	}

	cout << endl;
	g_Set.m_Printing = false;
	if (VisScript.m_MovieTrans_Fade) exitFadeTimer();
	//if (!g_Set.m_ServerMode) progress_end();
	bRet = true;

clean_up:
	//if (pNewMoviePtr) delete pNewMoviePtr;
	//if (pMovieBuffer) delete pMovieBuffer;
	//pMovieBuffer = NULL;
	setImageBuffer(VisScript.m_Width, VisScript.m_Height, false);

	g_Set.m_ShowLocater = bOldLocater;
	g_Set.m_ShowLighting = bOldLighting;
	g_Set.m_ShowTimeline = bOldTimeline;
	g_Set.m_NeedRedraw = true;

	return bRet;
}

/* =============================================================================
 =============================================================================== */
string CDraw::saveVisualization(string szFileName, CVisScript VisScript)
{
	bool	bRet;
	if (VisScript.m_Movie)
		bRet = saveMovie(szFileName, VisScript);
	else
		bRet = saveImage(szFileName, VisScript);
	if (!bRet)
		return "";

	//  if multiple files made then zip them up
	if ((VisScript.m_Movie && VisScript.m_MovieJPEGS) || (!VisScript.m_Movie && VisScript.m_Views.size() > 1))
	{
		changedir(getFilePath(szFileName));

		string			strBase = getFileNameBase(szFileName);
		vector<string>	vFiles = listfiles(".", false);
		for (int i = vFiles.size() - 1; i >= 0; i--)
		{
			if (getExt(vFiles[i]) != ".jpg" || vFiles[i].substr(0, strBase.size()) != strBase)
				vFiles.erase(vFiles.begin() + i);
		}

		compressFiles(strBase + ".zip", vFiles);
		for (int i = 0; i < vFiles.size(); i++) 
			delfile(vFiles[i]);
		changedir(g_Env.m_AppPath);

		setExt(szFileName, ".zip");
	}

	return szFileName;
}
