/* =============================================================================
	File: gl_windows.cpp

 =============================================================================== */

#pragma warning(push)
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)
#ifdef WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif
#pragma warning(pop)
#include <FL/Fl.H>
#include "timer.h"
#include "world.h"
#include "gl_draw.h"
#include "settings.h"
#include "utility.h"
#include "web/web_file.h"

#include <vl/VLgl.h>

/* =============================================================================
 =============================================================================== */

bool CWindow::in(int x, int y)
{
	m_InWindow = x > m_x && x < m_x + m_w && y > m_y && y < m_y + m_h;
	return m_InWindow;
}

/* =============================================================================
 =============================================================================== */
bool CWindow::getClick(int x, int y)
{
	m_state = WIN_NONE;
	if (!in(x, y))
		return false;
	if (x > m_x && x < m_x + 10 && y > m_y && y < m_y + 10)
		m_state = WIN_POS;
	else if (x > m_x + m_w - 10 && x < m_x + m_w && y > m_y + m_h - 10 && y < m_y + m_h)
		m_state = WIN_SIZE;
	return true;
}

/* =============================================================================
 =============================================================================== */
void CWindow::endClick(int x, int y)
{
	m_state = WIN_NONE;
}

/* =============================================================================
 =============================================================================== */
void CWindow::makePosVisible()
{
	m_w = min(m_Width, m_w);
	m_h = min(m_Height, m_h);
	m_x = max(0, min(m_x, m_Width - m_w));
	m_y = max(m_TopOffset, min(m_y, m_Height - m_BotOffset - m_h));
}

/* =============================================================================
 =============================================================================== */
void CWindow::setExtents(int w, int h, int top, int bot)
{
	if (m_Width != 0 && m_Height != 0)
	{
		m_w = m_w * w / m_Width;
		m_h = m_h * h / m_Height;
		m_x = m_x * w / m_Width;
		m_y = top + (m_y - top) * (h - (bot + top)) / (m_Height - (bot + top));
	}

	m_Width = w;
	m_Height = h;
	m_TopOffset = top;
	m_BotOffset = bot;
	makePosVisible();
}

/* =============================================================================
 =============================================================================== */
bool CWindow::setMove(int x, int y)
{
	if (m_state == WIN_POS)
	{
		m_x -= x;
		m_y -= y;
		makePosVisible();
	}
	else if (m_state == WIN_SIZE)
	{
		m_w -= x;
		m_h -= y;
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
void CWindow::setPos(int x, int y, int w, int h)
{
	if (x != -1) m_x = x;
	if (y != -1) m_y = y;
	if (w != -1) m_w = w;
	if (h != -1) m_h = h;
	makePosVisible();
}

/* =============================================================================
 =============================================================================== */
void CWindow::getPos(int &x, int &y, int &w, int &h) const
{
	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

/* =============================================================================
 =============================================================================== */
void CWindow::setTexWindow(float fTxMinX, float fTxMaxX, float fTxMinY, float fTxMaxY)
{
	m_TxMinX = fTxMinX;
	m_TxMaxX = fTxMaxX;
	m_TxMinY = fTxMinY;
	m_TxMaxY = fTxMaxY;
}

/* =============================================================================
 =============================================================================== */
void CWindow::draw() const
{
	if (m_h == 0 || m_h == 0)
		return;

	glColor4f(0.1, 0.1, 0.1, 0.85);
	glBegin(GL_QUADS);
	glVertex2f(m_x - 3, m_Height - (m_y + m_h + 3));
	glVertex2f(m_x + m_w + 3, m_Height - (m_y + m_h + 3));
	glVertex2f(m_x + m_w + 3, m_Height - (m_y - 3));
	glVertex2f(m_x - 3, m_Height - (m_y - 3));
	glEnd();

	//  Assumes texture is bound on entry
	if (m_TxMaxX > m_TxMinX && m_TxMaxY > m_TxMinY)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glColor4f(1, 1, 1, 1);

		glBegin(GL_QUADS);
		glTexCoord2f(m_TxMinX, m_TxMaxY);
		glVertex2f(m_x, m_Height - (m_y + m_h));	//  bottom left
		glTexCoord2f(m_TxMaxX, m_TxMaxY);
		glVertex2f(m_x + m_w, m_Height - (m_y + m_h));
		glTexCoord2f(m_TxMaxX, m_TxMinY);
		glVertex2f(m_x + m_w, m_Height - m_y);
		glTexCoord2f(m_TxMinX, m_TxMinY);
		glVertex2f(m_x, m_Height - m_y);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	//  draw window decorations
	if (m_InWindow)
	{
		glLineWidth(3);

		//  upper left - move
		glColor4f(0.25, 0.25, 0.25, 0.5);
		glBegin(GL_QUADS);
		glVertex2f(m_x, m_Height - (m_y + 16));
		glVertex2f(m_x + 16, m_Height - (m_y + 16));
		glVertex2f(m_x + 16, m_Height - m_y);
		glVertex2f(m_x, m_Height - m_y);
		glEnd();
		glColor4f(0.85, 0.85, 0.25, 1);
		glBegin(GL_LINES);
		glVertex2f(m_x + 2, m_Height - (m_y + 8));
		glVertex2f(m_x + 14, m_Height - (m_y + 8));
		glVertex2f(m_x + 8, m_Height - (m_y + 2));
		glVertex2f(m_x + 8, m_Height - (m_y + 14));
		glEnd();

		//  lower right size
		glColor4f(0.25, 0.25, 0.25, 0.5);
		glBegin(GL_QUADS);
		glVertex2f(m_x + m_w - 16, m_Height - (m_y + m_h));
		glVertex2f(m_x + m_w, m_Height - (m_y + m_h));
		glVertex2f(m_x + m_w, m_Height - (m_y + m_h - 16));
		glVertex2f(m_x + m_w - 16, m_Height - (m_y + m_h - 16));
		glEnd();
		glColor4f(0.85, 0.85, 0.25, 1);
		glBegin(GL_LINES);
		glVertex2f(m_x + m_w - 14, m_Height - (m_y + m_h - 3));
		glVertex2f(m_x + m_w - 2, m_Height - (m_y + m_h - 3));
		glVertex2f(m_x + m_w - 3, m_Height - (m_y + m_h - 2));
		glVertex2f(m_x + m_w - 3, m_Height - (m_y + m_h - 14));
		glEnd();
	}
}

/* =============================================================================
    Locater Window
 =============================================================================== */
void CLocater::getData(string &Path, double &x0, double &y0, double &x1, double &y1)
{
	Path = m_path;
	x0 = m_curx;
	y0 = m_cury;
	x1 = m_curx + m_curw;
	y1 = m_cury + m_curh;
}

/* =============================================================================
 =============================================================================== */
Vec3d CLocater::setPos(int x, int y)
{
	int w_x, w_y, w_w, w_h;
	getPos(w_x, w_y, w_w, w_h);

	float	lon = (float) (x - w_x) / float(w_w) * m_curw + m_curx;
	float	lat = (1.0f - (float) (y - w_y) / float(w_h)) * m_curh + m_cury;

	return Vec3d(lat, lon, 0);
}

/* =============================================================================
 =============================================================================== */
void CLocater::setPosGPS(Vec3d pos)
{
	m_pos = pos;
}

/* =============================================================================
 =============================================================================== */
void CLocater::setZoom(double f)
{
	m_zoom += f;
	m_zoom = min(20.0f, max(1.0f, m_zoom));

	float	new_w = m_w / m_zoom;
	float	new_h = m_h / m_zoom;

	//  center area
	m_curx = min(max(m_curx + m_curw / 2 - new_w / 2, m_x), (m_w + m_x) - new_w);
	m_cury = min(max(m_cury + m_curh / 2 - new_h / 2, m_y), (m_h + m_y) - new_h);
	m_curw = new_w;
	m_curh = new_h;
}

/* =============================================================================
 =============================================================================== */
void CLocater::setTranslate(int x, int y)
{
	if (getState() != WIN_NONE)
	{
		setMove(x, y);
		return;
	}

	int w_x, w_y, w_w, w_h;
	getPos(w_x, w_y, w_w, w_h);
	m_curx += (float) x * m_curw / (float) w_w;
	m_cury -= (float) y * m_curh / (float) w_h;
	m_curx = min(max(m_curx, m_x), (m_w + m_x) - m_curw);
	m_cury = min(max(m_cury, m_y), (m_h + m_y) - m_curh);
}

/* =============================================================================
 =============================================================================== */
void CLocater::clearTexture()
{
	deleteTexture(m_tex);
	m_tex = -1;
}

/* =============================================================================
 =============================================================================== */
bool CLocater::loadTexture(string FileName, double x0, double y0, double x1, double y1)
{
	double	xLoc = x0;
	double	yLoc = y0;
	double	wLoc = x1 - x0;
	double	hLoc = y1 - y0;
	if (FileName == "" || !fileexists(FileName)) 
		FileName = g_Env.m_SystemPath + "/ui/world.jpg";

	//  store the texture and set up the variables
	m_path = FileName;
	clearTexture();

	m_x = xLoc;
	m_y = yLoc;
	m_w = wLoc;
	m_h = hLoc;
	m_curx = m_x;
	m_cury = m_y;
	m_curw = m_w;
	m_curh = m_h;
	m_zoom = 1.0;

	return true;
}

/* =============================================================================
 =============================================================================== */
void CLocater::draw()
{

	//  see if we can load texture
	if (m_tex == -1)
	{
		int w_tx, h_tx;
		int tmpID = createTexture(m_path, w_tx, h_tx);
		if (tmpID == -1)
			return;
		m_ratio = w_tx == 0.0 ? 1.0 : (float) h_tx / (float) w_tx;
		m_tex = tmpID;

		int win_w = w_tx > h_tx ? 320 : (320 * w_tx) / h_tx;
		int win_h = (int) (m_ratio * win_w);
		int win_x = getWidth() - win_w - 10;

		CWindow::setPos(win_x, -1, win_w, win_h);
	}

	int x, y, w, h;
	CWindow::getPos(x, y, w, h);
	h = (int) (m_ratio * w);
	CWindow::setPos(x, y, w, h);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, m_tex);

	float	fTxMinX = (m_curx - m_x) / m_w;
	float	fTxMaxX = fTxMinX + m_curw / m_w;
	float	fTxMaxY = 1 - (m_cury - m_y) / m_h;
	float	fTxMinY = fTxMaxY - m_curh / m_h;
	setTexWindow(fTxMinX, fTxMaxX, fTxMinY, fTxMaxY);
	CWindow::draw();

	//  figure out cursor position
	Vec3d	pt = m_pos;
	if (pt[1] < m_curx || pt[1] > m_curx + m_curw || pt[0] < m_cury || pt[0] > m_cury + m_curh) 
		return;

	pt[1] = ((pt[1] - m_curx) / m_curw) * w + x;
	pt[0] = ((pt[0] - m_cury) / m_curh) * h + getHeight() - (y + h);
	glColor4f(1, 0, 0, 1);
	glLineWidth(1);
	glBegin(GL_LINES);
	if (pt[0] >= getHeight() - (y + h) && pt[0] <= getHeight() - y)
	{
		glVertex2f(max(pt[1] - 10, (double) x), pt[0]);
		glVertex2f(min(pt[1] + 10, (double) (x + w)), pt[0]);
	}

	if (pt[1] >= x && pt[1] <= (x + w))
	{
		glVertex2f(pt[1], max(pt[0] - 10, (double) (getHeight() - (y + h))));
		glVertex2f(pt[1], min(pt[0] + 10, (double) (getHeight() - y)));
	}

	glEnd();
}

/* =============================================================================
    POSITION INDICATOR
 =============================================================================== */
void CDraw::drawPosition(Vec3f pt)
{
	if (!m_ValidPos)
		return;
	pt[1] = m_Height - pt[1];
	glLineWidth(1);
	glColor4f(1, 1, 1, 1);
	glBegin(GL_LINES);
	glVertex2f(pt[0] - 5, pt[1]);
	glVertex2f(pt[0] + 5, pt[1]);
	glVertex2f(pt[0], pt[1] - 5);
	glVertex2f(pt[0], pt[1] + 5);
	glEnd();
}

/* =============================================================================
    COMPASS
 =============================================================================== */
bool CCompass::in(int x, int y)
{
	if (!g_Set.m_ShowCompass)
		return false;
	if (m_Action != kActionNone)
		return true;

	Vec3d	vCenter = Vec3d(m_X + m_W / 2, getHeight() - m_Y + m_H / 2, 0);
	return len(m_Loc - vCenter) < 60;
}

/* =============================================================================
 =============================================================================== */
bool CCompass::getFocusChange(int x, int y)
{
	if (m_Action != kActionNone)
		return false;
	m_Loc = Vec3d(x, y, 0);

	Vec3d	vCenter = Vec3d(m_X + m_W / 2, getHeight() - m_Y + m_H / 2, 0);
	double	dist = len(m_Loc - vCenter);
	int		iState = COM_NONE;
	if (dist < 20)
		iState = COM_ZOOM;
	else if (dist < 40)
		iState = COM_TRANS;
	else if (dist < 60)
		iState = COM_ROT;

	if (iState == m_State)
		return false;
	m_State = iState;
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CCompass::getClick(int x, int y)
{
	getFocusChange(x, y);
	if (m_State == COM_ZOOM)
		m_Action = kActionZoom;
	else if (m_State == COM_TRANS)
		m_Action = kActionTranslate;
	else if (m_State == COM_ROT)
		m_Action = kActionRotate;
	else
		return false;
	return true;
}

/* =============================================================================
 =============================================================================== */
void CCompass::setMove(int x, int y)
{
	Vec3d	dir = Vec3d(x, y, 0) - m_Loc;
	m_Dir = norm(dir) * min(5.0, len(dir) / 20.0);
}

/* =============================================================================
 =============================================================================== */
void CCompass::endClick(int x, int y)
{
	m_Action = kActionNone;
}

/* =============================================================================
 =============================================================================== */
void CCompass::clear()
{
	for (int i = 0; i < COM_CNT_TX; i++)
	{
		deleteTexture(m_tx[i]);
		m_tx[i] = -1;
	}
}

/* =============================================================================
 =============================================================================== */
void CCompass::draw(bool bInit)
{
	static int	tx_w;
	if (m_tx[COM_TX] == -1)
	{
		string	strUIPath = g_Env.m_SystemPath + "/ui/";
		for (int i = 0; i < COM_CNT_TX; i++)
			m_tx[i] = createUITexture(strUIPath + m_file[i], m_W, m_H, tx_w);
	}

	m_X = getWidth() - m_W;
	m_Y = getBotOffset() + m_H;
	if (bInit)
		return;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1, 1, 1, 1);

	int		tx;
	bool	bZoom = false;
	switch(m_State)
	{
	case COM_NONE:	tx = m_tx[COM_TX]; break;
	case COM_ZOOM:	tx = m_tx[COM_ZOOM_TX]; bZoom = true; break;
	case COM_TRANS: tx = m_tx[COM_TRANS_TX]; break;
	case COM_ROT:	tx = m_tx[COM_ROT_TX]; break;
	}

	glPushMatrix();
	glTranslatef(m_X + m_W / 2, m_Y - m_H / 2, 0);
	glBindTexture(GL_TEXTURE_2D, bZoom ? m_tx[COM_ZOOM_OVER_TX] : m_tx[COM_ZOOM_TX]);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(-m_W / 2, -m_H / 2);
	glTexCoord2f(1, 1);
	glVertex2f(m_W / 2, -m_H / 2);
	glTexCoord2f(1, 0);
	glVertex2f(m_W / 2, m_H / 2);
	glTexCoord2f(0, 0);
	glVertex2f(-m_W / 2, m_H / 2);
	glEnd();

	if (bZoom) tx = m_tx[COM_TX];

	float	cam_dir = fmod(m_CamAzimuth, vl_pi * 2) * 180.0f / vl_pi;
	if (cam_dir < 0) cam_dir += 360.0f;

	glRotatef(-cam_dir, 0, 0, 1);
	glBindTexture(GL_TEXTURE_2D, tx);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(-m_W / 2, -m_H / 2);
	glTexCoord2f(1, 1);
	glVertex2f(m_W / 2, -m_H / 2);
	glTexCoord2f(1, 0);
	glVertex2f(m_W / 2, m_H / 2);
	glTexCoord2f(0, 0);
	glVertex2f(-m_W / 2, m_H / 2);
	glEnd();
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
}

/* =============================================================================
    LIGHTING CONTROL
 =============================================================================== */
bool CLighting::in(int x, int y)
{
	if (!g_Set.m_ShowLighting)
		return false;
	if (m_Action != LIGHT_NONE)
		return true;
	return len(Vec3d(x, y, 0) - m_Center) < 60;
}

/* =============================================================================
 =============================================================================== */
bool CLighting::getFocusChange(int x, int y)
{
	if (m_Action != LIGHT_NONE)
		return false;

	Vec3d	dir = Vec3d(x, y, 0) - m_Center;
	double	dist = len(dir);
	int		iState = LIGHT_NONE;
	if (dist < 25 && abs(dir[0]) < 8)
		iState = LIGHT_LEVEL;
	else if (dist < 45 && dir[1] <= 0)
		iState = LIGHT_ANGLE;
	else if (dist < 60)
		iState = LIGHT_DIR;

	if (iState == m_State)
		return false;
	m_State = iState;
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CLighting::getClick(int x, int y)
{
	getFocusChange(x, y);
	m_Action = m_State;
	if (m_State == LIGHT_NONE)
		return false;
	return true;
}

/* =============================================================================
    reset direction vector
 =============================================================================== */
void CLighting::setMove(int x, int y)
{
	float	fVal;
	Vec3d	dir = Vec3d(x, y, 0) - m_Center;
	switch(m_Action)
	{
	case LIGHT_DIR:
		fVal = 90.0f - atan2(dir[1], dir[0]) * 180 / vl_pi;
		if (fVal < 0) fVal += 360;
		setDir(fVal);
		break;

	case LIGHT_ANGLE:
		if (dir[1] > 0)
			break;
		fVal = -atan2(dir[1], dir[0]) * 180 / vl_pi;
		setAngle(fVal);
		break;

	case LIGHT_LEVEL:
		fVal = LIGHT_LEVEL_MAX - (dir[1] + LIGHT_LEVEL_OFF) / LIGHT_LEVEL_STEP;
		fVal = max(1.0f, min(LIGHT_LEVEL_MAX, fVal));
		g_World.setShading(fVal);
		break;
	}
}

/* =============================================================================
 =============================================================================== */
void CLighting::endClick(int x, int y)
{
	setMove(x, y);
	m_Action = LIGHT_NONE;
}

/* =============================================================================
 =============================================================================== */
void CLighting::clear()
{
	for (int i = 0; i < LIGHT_CNT_TX; i++)
	{
		deleteTexture(m_tx[i]);
		m_tx[i] = -1;
	}
}

/* =============================================================================
 =============================================================================== */
void CLighting::draw(bool bInit)
{
	static int	tx_w;
	if (m_tx[LIGHT_DIR_TX] == -1)
	{
		string	strUIPath = g_Env.m_SystemPath + "/ui/";
		for (int i = 0; i < LIGHT_CNT_TX; i++) 
			m_tx[i] = createUITexture(strUIPath + m_file[i], m_W, m_H, tx_w);
	}

	m_X = getWidth() - m_W - (g_Set.m_ShowCompass ? m_W + 20 : 0);
	m_Y = getBotOffset() + m_H;
	m_Center = Vec3d(m_X + m_W / 2, getHeight() - m_Y + m_H / 2, 0);
	if (bInit)
		return;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1, 1, 1, 1);

	glPushMatrix();
	glTranslatef(m_X + m_W / 2, m_Y - m_H / 2, 0);
	for (int i = 0; i < LIGHT_CNT_TX; i += 2)
	{
		int tx = i;

		glPushMatrix();
		switch(tx)
		{
		case LIGHT_DIR_SET_TX:		glRotatef(getDir() - 180.0f, 0, 0, 1);
		case LIGHT_DIR_TX:			if (m_State == LIGHT_DIR) tx++;	 //  highlight
			break;
		case LIGHT_ANGLE_SET_TX:	glRotatef(270.0 + getAngle(), 0, 0, 1);
		case LIGHT_ANGLE_TX:		if (m_State == LIGHT_ANGLE) tx++;   //  highlight
			break;
		case LIGHT_LEVEL_SET_TX:	glTranslatef(0, ((g_World.getShading() * LIGHT_LEVEL_STEP) - LIGHT_LEVEL_OFF), 0);
		case LIGHT_LEVEL_TX:		if (m_State == LIGHT_LEVEL) tx++;   //  highlight
			break;
		}

		glBindTexture(GL_TEXTURE_2D, m_tx[tx]);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(-m_W / 2, -m_H / 2);
		glTexCoord2f(1, 1);
		glVertex2f(m_W / 2, -m_H / 2);
		glTexCoord2f(1, 0);
		glVertex2f(m_W / 2, m_H / 2);
		glTexCoord2f(0, 0);
		glVertex2f(-m_W / 2, m_H / 2);
		glEnd();
		glPopMatrix();
	}

	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
}

//  TIMELINE CONTROL
static int	TIME_BK_W = 512;
static int	TIME_BK_H = 128;
static int	TIME_BAR_X = 27;
static int	TIME_BAR_W = 443;
static int	g_btn_tx_x[TIME_DRAW_CNT] =
{
	0,
	33,
	23,
	27,
	22,
	21,
	21,
	25,
	471,
	74,
	109,
	140,
	171,
	202,
	233,
	264,
	301,
	338,
	385
};
static int	g_btn_tx_y[TIME_DRAW_CNT] = { 0, 84, 84, 84, 75, 60, 50, 96, 96, 10, 10, 10, 10, 10, 10, 10, 12, 12, 12 };
struct
{
	int x, y, w, h;
}
t_rect[] =
{
	{ 0, 0, TIME_BK_W, TIME_BK_H },
	{ -1, -1, -1, -1 },
	{ -1, 92, 10, 8 },
	{ -1, 92, 10, 8 },
	{ -1, 82, 12, 12 },
	{ -1, 67, 8, 8 },
	{ -1, 57, 8, 8 },
	{ 27, 102, 12, 8 },
	{ 471, 102, 12, 8 },
	{ 79, 22, 29, 18 },
	{ 109, 22, 30, 18 },
	{ 140, 22, 30, 18 },
	{ 171, 22, 30, 18 },
	{ 202, 22, 30, 18 },
	{ 233, 22, 30, 18 },
	{ 264, 22, 30, 18 },
	{ 307, 22, 29, 18 },
	{ 337, 22, 46, 18 },
	{ 384, 22, 45, 18 },
	{ 31, 83, TIME_BAR_W, 10 },
	{ 31, 68, TIME_BAR_W, 9 },
	{ 31, 58, TIME_BAR_W, 9 },
};

/* =============================================================================
 =============================================================================== */

inline bool in_time_rect(int x, int y, int i)
{
	return(x >= t_rect[i].x && x <= t_rect[i].x + t_rect[i].w && y >= t_rect[i].y && y <= t_rect[i].y + t_rect[i].h);
}

/* =============================================================================
 =============================================================================== */
void CTimeline::update_time_rect()
{
	double	start = g_World.getTimeLine().getStart();
	double	finish = g_World.getTimeLine().getFinish();
	double	duration = finish - start;
	double	time = g_World.getTimeLine().getTime() + g_World.getTimeLine().getFrameTime();
	double	lead = g_World.getTimeLine().getLead();
	double	trail = g_World.getTimeLine().getTrail();
	double	selStart = g_World.getTimeLine().getSelStart();
	double	selFinish = g_World.getTimeLine().getSelFinish();

	double	curLead = (lead == -1 || (time - lead < selStart)) ? selStart : time - lead;
	double	curTrail = (trail == -1 || (time + trail > selFinish)) ? selFinish : time + trail;

	t_rect[TIME_TIME].x = TIME_BAR_X + TIME_BAR_W * (time - start) / duration;
	t_rect[TIME_LEAD].x = TIME_BAR_X + TIME_BAR_W * (curLead - start) / duration + 2;
	t_rect[TIME_TRAIL].x = TIME_BAR_X + TIME_BAR_W * (curTrail - start) / duration + 2;
	t_rect[TIME_LOOP_LEFT].x = TIME_BAR_X + TIME_BAR_W * (selStart - start) / duration;
	t_rect[TIME_LOOP_RIGHT].x = TIME_BAR_X + TIME_BAR_W * (selFinish - start) / duration + 2;
}

/* =============================================================================
 =============================================================================== */
void CTimeline::updatePos()
{
	if (m_X + TIME_BK_W - 15 > getWidth()) m_X = getWidth() - TIME_BK_W + 15;
	if (m_X < -10) m_X = -10;
	if (m_Y + TIME_BK_H + getTopOffset() - 8 > getHeight()) 
		m_Y = getHeight() - getTopOffset() - TIME_BK_H + 8;
	if (m_Y - getBotOffset() < -10) m_Y = getBotOffset() - 10;
}

/* =============================================================================
 =============================================================================== */
bool CTimeline::in(int x, int y)
{
	if (!g_Set.m_ShowTimeline)
		return false;
	if (m_Action != TIME_NONE)
		return true;
	return in_time_rect(x - m_X, getHeight() - y - m_Y, 0);
}

/* =============================================================================
 =============================================================================== */
bool CTimeline::getFocusChange(int x, int y)
{
	if (m_Action != TIME_NONE)
		return false;
	x = x - m_X;
	y = getHeight() - y - m_Y;

	int iState = TIME_NONE;
	if (in_time_rect(x, y, TIME_BACK))
	{
		update_time_rect();
		for (int i = 1; i < TIME_STATE_CNT && iState == TIME_NONE; i++)
			if (in_time_rect(x, y, i)) iState = i;
		if (iState == TIME_NONE) iState = TIME_BACK;
	}

	m_State = iState;
	if (iState == TIME_NONE)
		return false;
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CTimeline::getClick(int x, int y)
{
	getFocusChange(x, y);
	m_Action = m_State;
	if (m_State == TIME_NONE)
		return false;
	m_click_off_x = x - m_X;
	m_click_off_y = getHeight() - y - m_Y;
	return true;
}

/* =============================================================================
 =============================================================================== */
double CTimeline::get_time_from_pos(int x, bool bSel)
{
	double	start = g_World.getTimeLine().getStart();
	double	finish = g_World.getTimeLine().getFinish();
	double	duration = finish - start;
	x -= 6;
	x = max(0, min(TIME_BAR_W, x - (TIME_BAR_X + m_X)));

	double	dtime = start + (duration * x) / TIME_BAR_W;
	if (bSel)
	{
		start = g_World.getTimeLine().getSelStart();
		finish = g_World.getTimeLine().getSelFinish();
	}

	return max(start, min(finish, dtime));
}

/* =============================================================================
 =============================================================================== */
void CTimeline::setMove(int x, int y)
{
	double		dtime;
	CTimeLine	&tmln = g_World.getTimeLine();
	switch(m_Action)
	{
	case TIME_BACK: //  move
		m_X = x - m_click_off_x;
		m_Y = getHeight() - (y + m_click_off_y);
		updatePos();
		break;

	case TIME_TIME:
		dtime = get_time_from_pos(x, true);
		tmln.setCurTime(dtime);
		break;

	case TIME_LEAD:
		dtime = max(0.0, tmln.getTime() - get_time_from_pos(x, true));
		tmln.setLead(dtime);
		g_World.getDataSet().setLead(dtime);
		break;

	case TIME_TRAIL:
		dtime = max(0.0, get_time_from_pos(x, true) - tmln.getTime());
		tmln.setTrail(dtime);
		g_World.getDataSet().setTrail(dtime);
		break;

	case TIME_LOOP_LEFT:
		dtime = max(tmln.getStart(), min(tmln.getSelFinish(), get_time_from_pos(x, false)));
		if (tmln.getTime() < dtime) tmln.setCurTime(dtime);
		tmln.setSelStart(dtime);
		g_World.getDataSet().setSelStart(dtime);
		break;

	case TIME_LOOP_RIGHT:
		dtime = max(tmln.getSelStart(), min(tmln.getFinish(), get_time_from_pos(x, false)));
		if (tmln.getTime() > dtime) tmln.setCurTime(dtime);
		tmln.setSelFinish(dtime);
		g_World.getDataSet().setSelFinish(dtime);
		break;
	}
}

/* =============================================================================
 =============================================================================== */
double stepTime(double start, double finish, double tm, bool bForward)
{
	double	span = finish - start;
	if (span == 0)
		return tm;

	int		step = 0;
	double	dSpan[] =
	{
		DUR_DAY * 2,
		DUR_HOUR * 6,
		DUR_HOUR * 2,
		DUR_MIN * 30,
		DUR_MIN * 5,
		DUR_MIN * 2,
		DUR_SEC * 30,
		DUR_SEC,
		0
	};
	double	dStep[] =
	{
		DUR_HOUR * 6,
		DUR_HOUR,
		DUR_MIN * 15,
		DUR_MIN * 5,
		DUR_MIN,
		DUR_SEC * 15,
		DUR_SEC * 5,
		DUR_SEC,
		0
	};
	if (span > DUR_WEEK)
		step = DUR_DAY * (max(1, (int(span / DUR_DAY) / 15)));
	else
		for (int i = 0; i < dSpan[i] != 0.0 && step == 0; i++)
			if (span > dSpan[i]) 
				step = dStep[i];

	double	newtm = bForward ? (((int) tm / step) + 1) * step : (((int) tm - 1) / step) * step;
	return min(finish, max(start, newtm));
}

/* =============================================================================
 =============================================================================== */
void CTimeline::endClick(int x, int y)
{
	getFocusChange(x, y);
	if (m_Action != m_State)
		return;

	bool		bActive;
	double		dtime;
	CTimeLine	&tmln = g_World.getTimeLine();
	switch(m_Action)
	{
	case TIME_END1:
		if (getUICallback() != NULL) (*getUICallback()) (1);
		break;

	case TIME_END2:
		if (getUICallback() != NULL) (*getUICallback()) (2);
		break;

	case TIME_BEGIN_BTN:
		tmln.setCurTime(tmln.getStart());
		break;

	case TIME_RR_BTN:
		if (tmln.getPlaying())
			tmln.setSlower();
		else
			tmln.setCurTime(stepTime(tmln.getStart(), tmln.getFinish(), tmln.getTime(), false));
		break;

	case TIME_REV_BTN:
		tmln.setPlay(true, true);
		break;

	case TIME_STOP_BTN:
		tmln.setPlay(false);
		break;

	case TIME_FWD_BTN:
		tmln.setPlay(true);
		break;

	case TIME_FF_BTN:
		if (tmln.getPlaying())
			tmln.setFaster();
		else
			tmln.setCurTime(stepTime(tmln.getStart(), tmln.getFinish(), tmln.getTime(), true));
		break;

	case TIME_END_BTN:
		tmln.setCurTime(tmln.getFinish());
		break;

	case TIME_LOOP_BTN:
		tmln.setLoop(!tmln.getLoop());
		break;

	case TIME_LEAD_BTN:
		bActive = tmln.getLead() == -1;
		tmln.setLead(bActive ? 0 : -1);
		g_World.getDataSet().setLead(bActive ? 0 : -1);
		break;

	case TIME_TRAIL_BTN:
		bActive = tmln.getTrail() == -1;
		tmln.setTrail(bActive ? 0 : -1);
		g_World.getDataSet().setTrail(bActive ? 0 : -1);
		break;

	case TIME_TIME_SET:
		dtime = get_time_from_pos(x, true);
		tmln.setCurTime(dtime);
		break;

	case TIME_LEAD_SET:
		if (tmln.getLead() != NO_TIME)
		{
			dtime = max(0.0, tmln.getTime() - get_time_from_pos(x, true));
			tmln.setLead(dtime);
			g_World.getDataSet().setLead(dtime);
		}
		break;

	case TIME_TRAIL_SET:
		if (tmln.getTrail() != NO_TIME)
		{
			dtime = max(0.0, get_time_from_pos(x, true) - tmln.getTime());
			tmln.setTrail(dtime);
			g_World.getDataSet().setTrail(dtime);
		}
		break;
	}

	m_Action = TIME_NONE;
}

/* =============================================================================
 =============================================================================== */
void CTimeline::clear()
{
	for (int i = 0; i < TIME_TX_CNT; i++)
	{
		deleteTexture(m_tx[i]);
		m_tx[i] = -1;
	}
}

/* =============================================================================
 =============================================================================== */
void CTimeline::draw(bool bInit)
{
	static int	tx_w;
	if (m_tx[TIME_BACK] == -1)
	{
		int		w, h;
		string	strUIPath = g_Env.m_SystemPath + "/ui/";
		m_tx[0] = createUITexture(strUIPath + m_file[0], w, h, tx_w);
		for (int i = 1; i < TIME_TX_CNT; i++) 
			m_tx[i] = createUITexture(strUIPath + m_file[i], w, h, tx_w);
	}

	if (bInit)
		return;
	if (m_X == MAX_INT)
	{
		m_X = (getWidth() - TIME_BK_W) / 2;
		m_Y = getBotOffset() - 10;
	}

	update_time_rect();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1, 1, 1, 1);

	glPushMatrix();
	glTranslatef(m_X, m_Y, 0);
	for (int i = 0, ndx = 0; i < TIME_DRAW_CNT; i++)
	{
		int tx = ndx;
		int x = g_btn_tx_x[i];
		int y = g_btn_tx_y[i];
		int w = m_W;
		int h = m_H;

		glPushMatrix();
		if (i == TIME_BACK)
		{
			w = TIME_BK_W;
			h = TIME_BK_H;
			ndx++;
		}
		else if (i <= TIME_LOOP_RIGHT)
		{
			int selStart = t_rect[TIME_LOOP_LEFT].x - TIME_BAR_X;
			int selEnd = t_rect[TIME_LOOP_RIGHT].x - TIME_BAR_X;
			if (i == TIME_LOOP_LEFT) x += selStart;
			if (i == TIME_LOOP_RIGHT) x += selEnd;
			if (i == TIME_LOOP_BAR)
			{
				x += selStart;
				w = (selEnd - selStart) - 6;
			}

			ndx++;
		}
		else if (i <= TIME_END2)
		{
			if (i == TIME_TIME)
			{
				x += t_rect[TIME_TIME].x - TIME_BAR_X;
				tx++;
			}
			else if (i == TIME_LEAD)
			{
				x += t_rect[TIME_LEAD].x - TIME_BAR_X;
				if (g_World.getTimeLine().getLead() != -1) tx++;
			}
			else if (i == TIME_TRAIL)
			{
				x += t_rect[TIME_TRAIL].x - TIME_BAR_X;
				if (g_World.getTimeLine().getTrail() != -1) tx++;
			}

			ndx += 2;
		}
		else
		{
			if (m_Action == i)
				tx++;
			else if (i == TIME_REV_BTN)
			{
				if (g_World.getTimeLine().getPlaying() && g_World.getTimeLine().getReverse())
					tx += 2;
			}
			else if (i == TIME_STOP_BTN)
			{
				if (!g_World.getTimeLine().getPlaying()) tx += 2;
			}
			else if (i == TIME_FWD_BTN)
			{
				if (g_World.getTimeLine().getPlaying() && !g_World.getTimeLine().getReverse()) 
					tx += 2;
			}
			else if (i == TIME_LOOP_BTN)
			{
				if (g_World.getTimeLine().getLoop()) tx += 2;
			}
			else if (i == TIME_LEAD_BTN)
			{
				if (g_World.getTimeLine().getLead() != NO_TIME) tx += 2;
			}
			else if (i == TIME_TRAIL_BTN)
			{
				if (g_World.getTimeLine().getTrail() != NO_TIME) tx += 2;
			}

			ndx += 3;
		}

		glBindTexture(GL_TEXTURE_2D, m_tx[tx]);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(x, y);
		glTexCoord2f(1, 1);
		glVertex2f(x + w, y);
		glTexCoord2f(1, 0);
		glVertex2f(x + w, y + h);
		glTexCoord2f(0, 0);
		glVertex2f(x, y + h);
		glEnd();
		glPopMatrix();
	}

	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
}

/* =============================================================================
 =============================================================================== */
void CDraw::drawTimelineText()
{
	glPushMatrix();
	glTranslatef(getTimeline().getX(), getTimeline().getY(), 0);

	string	strTime = g_World.getTimeLine().getTimeString(g_World.getTimeLine().getTime());
	glDrawText(strTime, CLR_WHITE, 210, 97);

	if (getTimeline().getLoopLeft() || getTimeline().getLoopRight())
	{
		bool	bStart = getTimeline().getLoopLeft();
		double	dtime = bStart ? g_World.getTimeLine().getSelStart() : g_World.getTimeLine().getSelFinish();
		string	strTime = g_World.getTimeLine().getTimeString(dtime);
		glDrawText(strTime, CLR_OFFWHITE, bStart ? 35 : TIME_BK_W - 135, 70);
	}

	glPopMatrix();
}

/* =============================================================================
    Watermark and Timestamp
 =============================================================================== */
void CDraw::drawTimeStamp()
{
	color	clr = 0xffcccccc;
	string	strTime;

	if (getCameraTracking() || getValidPos())
	{
		Vec3d	pos = getCameraTracking() ? getDataCamPos() : getTerrainPos();
		char	buf[256];
		sprintf(buf, "%s  %.0lf m    ", getStringFromPosition(pos, false).c_str(), pos[2]);
		strTime = buf;
	}

	strTime += g_World.getTimeLine().getTimeString(g_World.getTimeLine().getTime());
	glDrawText(strTime, clr, getWidth() - 550, 2);
}

/* =============================================================================
 =============================================================================== */
void CDraw::clearWatermark()
{
	deleteTexture(m_WaterMark_tx);
	m_WaterMark_tx = -1;
}

/* =============================================================================
 =============================================================================== */
void CDraw::initWatermark()
{
	int w, h;
	if (m_WaterMark_tx == -1) 
		m_WaterMark_tx = createTexture(g_Env.m_SystemPath + "/ui/Watermark.png", w, h);
}

#define WATERMARK_SZ	64

/* =============================================================================
 =============================================================================== */

void CDraw::drawWatermark()
{
	if (m_WaterMark_tx == -1)
		return;
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, m_WaterMark_tx);

	glPushMatrix();

	int x = 8;
	int y = getHeight() - (WATERMARK_SZ + 8);
	int w = WATERMARK_SZ, h = WATERMARK_SZ;
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(x, y);
	glTexCoord2f(1, 1);
	glVertex2f(x + w, y);
	glTexCoord2f(1, 0);
	glVertex2f(x + w, y + h);
	glTexCoord2f(0, 0);
	glVertex2f(x, y + h);
	glEnd();
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

//  g_World Screen Images
int		g_ScreenTex = -1;
string	g_ScreenTexPath;
double	g_ScreenDelay = 0.0;
double	g_ScreenFadeTot = 0.0;
double	g_ScreenFadeCur = 0.0;

/* =============================================================================
 =============================================================================== */

void CDraw::initFadeTimer()
{

	//  grab screen image
	GLint	vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	int				ww = vp[2];
	int				hh = vp[3];
	unsigned char	*pData = NULL;
	if (!mem_alloc(pData, 4 * ww * hh))
		return;
	drawGL();
	glReadBuffer(GL_BACK);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, ww, hh, GL_RGBA, GL_UNSIGNED_BYTE, pData);
	deleteTexture(g_ScreenTex);
	g_ScreenTex = createTexture(pData, ww, hh, GL_RGBA, GL_RGBA, 0);
	delete pData;
}

/* =============================================================================
 =============================================================================== */
void CDraw::startFadeTimer(double fTime)
{
	g_Timers.startTimer(FADE_TIMER);
	g_ScreenFadeTot = fTime;
	g_ScreenFadeCur = -1.0;
	g_ScreenDelay = -1.0;
}

/* =============================================================================
 =============================================================================== */
void CDraw::setFadeTimer(double fTime)
{
	g_ScreenDelay = 0.0;
	g_ScreenFadeCur = fTime;
}

/* =============================================================================
 =============================================================================== */
void CDraw::exitFadeTimer()
{
	deleteTexture(g_ScreenTex);
	g_ScreenTex = -1;
}

/* =============================================================================
 =============================================================================== */
void CDraw::drawFadeImage()
{
	if (g_ScreenTex == -1)
		return;

	double	time = g_ScreenFadeCur < 0.0 ? g_Timers.pollTimer(FADE_TIMER) : g_ScreenFadeCur;
	if (g_ScreenDelay < 0.0) g_ScreenDelay = time;

	double	dImageTime = time - g_ScreenDelay;
	double	dTimeOut = g_ScreenFadeTot;
	if (dImageTime > dTimeOut + .5 || g_ScreenTex == -1) //  image done fading
		return;

	g_Set.m_NeedRedraw = true;
	dImageTime = min(dImageTime, dTimeOut);

	double	dTrans = 1.0 - dImageTime / dTimeOut;		//  fade

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1, 1, 1, dTrans);
	glBindTexture(GL_TEXTURE_2D, g_ScreenTex);

	int w = getWidth();
	int h = getHeight();
	int x = 0;
	int y = 0;
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x, y);
	glTexCoord2f(1, 0);
	glVertex2f(x + w, y);
	glTexCoord2f(1, 1);
	glVertex2f(x + w, y + h);
	glTexCoord2f(0, 1);
	glVertex2f(x, y + h);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glColor4f(1, 1, 1, 1);
}

/* =============================================================================
 =============================================================================== */
void CDraw::drawScreenImages()
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	for (int i = 0; i < g_World.getTextureSet().getTextureLayerCnt(); i++)
	{
		CTextureLayer	&Texture = g_World.getTextureLayer(i);
		if (!Texture.getActive() || !Texture.getIsScreen())
			continue;

		int iTxID = Texture.getScreenTex();
		if (iTxID == -1)
		{
			int		w, h, txw;
			string	strLocal = cacheWebFile(Texture.getLocalFilePath());
			iTxID = createUITexture(strLocal, w, h, txw);
			if (iTxID == -1)
			{
				cout << ("Unable to create texture for " + Texture.getName());
				continue;
			}

			Texture.setScreenTex(iTxID);
			Texture.setScreenW(w);
			Texture.setScreenH(h);
			Texture.setLocalFilePath(strLocal);
		}

		glBindTexture(GL_TEXTURE_2D, iTxID);

		glColor4f(1, 1, 1, Texture.getTransparency());

		Vec3d	minpos = Texture.getMinPos();
		Vec3d	maxpos = Texture.getMaxPos();

		//  position is based on percentage of the screen size
		if (Texture.getScreenType() == SCREEN_PERCENT)
		{
			minpos[1] *= (double) getWidth() / 100.0;
			maxpos[1] *= (double) getWidth() / 100.0;
			minpos[0] *= (double) getHeight() / 100.0;
			maxpos[0] *= (double) getHeight() / 100.0;
		}

		//  negative entries are from the max height or width
		if (minpos[1] < 0) minpos[1] += getWidth();
		if (maxpos[1] < 0) minpos[1] += getWidth();
		if (minpos[0] < 0) minpos[0] += getHeight();
		if (maxpos[0] < 0) minpos[0] += getHeight();

		// if bottom and right are filled in then use them to set width of image
		int w = maxpos[1] ? maxpos[1] : Texture.getScreenW();
		int h = maxpos[0] ? maxpos[0] : Texture.getScreenH();

		//  set position, center on x, y
		int x = minpos[1] - w / 2;
		int y = getHeight() - (minpos[0] - h / 2);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(x, y - h);
		glTexCoord2f(1, 1);
		glVertex2f(x + w, y - h);
		glTexCoord2f(1, 0);
		glVertex2f(x + w, y);
		glTexCoord2f(0, 0);
		glVertex2f(x, y);
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);
	glColor4f(1, 1, 1, 1);
}

/* =============================================================================
    LEGEND
 =============================================================================== */
void d_getMinMaxText(CDataLayer *pDataLayer, string &strMin, string &strMax)
{
	char	buf[32];
	float	fmin = pDataLayer->getCurVarMin();
	float	fmax = pDataLayer->getCurVarMax();
	if (pDataLayer->getShowParticles()
		&&	pDataLayer->getParticleDataLoaded()
		&&	pDataLayer->getParticleData().getVarCnt() >= 4)
	{
		fmin = pDataLayer->getParticleData().getVarMin(4);
		fmax = pDataLayer->getParticleData().getVarMax(4);
	}

	fmin = pDataLayer->getUseDisplayMinVal() ? pDataLayer->getDisplayMinVal() : fmin;
	fmax = pDataLayer->getUseDisplayMaxVal() ? pDataLayer->getDisplayMaxVal() : fmax;

	float	range = fmax - fmin;
	string	strFormat = range < .01 || range > 10000 ? "%1.2e" : "%.2f";
	sprintf(buf, strFormat.c_str(), fmin);
	strMin = buf;
	sprintf(buf, strFormat.c_str(), fmax);
	strMax = buf;
}

/* =============================================================================
 =============================================================================== */
int CDraw::getLegendSelDataset()
{
	return m_LegendSelDataset;
}

/* =============================================================================
 =============================================================================== */
vector<int> CDraw::getLegendList()
{
	vector<int> ndxLegend;
	int			h = min(g_Set.m_LegendHeight, getHeight() - getBotOffset() - getTopOffset() - 42);
	int			h_tot = h + 42;
	int			iGrad = g_Set.m_ShowTrnLegend ? g_World.getTextureSet().getCurGradIndex() : -1;
	if (iGrad != -1) 
		ndxLegend.push_back(-1 - iGrad);
	for (int i = 0; i < g_World.getDataSet().getDataLayerCnt(); i++)
	{
		if ((getHeight() - getBotOffset() - getTopOffset()) < (ndxLegend.size() + 1) * h_tot) 
			break;
		if (g_World.getDataLayer(i).getActive() && g_World.getDataLayer(i).getGradientLegend()) 
			ndxLegend.push_back(i);
	}

	return ndxLegend;
}

/* =============================================================================
 =============================================================================== */
bool CDraw::getLegendClick(int x, int y)
{
	vector<int> ndxLegend = getLegendList();
	if (ndxLegend.size() == 0)
		return false;

	int h = min(g_Set.m_LegendHeight, getHeight() - getBotOffset() - getTopOffset() - 42);
	int h_tot = h + 42;
	m_LegendSelDataset = -1;
	for (int i = 0, bot = getRealBot(); i < ndxLegend.size(); i++, bot -= h_tot)
		if (x >= 0 && x <= 40 && y >= bot - 160 && y <= bot - 10)
		{
			m_LegendSelDataset = ndxLegend[i];
			return true;
		}

	return false;
}

/* =============================================================================
 =============================================================================== */
void CDraw::drawLegend()
{
	vector<int> ndxLegend = getLegendList();
	if (ndxLegend.size() == 0)
		return;

	glPushMatrix();

	int w = 24;
	int h = min(g_Set.m_LegendHeight, getHeight() - getBotOffset() - getTopOffset() - 42);
	int h_tot = h + 42;

	glTranslatef(10, getBotOffset() + 30, 0);

	color	clr = CLR_WHITE;
	glColor4f(1, 1, 1, 1);

	for (int i = 0; i < ndxLegend.size(); i++)
	{
		int		iCurNdx = ndxLegend[i];
		bool	bData = (iCurNdx >= 0);

		int		txID = -1;
		string	strCaption;
		if (bData)
		{
			CDataLayer	&DataLayer = g_World.getDataLayer(iCurNdx);
			txID = DataLayer.getGradient().getTexID();

			int iVar = DataLayer.getCurVar();
			strCaption = "Data: " + (iVar == -1 ? "None" : DataLayer.getData().getVarName(iVar));
		}
		else
		{
			iCurNdx = (-1 - iCurNdx);
			txID = g_World.getTextureLayer(iCurNdx).getGradient().getTexID();
			strCaption = "Terrain: " + g_World.getTextureLayer(iCurNdx).getName();
		}

		if (txID == -1)
			continue;

		glBindTexture(GL_TEXTURE_2D, txID);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBegin(GL_QUADS);
		glTexCoord1f(0);
		glVertex2f(0, 0);
		glTexCoord1f(0);
		glVertex2f(w, 0);
		glTexCoord1f(1);
		glVertex2f(w, h);
		glTexCoord1f(1);
		glVertex2f(0, h);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		if (bData)
		{
			CDataLayer	&DataLayer = g_World.getDataLayer(iCurNdx);
			string		strMin, strMax;
			d_getMinMaxText(&DataLayer, strMin, strMax);
			glDrawText(strMax, clr, w, h - 6);
			glDrawText(strMin, clr, w, -6);
		}
		else
		{
			const CGradient	&Grad = g_World.getTextureLayer(iCurNdx).getGradient();

			//  draw text
			for (int j = 0; j < Grad.getMapCnt(); j++)
			{
				char	buf[1024];
				sprintf(buf, " %.0lf", Grad.getMapDepth(j));

				int yOff = Grad.getMapVal(j) * h;
				glDrawText(buf, clr, w, yOff - 6);
			}
		}

		glDrawText(strCaption, clr, -5, -20);
		glTranslatef(0, h_tot, 0);
	}

	glPopMatrix();
}

/* =============================================================================
    [ draw Screen Windows ]
 =============================================================================== */
void CDraw::initWindows_cmdln()
{
	initText();
	initWatermark();
	editCompass().draw(true);
	g_Set.m_ShowLighting = false;
	g_Set.m_ShowTimeline = false;
	g_Set.m_ShowVB = false;
	g_Set.m_ShowDB = false;
	g_Set.m_ShowLocater = false;
	resetScreenBounds();
}

/* =============================================================================
 =============================================================================== */
void CDraw::clearWindows_cmdln()
{
	clearWatermark();
	editCompass().clear();
	delText();
}

/* =============================================================================
 =============================================================================== */
void CDraw::initWindows(LPTimeLineUICallback pTimeFunc)
{
	resetScreenBounds();
	initText();
	initWatermark();
	editCompass().draw(true);
	editLighting().draw(true);
	editTimeline().draw(true);
	editTimeline().setUICallback(pTimeFunc);
	getLocater().loadTexture("", -180, -90, 180, 90);
}

/* =============================================================================
 =============================================================================== */
void CDraw::clearWindows()
{
	clearWatermark();
	editCompass().clear();
	editLighting().clear();
	editTimeline().clear();
	getLocater().clearTexture();
	delText();
}

/* =============================================================================
 =============================================================================== */
void CDraw::enterOrtho2D()
{
	glMatrixMode(GL_PROJECTION);	//  Set our matrix to our projection matrix
	glPushMatrix();		//  Push on a new matrix to work with
	glLoadIdentity();	//  reset the matrix
	glMatrixMode(GL_MODELVIEW); //  Set our matrix to our model view matrix
	glPushMatrix();		//  Push on a new matrix to work with
	glLoadIdentity();	//  Reset that matrix
	glOrtho(0, m_Width, 0, m_Height, -1, 1);
}

/* =============================================================================
 =============================================================================== */
void CDraw::exitOrtho2D()
{
	glPopMatrix();	//  Pop the current modelview matrix off the stack
	glMatrixMode(GL_PROJECTION);	//  Go back into projection mode
	glPopMatrix();	//  Pop the projection matrix off the stack
	glMatrixMode(GL_MODELVIEW); //  Set our matrix to our model view matrix
}

/* =============================================================================
 =============================================================================== */
void CDraw::drawWindows()
{
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	Vec3f	screen_pt = get2DPointGPS(m_TerrainPos);

	//  set up in ortho mode
	enterOrtho2D();

	drawScreenImages();

	if (g_Set.m_ShowLocater)
	{
		getLocater().setPosGPS(getLookAtGPS());
		getLocater().draw();
	}

	if (g_Set.m_ShowPosition)
		drawPosition(screen_pt);

	drawLegend();

	if (g_Set.m_ShowCompass)
	{
		editCompass().setCamAzimuth(getCamera().getAzimuth());
		editCompass().draw();
	}

	if (g_Set.m_ShowLighting) 
		editLighting().draw();

	if (g_Set.m_ShowTimeline)
	{
		editTimeline().draw();
		drawTimelineText();
	}

	if (g_Set.m_Printing)
	{
		if (g_Set.m_TimeStamp) drawTimeStamp();
		if (g_Set.m_WaterMark) drawWatermark();
	}

	if (g_Set.m_Presenting || g_Set.m_Printing) 
		drawFadeImage();

	exitOrtho2D();
}
