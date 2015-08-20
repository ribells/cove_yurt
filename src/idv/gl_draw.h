#pragma once

#include <vector>
#include "scene/camera.h"
#include "scene/image.h"
#include "const.h"

bool intersectLinePlane( Vec3d LiP0, Vec3d LiP1, Vec3d PnV0, Vec3d PnN, Vec3d &I );
bool intersectSphere( Vec3d& localRayStart, Vec3d& localRayEnd, double radius, Vec3d* localIsect, double* dist );
void getSceneRay( int x, int y, Vec3d &rayStart, Vec3d &rayEnd );
bool getSphereIntersect( int x, int y, Vec3d Center, double radius, Vec3d & pnt);
bool getPlaneIntersect( int x, int y, Vec3d *verts, Vec3d & pnt);

enum {SEL_TRN, SEL_DATA, SEL_OBJ, SEL_LINE, SEL_MAX};
enum {WIN_NONE, WIN_POS, WIN_SIZE};
enum {CAM_TRACK, CAM_FOLLOW, CAM_MAX};
enum {ANGLE_TOP, ANGLE_LEFT, ANGLE_RIGHT, ANGLE_BACK, ANGLE_ZOOMIN, ANGLE_ZOOMOUT, ANGLE_MAX};

class CWindow
{
private:
	int m_Height, m_Width, m_BotOffset, m_TopOffset;
	int	m_x, m_y, m_w, m_h;
	int	m_state;
	bool m_InWindow;
	float m_TxMinX;
	float m_TxMaxX;
	float m_TxMinY;
	float m_TxMaxY;
	bool m_Show;

public:
	CWindow() : m_Height(0), m_Width(0), m_BotOffset(0), m_TopOffset(0), m_x(0), m_y(0), m_w(0), m_h(0),
		m_state(WIN_NONE), m_InWindow(false), m_TxMinX(0.0), m_TxMinY(0.0), m_TxMaxX(1.0), m_TxMaxY(1.0), m_Show(true)
	{} 

	bool in(int x, int y);
	bool getClick(int x, int y);
	void endClick(int x, int y);
	bool setMove(int x, int y);
	void draw() const;

	int getState() const { return m_state; }

	void makePosVisible();
	void setPos(int x, int y, int w, int h);
	void getPos(int &x, int &y, int &w, int &h) const;
	void setTexWindow(float fTxMinX, float fTxMaxX, float fTxMinY, float fTxMaxY);

	void setExtents(int w, int h, int top, int bot);
	int getWidth() const { return m_Width; }
	int getHeight() const { return m_Height; }
	int getTopOffset() const { return m_TopOffset; }
	int getBotOffset() const { return m_BotOffset; }

	bool getShow() const { return m_Show; }
	void setShow(bool b) { m_Show = b; }
private:
	bool setClick(int x, int y);
};

class CDataWindowSet : public CWindow
{
	vector<CWindow *>	m_DataWindows;
	int		m_DataCur;
	int		m_Height, m_Width, m_BotOffset, m_TopOffset;

public:
	CDataWindowSet() : m_DataCur(-1) {}
	~CDataWindowSet()
	{
		for (int i = 0; i < m_DataWindows.size(); i++)
			delete m_DataWindows[i];
	}

	CWindow * addWindow(int x, int y, int w, int h)
	{
		CWindow * pWindow = new CWindow();
		pWindow->setExtents(m_Width, m_Height, m_TopOffset, m_BotOffset);
		if (w == 0 || h == 0)
		{
			static int x_off = 0, y_off = 0;
			pWindow->setPos(10+x_off, 10+m_TopOffset+y_off, 360, 240);
			x_off += 20; y_off += 20;
			if (10+m_TopOffset+y_off+240 > m_Height-m_BotOffset) 
				y_off = 0;
			if (10+x_off+360 > m_Width) 
				x_off = 0;
		}
		else
			pWindow->setPos(x, y, w, h);
		m_DataWindows.push_back(pWindow);
		return pWindow;
	}

	void delWindow(CWindow *pWindow)
	{
		m_DataCur = -1;
		for (int i = 0; i < m_DataWindows.size(); i++)
			if (m_DataWindows[i] == pWindow)
			{
				delete (m_DataWindows[i]);
				m_DataWindows.erase(m_DataWindows.begin() + i);
				return;
			}
	}
	bool in(int x, int y)
	{
		m_DataCur = -1;
		for (int i = m_DataWindows.size()-1; i >= 0 ; i--)
			if (m_DataWindows[i]->getShow() && m_DataWindows[i]->in(x, y))
			{
				m_DataCur = i;
				return true;
			}
			return false;
	}
	bool getClick(int x, int y)
	{
		if (!in(x, y))
			return false;
		return m_DataWindows[m_DataCur]->getClick(x, y);
	}
	void endClick(int x, int y)
	{
		if (m_DataCur == -1)
			return;
		m_DataWindows[m_DataCur]->endClick(x, y);
	}
	void setMove(int x, int y)
	{
		if (m_DataCur == -1)
			return;
		m_DataWindows[m_DataCur]->setMove(x, y);
	}
	void setExtents(int w, int h, int top, int bot)
	{
		m_Width = w;
		m_Height = h;
		m_TopOffset = top;
		m_BotOffset = bot;
		for (int i = 0; i < m_DataWindows.size(); i++)
			m_DataWindows[i]->setExtents(m_Width, m_Height, m_TopOffset, m_BotOffset);
	}
	void makePosVisible()
	{
		for (int i = 0; i < m_DataWindows.size(); i++)
			m_DataWindows[i]->makePosVisible();
	}
};

class CLocater : public CWindow
{
private:
	double m_curx, m_cury, m_curw, m_curh;
	double m_x, m_y, m_w, m_h;
	string m_path;
	float m_zoom;
	float m_ratio;
	int m_tex;
	Vec3d m_pos;

public:
	CLocater() : m_zoom(1.0), m_ratio(1.0), m_tex(-1) { m_pos = vl_0; }
	Vec3d setPos(int x, int y);
	void setPosGPS(Vec3d pos);
	void setZoom(double f);
	void setTranslate(int x, int y);
	void clearTexture();
	bool loadTexture(string FileName, double x, double y, double w, double h);
	void getData(string &Path, double &x0, double &y0, double &x1, double &y1);
	void draw();
};


class CControl
{
private:
	int m_Height, m_Width, m_BotOffset, m_TopOffset;

public:
	CControl() : m_Height(0), m_Width(0), m_BotOffset(0), m_TopOffset(0) {} 
	virtual bool in(int x, int y) = 0;
	virtual bool getFocusChange(int x, int y) = 0;
	virtual bool getClick(int x, int y) = 0;
	virtual void endClick(int x, int y) = 0;
	virtual void setMove(int x, int y) = 0;
	virtual void draw(bool bInit = false) = 0;
	virtual void clear() = 0;
	void setExtents(int w, int h, int top, int bot)
	{
		m_Width = w;
		m_Height = h;
		m_TopOffset = top;
		m_BotOffset = bot;
	}
	int getWidth() const { return m_Width; }
	int getHeight() const { return m_Height; }
	int getTopOffset() const { return m_TopOffset; }
	int getBotOffset() const { return m_BotOffset; }
};

class CCompass : public CControl
{
private:
	enum {COM_TX, COM_TRANS_TX, COM_ROT_TX, COM_ZOOM_TX, COM_ZOOM_OVER_TX, COM_CNT_TX};
	enum {COM_NONE, COM_ZOOM, COM_TRANS, COM_ROT};

	Vec3d m_Loc;
	Vec3d m_Dir;
	int m_X, m_Y, m_W, m_H, m_LEFT;
	double m_CamAzimuth;

	string m_file[COM_CNT_TX];
	int m_tx[COM_CNT_TX];
	int m_State;
	MouseAction_t m_Action;

public:
	CCompass() : m_CamAzimuth(0.0), m_X(0), m_Y(0), m_W(0), m_H(0), m_LEFT(0)
	{
		string fname[COM_CNT_TX] = {"compass_all.png", "compass_trans.png", "compass_rot.png", "compass_zoom.png", "compass_zoom_over.png"};
		for (int i = 0; i < COM_CNT_TX; i++)
		{
			m_file[i] = fname[i];
			m_tx[i] = -1;
		}
		m_Action = kActionNone;
		m_State = COM_NONE;
		m_Loc = vl_0;
		m_Dir = vl_0;
	}

	bool in(int x, int y);
	bool getFocusChange(int x, int y);
	bool getClick(int x, int y);
	void endClick(int x, int y);
	void setMove(int x, int y);
	void draw(bool bInit = false);
	void clear();

	void setCamAzimuth(double d) { m_CamAzimuth = d; }
	MouseAction_t getAction() const { return m_Action; }
	Vec3d getDir() const { return m_Dir; }
};

class CLighting : public CControl
{
private:
	enum {LIGHT_DIR_TX, LIGHT_DIR_OVER_TX, LIGHT_DIR_SET_TX, LIGHT_DIR_SET_OVER_TX, 
		LIGHT_ANGLE_TX, LIGHT_ANGLE_OVER_TX, LIGHT_ANGLE_SET_TX, LIGHT_ANGLE_SET_OVER_TX,
		LIGHT_LEVEL_TX, LIGHT_LEVEL_OVER_TX, LIGHT_LEVEL_SET_TX, LIGHT_LEVEL_SET_OVER_TX, LIGHT_CNT_TX};
	enum {LIGHT_NONE, LIGHT_DIR, LIGHT_ANGLE, LIGHT_LEVEL};

	int m_X, m_Y, m_W, m_H, m_LEFT;
	string m_file[LIGHT_CNT_TX];
	int m_tx[LIGHT_CNT_TX];

	float LIGHT_LEVEL_STEP;
	float LIGHT_LEVEL_MAX;
	float LIGHT_LEVEL_OFF;
	int m_State;
	int m_Action;
	Vec3d m_Center;
	Vec3d m_Dir;

	float m_LightDir;
	float m_LightAngle;

public:
	CLighting() : m_LightDir(45.0f), m_LightAngle(45.0f), m_X(0), m_Y(0), m_W(0), m_H(0), m_LEFT(0)
	{
		string fname[LIGHT_CNT_TX] = {"LightDir.png", "LightDirOver.png", "LightDirSet.png", "LightDirSetOver.png",
			"LightAngle.png", "LightAngleOver.png", "LightAngleSet.png", "LightAngleSetOver.png",
			"LightLevel.png", "LightLevelOver.png", "LightLevelSet.png", "LightLevelSetOver.png"};
		for (int i = 0; i < LIGHT_CNT_TX; i++)
		{
			m_file[i] = fname[i];
			m_tx[i] = -1;
		}
		LIGHT_LEVEL_STEP = 4.0f;
		LIGHT_LEVEL_MAX = 10.0f;
		LIGHT_LEVEL_OFF = 20.0f;
		m_State = LIGHT_NONE;
		m_Action = LIGHT_NONE;
		m_Center = vl_0;
		m_Dir = vl_0;
	}

	bool in(int x, int y);
	bool getFocusChange(int x, int y);
	bool getClick(int x, int y);
	void endClick(int x, int y);
	void setMove(int x, int y);
	void draw(bool bInit = false);
	void clear();

	float getDir() const { return m_LightDir; }
	void setDir(float fDir) { m_LightDir = fDir; }
	float getAngle() const { return m_LightAngle; }
	void setAngle(float fAngle) { m_LightAngle = fAngle; }
};

typedef void (*LPTimeLineUICallback)(int);

class CTimeline : public CControl
{
#define TIME_TX_CNT	44 
#define TIME_STATE_CNT 22
#define TIME_DRAW_CNT 19

private:
	enum {TIME_NONE= -1, TIME_BACK,TIME_LOOP_BAR,TIME_LOOP_LEFT,TIME_LOOP_RIGHT,
		TIME_TIME,TIME_LEAD,TIME_TRAIL,TIME_END1,TIME_END2,
		TIME_BEGIN_BTN,TIME_RR_BTN,TIME_REV_BTN,TIME_STOP_BTN,TIME_FWD_BTN,TIME_FF_BTN,TIME_END_BTN,
		TIME_LOOP_BTN,TIME_LEAD_BTN,TIME_TRAIL_BTN,
		TIME_TIME_SET, TIME_LEAD_SET, TIME_TRAIL_SET
	};

	int m_X, m_Y, m_W, m_H, m_LEFT;
	string m_file[TIME_TX_CNT];
	int m_tx[TIME_TX_CNT];
	int m_State;
	int m_Action;
	int m_click_off_x;
	int m_click_off_y;

	LPTimeLineUICallback	m_pUICallback;

public:

	CTimeline() : m_X(MAX_INT), m_Y(MAX_INT), m_W(64), m_H(64), m_LEFT(0)
	{
		string fname[TIME_TX_CNT] = {
			"Time.png", 
			"TimeLoopBar.png",
			"TimeLoopLeft.png",
			"TimeLoopRight.png",
			"TimeTime.png", "TimeTimeOn.png",
			"TimeLead.png", "TimeLeadOn.png",
			"TimeLead.png", "TimeLeadOn.png",
			"TimeEnd.png", "TimeEndOn.png",
			"TimeEnd.png", "TimeEndOn.png",
			"TimeBeginBtn.png", "TimeBeginBtnDn.png", "TimeBeginBtnOn.png",
			"TimeRRBtn.png", "TimeRRBtnDn.png", "TimeRRBtnOn.png",
			"TimeRevBtn.png", "TimeRevBtnDn.png", "TimeRevBtnOn.png",
			"TimeStopBtn.png", "TimeStopBtnDn.png", "TimeStopBtnOn.png",
			"TimeFwdBtn.png", "TimeFwdBtnDn.png", "TimeFwdBtnOn.png",
			"TimeFFBtn.png", "TimeFFBtnDn.png", "TimeFFBtnOn.png",
			"TimeEndBtn.png", "TimeEndBtnDn.png", "TimeEndBtnOn.png",
			"TimeLoopBtn.png", "TimeLoopBtnDn.png", "TimeLoopBtnOn.png",
			"TimeLeadBtn.png", "TimeLeadBtnDn.png", "TimeLeadBtnOn.png",
			"TimeTrailBtn.png", "TimeTrailBtnDn.png", "TimeTrailBtnOn.png"
		};
		for (int i = 0; i < TIME_TX_CNT; i++)
		{
			m_file[i] = fname[i];
			m_tx[i] = -1;
		}
		m_State = TIME_NONE;
		m_Action = TIME_NONE;
		m_click_off_x = -1;
		m_click_off_y = -1;
		m_pUICallback = NULL;
	}

	bool in(int x, int y);
	bool getFocusChange(int x, int y);
	bool getClick(int x, int y);
	void endClick(int x, int y);
	void setMove(int x, int y);
	void draw(bool bInit = false);
	void clear();

	void updatePos();
	void setExtents(int w, int h, int top, int bot)
	{
		CControl::setExtents(w, h, top, bot);
		updatePos();
	}
	void update_time_rect();
	double get_time_from_pos(int x, bool bSel);

	void setUICallback(LPTimeLineUICallback func) { m_pUICallback = func; }
	LPTimeLineUICallback getUICallback() { return m_pUICallback; }
	int getX() const { return m_X; }
	int getY() const { return m_Y; }
	bool getLoopLeft() const { return m_Action == TIME_LOOP_LEFT; }
	bool getLoopRight() const { return m_Action == TIME_LOOP_RIGHT; }
};

//==========[ class CDraw ]==================================================

class CVisScript;

class CDraw
{
private:
	Camera	m_Camera;

	CLocater	m_Locater;
	CDataWindowSet	m_DataWindowSet;

	CCompass m_Compass;
	CLighting m_Lighting;
	CTimeline m_Timeline;

	int		m_WaterMark_tx;

	int		m_Mouse_X, m_Mouse_Y;
	int		m_Width, m_Height;

	Vec3d	m_TerrainPos;
	Vec3d	m_CursorPos;

	bool	m_ValidPos;
	double	m_GeoCenter;

	int		m_TopOffset;
	int		m_BotOffset;

	bool	m_CameraTracking;
	Vec3f	m_DataCamOffset;
	Vec3d	m_DataCamPos;

	float	m_Far_P;
	float	m_Near_P;
	float	m_FOV;

	vector<Vec3d> m_ListOutlines;
	vector<Vec3d> m_SelListOutlines;

	int		m_LegendSelDataset;
	int		m_Quality;

public:

	CDraw()
	{
		m_Mouse_X = -1, m_Mouse_Y = -1;
		m_Width = 0, m_Height = 0;
		m_TerrainPos = vl_0;
		m_CursorPos = vl_0;
		m_ValidPos = false;
		m_GeoCenter = 0.0;
		m_WaterMark_tx = -1;

		m_DataCamOffset = vl_0;
		m_DataCamPos = vl_0;

		m_Far_P = 1;
		m_Near_P = 45.0;
		m_FOV = 45.0;

		m_CameraTracking = false;

		m_LegendSelDataset = -1;
		setScreenOffsets(0, 0);
		m_Quality = 90;

		m_Lighting.setDir(45.0f);
		m_Lighting.setAngle(45.0f);
	}

	void initState();
	void resize( int w = 0, int h = 0 );
	void clear_screen();
	void enterOrtho2D();
	void exitOrtho2D();

	CDataWindowSet & getDataWindowSet() { return m_DataWindowSet; }
	CLocater & getLocater() { return m_Locater; }

	const CCompass & getCompass() const { return m_Compass; }
	CCompass & editCompass() const { return const_cast<CCompass &>(m_Compass); }
	const CLighting & getLighting() const { return m_Lighting; }
	CLighting & editLighting() const { return const_cast<CLighting &>(m_Lighting); }
	const CTimeline & getTimeline() const { return m_Timeline; }
	CTimeline & editTimeline() const { return const_cast<CTimeline &>(m_Timeline); }

	void drawTimelineText();

	float getLightDir() const { return m_Lighting.getDir(); }
	void setLightDir(float fDir) { m_Lighting.setDir(fDir); }
	float getLightAngle() const { return m_Lighting.getAngle(); }
	void setLightAngle(float fAngle) { m_Lighting.setAngle(fAngle); }
	Vec3d getLightPosition() const;

	Camera & getCamera() const { return const_cast<Camera &>(m_Camera); }
	void setLookAtGPS( const Vec3d lookAt );
	Vec3d getLookAtGPS() const;

	Vec3d getTerrainPos() const { return m_TerrainPos; }
	void setTerrainPos(Vec3d pos) { m_TerrainPos = pos; m_ValidPos = true; }
	bool getValidPos() const { return m_ValidPos; }
	Vec3d getCursorPos() const { return m_CursorPos; }
	void setCursorPos(Vec3d pos) { m_CursorPos = pos; }
	int getMouseX() const { return m_Mouse_X; }
	int getMouseY() const { return m_Mouse_Y; }
	void setMousePos(int x, int y) { m_Mouse_X = x; m_Mouse_Y = y; }
	double getGeoMin() const { return -180.0+m_GeoCenter; }
	double getGeoMax() const { return 180.0+m_GeoCenter; }
	double getGeoCenter() const { return m_GeoCenter; }
	void setGeoCenter(double d) { m_GeoCenter = d; }

	vector<Vec3d> &getListOutlines() const { return const_cast<vector<Vec3d> &>(m_ListOutlines); }
	vector<Vec3d> &getSelListOutlines() const { return const_cast<vector<Vec3d> &>(m_SelListOutlines); }
	Vec3d getListOutlinesValue(int i) { return m_ListOutlines[i]; }

	int getWidth() const { return m_Width; }
	int getHeight() const { return m_Height; }

	Vec3d getDataCamPos() const { return m_DataCamPos; }
	void setDataCamPos(Vec3d pos) { m_DataCamPos = pos; }
	void setDataCamOffset(Vec3f off) { m_DataCamOffset = off; }

	Vec3f get2DPoint(Vec3f pos) const;
	Vec3f get2DPointGPS(Vec3d pos) const;
	Vec3f get3DPoint(int x2, int y2, double depth) const;

	bool getMinCellSize(double m_LonMin, double m_LatMin, double m_LonMax, double m_LatMax, double &fMinCell) const;
	bool GetTerrainHeight( double dLat, double dLong, double &dDepth ) const;
	bool setTerrainHeight( Vec3d &pos ) const;
	bool setTerrainHeightScene( Vec3f &pos ) const
	{
		Vec3d pnt = getGPSFromScenePnt(pos);
		if (!GetTerrainHeight(pnt[0], pnt[1], pnt[2]))
			return false;
		pos = getScenePntFromGPS(pnt);
		return true;
	}
	void resetLookAtHeight();

	Vec3f getScenePntFromGPS(Vec3d pos, bool bScaled = true) const;
	Vec3d getGPSFromScenePnt(Vec3f pos) const;
	Vec3d getCameraFromGPS(Vec3d pntGPS) const;
	Vec3d getGPSFromCamera(Vec3d pnt) const;

	void gotoCameraView(Vec3d pntGPS, double elev, double azim, double dolly, double time = -1.0);
	void gotoCameraViewRange(Vec3d min_pos, Vec3d max_pos);
	void gotoGPSPoint(Vec3d pnt, double fDistKm);
	void gotoGPSRange(Vec3d pnt1, Vec3d pnt2, double fDist);

	void rotateCamera(double time, bool bCW = true);
	void getCameraView(Vec3d &pos, double &elev, double &azim, double &dolly);

	bool inTopView() const;
	void setViewAngle(int iType);
	int addCurrentView();
	void updateView(int ndx);

	Vec3f getCameraPosition() const;
	Vec3d getCameraPositionGPS() const;

	double getCameraDistKM(Vec3f pos) const;
	double getCameraDistKM_GPS(Vec3d pos) const;
	double getCameraDistKM() const;

	bool getCameraExtent(Vec3d &minpos, Vec3d &maxpos);
	void resetScreenBounds();

	void SetCameraPosition(Vec3f pos, Vec3f dir);

	bool getCameraTracking() const { return m_CameraTracking; }
	void setCameraTracking(bool b) { m_CameraTracking = b; }

	bool drawGL();

	bool getTerrainIntersect( int xPos, int yPos, Vec3d &pos );
	bool getScreenSelect(int xPos, int yPos, int &iType, int &iLayer, int &iObj, int &iSubObj);

	bool init_drawGL();

	string saveVisualization(string fileName, CVisScript VisScript);
	bool saveImage(string fileName, CVisScript VisScript);
	bool saveMovie(string szFileName, CVisScript VisScript);

	void initText();
	void delText();
	void setGLFont(int size);
	void glDrawText(string strText, color clr, int size, float x, float y, float z);
	void glDrawText(string strText, color clr, float x, float y);
	void drawScaledText(string strText, color clr, Vec3f pos, float scale);
	void draw3DText(string strText, Vec3f pos, color clr, int size, bool bFlush);

	void drawTimeStamp();
	void clearWatermark();
	void initWatermark();
	void drawWatermark();

	void initFadeTimer();
	void startFadeTimer(double fTime);
	void setFadeTimer(double fTime);
	void exitFadeTimer();
	void drawFadeImage();
	void drawScreenImages();

	void drawVerticalImages();

	int  getLegendSelDataset();
	vector<int> getLegendList();
	bool getLegendClick(int x, int y);
	void drawLegend();

	int	getTopOffset() { return m_TopOffset; }
	int	getBotOffset() { return m_BotOffset; }
	int getRealBot() { return getHeight()-getBotOffset(); }
	void updateWindows()
	{
		m_Compass.setExtents(m_Width, m_Height, m_TopOffset, m_BotOffset);
		m_Lighting.setExtents(m_Width, m_Height, m_TopOffset, m_BotOffset);
		m_Timeline.setExtents(m_Width, m_Height, m_TopOffset, m_BotOffset);
		getLocater().setExtents(m_Width, m_Height, m_TopOffset, m_BotOffset);
		getDataWindowSet().setExtents(m_Width, m_Height, m_TopOffset, m_BotOffset);
	}
	void setScreenOffsets(int iTop, int iBot)
	{
		m_TopOffset = iTop;
		m_BotOffset = iBot;
		updateWindows();
	}
	void setScreenExtents(int w, int h)
	{
		m_Width = w;
		m_Height = h;
		updateWindows();
	}

	void resetWindows()
	{
		m_Timeline.updatePos();
		m_Locater.makePosVisible();
		getDataWindowSet().makePosVisible();
	}
	void initWindows(LPTimeLineUICallback pTimeFunc);
	void clearWindows();
	void resize_glwindow(int w, int h);

	void initWindows_cmdln();
	void clearWindows_cmdln();

	void getObjectPickValue(int iVal, int &iLayer, int &iObj) const
	{
		iVal &= 0x00ffffff;
		iLayer = iVal>>16;
		iObj = iVal & 0x0000ffff;
	}
	void getControlPickValue(int iVal,int &iObj, int &iSubObj) const
	{
		iObj = iVal & 0x000000ff;
		iSubObj = (iVal >> 8) - 1;
	}
	void getLinePickValue(int iVal, int &iLayer, int &iObj, int &iSubObj) const
	{
		iVal &= 0x00ffffff;
		iLayer = iVal>>16;
		getControlPickValue(iVal &= 0x0000ffff, iObj, iSubObj);
	}

private: 
	void drawWindows();
	void drawWindowDecorations(int x, int y, int w, int h);

	void drawPosition(Vec3f pt);
	void drawLightVector();

	bool setImageBuffer(int ww, int hh, bool bEntry);
	void getGLFrame(unsigned char *imageBuffer, int ww, int hh);
	bool saveGLFrame(string fileName);
	void initVisSettings(CVisScript &VisScript);

	bool saveLayeredFrame(string fileName);

};


extern CDraw  g_Draw;

