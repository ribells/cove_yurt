#pragma once

#include "const.h"
#include "xmlParser.h"

typedef int (getPref)(void);
typedef void (setPref)(int);
typedef string (getPrefList)(int);

#define SET_VIEW	1
#define SET_LIST	2
#define SET_COLOR	4
#define SET_WARN	8

struct pref_t {
	const string pLabel;
	getPref	*pGet;
	setPref *pSet;
	getPrefList *pGetList;
	int iFlags;
	const string pToolTip;
};

enum {PLAY_STOP = 0, PLAY_STEP, PLAY_AUTO, PLAY_MAX};

//==========[ class CSettings ]==================================================
class CSettings
{
public:
	//internal state
	bool	m_NeedRedraw;
	bool	m_DragMooring;
	bool	m_UpdataLayerTree;
	bool	m_HoldCursorHeight;
	bool	m_PosFollowsCursor;
	bool	m_Printing;
	bool	m_Presenting;
	bool	m_Initializing;
	bool	m_PythonAvailable;
	bool	m_UserLoggedOn;
	bool	m_AdminUser;

	bool	m_SessionCapture;
	int		m_SessionPlayback;

	bool	m_ShowScreenSize;
	bool	m_DownloadPage;
	bool	m_ExpandDataDlg;

	//end internal state

	double	m_JiggleFactor;
	bool	m_ShowDBTop;
	bool	m_ShowVBTop;
	bool	m_ShowDB;
	bool	m_ShowVB;
	bool	m_ShowDebugInfo;
	bool	m_MapView;
	bool	m_ShowLighting;
	bool	m_ShowTimeline;
	bool	m_AutoSave;
	bool	m_SystemTesting;
	bool	m_AutoRecenter;
	bool	m_AutoTileFactor;
	int		m_scrn_x, m_scrn_y, m_scrn_w, m_scrn_h;

	string	m_strUserName;
	string	m_strPassword;
	string	m_StartupFile;
	vector<string>	m_RecentFiles;

	bool	m_ServerMode;
	bool	m_Console;

	bool	m_ShowLatLong;
	bool	m_ShowUTMGrid;
	bool	m_ShowGridLabels;
	double	m_GridFactor;

	int		m_Projection;
	bool	m_ShowPosition;
	bool	m_ShowOutlines;
	bool	m_ShowTrnLegend;
	int		m_LegendHeight;
	bool	m_AnimateCamera;
	double	m_FogFactor;

	color	m_BackColor;
	color	m_TitleColor;
	color	m_GridColor;
	color	m_FogColor;

	Vec3d	m_LocalOrigin;

	bool	m_ShowTerrain;
	bool	m_ShowData;
	bool	m_ShowObjects;
	bool	m_ShowLines;
	bool	m_ShowNames;
	bool	m_ShowIcons;

	float	m_ObjectSize;

	bool	m_ShowLargeText;
	bool	m_ShowLayoutInfo;
	int		m_PositionUnits;
	int		m_DistanceUnits;
	bool	m_LogDebugOutput;
	bool	m_CheckForUpdates;
	bool	m_ShowCompass;
	bool	m_ShowLocater;
	bool	m_WaterMark;
	bool	m_TimeStamp;

	string	m_strVersion;
	string	m_strGLVersion;
	string	m_strCopyright;

public:

	CSettings()
	{
		m_NeedRedraw = true;
		m_Initializing = true;
		m_Printing = false;
		m_Presenting = false;
		m_DragMooring = false;
		m_UpdataLayerTree = false;
		m_HoldCursorHeight = false;
		m_PythonAvailable = false;
		m_UserLoggedOn = false;
		m_AdminUser = false;

		m_SessionCapture = false; //set to true to have session capture start at startup
		m_SessionPlayback = PLAY_STOP;

		m_ShowScreenSize = false;
		m_DownloadPage = false;
		m_ExpandDataDlg = false;

		m_ServerMode = false;
		m_scrn_x = m_scrn_y = m_scrn_w = m_scrn_h = 0;

		m_ShowLatLong = false;
		m_ShowUTMGrid = false;
		m_ShowGridLabels = true;
		m_GridFactor = 1.0;
		m_ShowPosition = false;
		m_PosFollowsCursor = true;
		m_ShowOutlines = false;
		m_ShowTrnLegend = true;
		m_LegendHeight = 128;

		m_Projection = 1;	//PROJ_GEO
		m_AnimateCamera = true;
		m_FogFactor = 0.0;
		m_BackColor = 0xff000000;
		m_TitleColor = 0xdd7fffcc;
		m_GridColor = 0x7f000000;
		m_FogColor = 0xcc444444;
		m_JiggleFactor = 2.0;

		m_ShowTerrain = true;
		m_ShowData = true;
		m_ShowObjects = true;
		m_ShowIcons = false;
		m_ShowLines = true;
		m_ShowNames = true;

		m_ShowLargeText = false;
		m_ShowDBTop = true;
		m_ShowVBTop = true;
		m_ShowDB = true;
		m_ShowVB = true;

		m_ShowLayoutInfo = false;
		m_ShowDebugInfo = false;
		m_MapView = false;
		m_ShowCompass = true;
		m_ShowLocater = false;
		m_ShowLighting = true;
		m_ShowTimeline = false;

		m_PositionUnits = 0;
		m_DistanceUnits = 0;
		m_LocalOrigin = Vec3d(47.8939,-129.1645,0);
		m_LogDebugOutput = true;
		m_CheckForUpdates = true;

		m_WaterMark = true;
		m_TimeStamp = true;

		m_SystemTesting = false;
		m_AutoRecenter = true;
		m_AutoTileFactor = false;

		m_strUserName = "";
		m_strPassword = "";

		m_strVersion = "0.0";
		m_strGLVersion = "0.0";
		m_strCopyright = "(c) University of Washington 2008-2010";

#ifdef _DEBUG
		m_Console = true;
#else
		m_Console = false;
#endif

		m_StartupFile = "";
		m_RecentFiles.clear();
	}
	~CSettings()
	{
	}

	void setScreenPos(int x, int y, int w, int h)
	{
		m_scrn_x = x; m_scrn_y = y; m_scrn_w = w; m_scrn_h = h;
	}
	void getScreenPos(int &x, int &y, int &w, int &h)
	{
		x = m_scrn_x; y = m_scrn_y; w = m_scrn_w; h = m_scrn_h;
	}

	bool serialize_Node(XMLNode &xn1, bool bView, bool bFile = false);
	bool deserialize_Node(XMLNode &xn1, bool bFile = false);
	bool deserialize(string strText);
	string serialize(bool bView);
	bool readFile(string strPath);
	void writeFile(string strPath);

};

extern CSettings g_Set;

