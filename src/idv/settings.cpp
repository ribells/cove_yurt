/* =============================================================================
	File: settings.cpp

 =============================================================================== */

#include "utility.h"
#include "web/web_file.h"

#include "world.h"
#include "gl_draw.h"
#include "settings.h"

CSettings		g_Set;

//  SETTINGS FUNCTIONS
float				g_Heights[] = { 0.0001, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 10.0, 20.0, 50.0, 100.0, -1.0 };
string getHeightText(int i)
{
	static char buf[32];
	if (g_Heights[i] < 0)
		return "";
	sprintf(buf, "%.1f", g_Heights[i]);
	return buf;
}
int getLandHeightFactor()
{
	float	dHeight = g_World.getScale().land;
	for (int i = 0; g_Heights[i] >= 0; i++)
		if (dHeight <= g_Heights[i])
			return i;
	return 0;
}
void setLandHeightFactor(int i)
{
	t_scale scale = g_World.getScale();
	scale.land = g_Heights[i];
	g_World.setScale(scale);
	g_Draw.resetLookAtHeight();
}
int getOceanHeightFactor()
{
	float	dHeight = g_World.getScale().sea;
	for (int i = 0; g_Heights[i] >= 0; i++)
		if (dHeight <= g_Heights[i])
			return i;
	return 0;
}
void setOceanHeightFactor(int i)
{
	t_scale scale = g_World.getScale();
	scale.sea = g_Heights[i];
	g_World.setScale(scale);
	g_Draw.resetLookAtHeight();
}

float	g_Tile[] = { 0.25, 0.50, 1.0, 2.0, 4.0, 8.0, -1.0 };
string getTileText(int i)
{
	static char buf[32];
	if (g_Tile[i] < 0)
		return "";
	sprintf(buf, "%.1f", g_Tile[i]);
	return buf;
}
int getTileFactor()
{
	double	dTile = g_World.getTerrain().getTileFactor();
	for (int i = 0; g_Tile[i] >= 0; i++)
		if (dTile <= g_Tile[i])
			return i;
	return 0;
}
void setTileFactor(int i)
{
	if (getTileFactor() == i)
		return;
	g_World.getTerrain().setTileFactor(g_Tile[i]);
}

float	g_ObjectSize[] = { 0.25, 0.50, 1.0, 1.5, 2.0, 3.0, 5.0, 10.0, 50.0, -1.0 };
string getObjectSizeText(int i)
{
	static char buf[32];
	if (g_ObjectSize[i] < 0)
		return "";
	sprintf(buf, "%.1f", g_ObjectSize[i]);
	return buf;
}
int getObjectSize()
{
	float	fMag = g_Set.m_ObjectSize;
	for (int i = 0; g_ObjectSize[i] >= 0; i++)
		if (fMag <= g_ObjectSize[i])
			return i;
	return 0;
}
void setObjectSize(int i)
{
	g_Set.m_ObjectSize = g_ObjectSize[i];
}

float	g_Fog[] = { 0, 0.1, 0.2, 0.4, 0.5, 0.6, 0.8, 1.0, -1 };
string getFogText(int i)
{
	static char buf[32];
	if (g_Fog[i] < 0)
		return "";
	sprintf(buf, "%.1f", g_Fog[i]);
	return buf;
}
int getFogFactor()
{
	for (int i = 0; g_Fog[i] >= 0; i++)
		if (g_Set.m_FogFactor <= g_Fog[i])
			return i;
	return 0;
}
void setFogFactor(int i)
{
	g_Set.m_FogFactor = g_Fog[i];
}

float	g_Grid[] = { 0.25, 0.5, 1.0, 2.0, 4.0, -1 };
string getGridText(int i)
{
	static char buf[32];
	if (g_Grid[i] < 0)
		return "";
	sprintf(buf, "%.1f", g_Grid[i]);
	return buf;
}
int getGridFactor()
{
	for (int i = 0; g_Grid[i] >= 0; i++)
		if (g_Set.m_GridFactor <= g_Grid[i])
			return i;
	return 0;
}
void setGridFactor(int i)
{
	g_Set.m_GridFactor = g_Grid[i];
}

float	g_Jiggle[] = { 0.0, 2.0, 4.0, 6.0, 8, 0, -1 };
string getJiggleText(int i)
{
	static char buf[32];
	if (g_Jiggle[i] < 0)
		return "";
	sprintf(buf, "%.1f", g_Jiggle[i]);
	return buf;
}
int getJiggleFactor()
{
	for (int i = 0; g_Jiggle[i] >= 0; i++)
		if (g_Set.m_JiggleFactor <= g_Jiggle[i])
			return i;
	return 0;
}
void setJiggleFactor(int i)
{
	g_Set.m_JiggleFactor = g_Jiggle[i];
}

float	g_LegendHeight[] = { 128, 256, 512, 1024, 2048, -1 };
string getLegendHtTxt(int i)
{
	static char buf[32];
	if (g_LegendHeight[i] < 0)
		return "";
	sprintf(buf, "%.0f", g_LegendHeight[i]);
	return buf;
}
int getLegendHeight()
{
	for (int i = 0; g_LegendHeight[i] >= 0; i++)
		if (g_Set.m_LegendHeight <= g_LegendHeight[i])
			return i;
	return 0;
}
void setLegendHeight(int i)
{
	g_Set.m_LegendHeight = g_LegendHeight[i];
}

string	g_PosUnit[] = { "Degrees", "Deg Min", "Deg Min Sec", "UTM", "Local", "" };
string getPosUnitText(int i)
{
	return g_PosUnit[i];
}
int getPositionUnits()
{
	return g_Set.m_PositionUnits;
}
void setPositionUnits(int i)
{
	g_Set.m_PositionUnits = i;
}

string	g_PosOriginText[] = { "Endeavour", "Axial", "" };
Vec3d	g_PosOrigin[] = { Vec3d(47.8939, -129.1645, 0), Vec3d(45.900, -130.066667, 0), vl_0 };
string getPosOriginText(int i)
{
	return g_PosOriginText[i];
}
int getPosOrigin()
{
	for (int i = 0; g_PosOriginText[i].size(); i++)
		if (g_Set.m_LocalOrigin == g_PosOrigin[i])
			return i;
	return 0;
}
void setPosOrigin(int i)
{
	g_Set.m_LocalOrigin = g_PosOrigin[i];
}

string	g_DistUnit[] = { "Kilometers", "Miles", "Nautical Miles", "" };
string getDistUnitText(int i)
{
	return g_DistUnit[i];
}
int getDistanceUnit()
{
	return g_Set.m_DistanceUnits;
}
void setDistanceUnit(int i)
{
	g_Set.m_DistanceUnits = i;
}

string	g_DateTimeFormat[] = { "MDY", "DMY", "" };
string getDateTimeFormatText(int i)
{
	return g_DateTimeFormat[i];
}
int getDateTimeFormat()
{
	return g_Env.m_TimeFormat;
}
void setDateTimeFormat(int i)
{
	g_Env.m_TimeFormat = i;
}

string	g_PanelText[] = { "Hide", "At Top", "At Bottom", "" };
string getPanelText(int i)
{
	return g_PanelText[i];
}
int getShowDB()
{
	return g_Set.m_ShowDB ? g_Set.m_ShowDBTop ? 1 : 2 : 0;
}
void setShowDB(int i)
{
	g_Set.m_ShowDB = (i > 0);
	g_Set.m_ShowDBTop = (i == 1);
	g_Draw.resetScreenBounds();
}
int getShowVB()
{
	return g_Set.m_ShowVB ? g_Set.m_ShowVBTop ? 1 : 2 : 0;
}
void setShowVB(int i)
{
	g_Set.m_ShowVB = (i > 0);
	g_Set.m_ShowVBTop = (i == 1);
	g_Draw.resetScreenBounds();
}

string	g_ProjectionText[] = { "Globe", "Geographic", "Mercator", "", "Elliptical", "" };
string getProjectionText(int i)
{
	return g_ProjectionText[i];
}
int get_projection()
{
	return g_Set.m_Projection;
}
void set_projection(int i)
{
	g_Set.m_Projection = i;
	g_World.getTerrain().setProjection(g_Set.m_Projection);
	g_World.getDataSet().setDirty();
}

float	g_GeoCenter[] = { -180, -144, -108, -72, -36, 0, 36, 72, 108, 144, 180, NODATA };
string getGeoCenterText(int i)
{
	static char buf[32];
	if (ISNODATA(g_GeoCenter[i]))
		return "";
	sprintf(buf, "%.1f", g_GeoCenter[i]);
	return buf;
}
int getGeoCenter()
{
	int iSel = 0;
	for (; g_GeoCenter[iSel] != 180; iSel++)
		if (g_Draw.getGeoCenter() <= g_GeoCenter[iSel])
			break;
	return iSel;
}
void setGeoCenter(int i)
{
	if (g_Draw.getGeoCenter() == g_GeoCenter[i])
		return;
	g_Draw.setGeoCenter(g_GeoCenter[i]);
	g_World.getTerrain().setNeedTerrainUpdate();
	g_World.getDataSet().setDirty();
}

/* =============================================================================
 =============================================================================== */
int getShowLocater()
{
	return g_Set.m_ShowLocater ? 1 : 0;
}
void setShowLocater(int i)
{
	g_Set.m_ShowLocater = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowPosition()
{
	return g_Set.m_ShowPosition ? 1 : 0;
}
void setShowPosition(int i)
{
	g_Set.m_ShowPosition = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getPosFollowsCursor()
{
	return g_Set.m_PosFollowsCursor ? 1 : 0;
}
void setPosFollowsCursor(int i)
{
	g_Set.m_PosFollowsCursor = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int get_show_utm_grid()
{
	return g_Set.m_ShowUTMGrid ? 1 : 0;
}
void set_show_utm_grid(int i)
{
	g_Set.m_ShowUTMGrid = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int get_show_latlon_grid()
{
	return g_Set.m_ShowLatLong ? 1 : 0;
}
void set_show_latlon_grid(int i)
{
	g_Set.m_ShowLatLong = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowGridLabels()
{
	return g_Set.m_ShowGridLabels ? 1 : 0;
}
void setShowGridLabels(int i)
{
	g_Set.m_ShowGridLabels = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowOutlines()
{
	return g_Set.m_ShowOutlines ? 1 : 0;
}
void setShowOutlines(int i)
{
	g_Set.m_ShowOutlines = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowTrnLegend()
{
	return g_Set.m_ShowTrnLegend ? 1 : 0;
}
void setShowTrnLegend(int i)
{
	g_Set.m_ShowTrnLegend = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getHideTileSeams()
{
	return g_World.getTerrain().getShowSkirts() ? 1 : 0;
}
void setHideTileSeams(int i)
{
	g_World.getTerrain().setShowSkirts(i == 1);
}

/* =============================================================================
 =============================================================================== */
int get_show_names()
{
	return g_Set.m_ShowNames ? 1 : 0;
}
void set_show_names(int i)
{
	g_Set.m_ShowNames = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowLargeText()
{
	return g_Set.m_ShowLargeText ? 1 : 0;
}
void setShowLargeText(int i)
{
	g_Set.m_ShowLargeText = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowTerrain()
{
	return g_Set.m_ShowTerrain ? 1 : 0;
}
void setShowTerrain(int i)
{
	g_Set.m_ShowTerrain = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getAnimateCamera()
{
	return g_Set.m_AnimateCamera ? 1 : 0;
}
void setAnimateCamera(int i)
{
	g_Set.m_AnimateCamera = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getMomentumCamera()
{
	return g_Draw.getCamera().getMomentum() ? 1 : 0;
}
void setMomentumCamera(int i)
{
	g_Draw.getCamera().setMomentum(i == 1);
}

/* =============================================================================
 =============================================================================== */
int getServerMode()
{
	return g_Set.m_ServerMode ? 1 : 0;
}
void set_server_mode(int i)
{
	g_Set.m_ServerMode = (i == 1);
	runWebServer(g_Set.m_ServerMode);
}

/* =============================================================================
 =============================================================================== */
int getShowLayoutInfo()
{
	return g_Set.m_ShowLayoutInfo ? 1 : 0;
}
void setShowLayoutInfo(int i)
{
	g_Set.m_ShowLayoutInfo = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowCompass()
{
	return g_Set.m_ShowCompass ? 1 : 0;
}
void setShowCompass(int i)
{
	g_Set.m_ShowCompass = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowLighting()
{
	return g_Set.m_ShowLighting ? 1 : 0;
}
void setShowLighting(int i)
{
	g_Set.m_ShowLighting = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowTimeline()
{
	return g_Set.m_ShowTimeline ? 1 : 0;
}
void setShowTimeline(int i)
{
	g_Set.m_ShowTimeline = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowIcons()
{
	return g_Set.m_ShowIcons ? 1 : 0;
}
void setShowIcons(int i)
{
	g_Set.m_ShowIcons = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowObjects()
{
	return g_Set.m_ShowObjects ? 1 : 0;
}
void setShowObjects(int i)
{
	g_Set.m_ShowObjects = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getShowLines()
{
	return g_Set.m_ShowLines ? 1 : 0;
}
void setShowLines(int i)
{
	g_Set.m_ShowLines = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getMapView()
{
	return g_Set.m_MapView ? 1 : 0;
}
void setMapView(int i)
{
	if (g_Set.m_MapView == (bool)i)
		return;
	g_Set.m_MapView = (i == 1);
	if (i)
		g_Draw.getCamera().setElevation(-vl_pi / 2);
	else
		g_Draw.getCamera().setElevation(-vl_pi / 8);
}

/* =============================================================================
 =============================================================================== */
int getShowDebugInfo()
{
	return g_Set.m_ShowDebugInfo ? 1 : 0;
}
void setShowDebugInfo(int i)
{
	g_Set.m_ShowDebugInfo = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getLogDebugOutput()
{
	return g_Set.m_LogDebugOutput ? 1 : 0;
}
void setLogDebugOutput(int i)
{
	g_Set.m_LogDebugOutput = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getCheckForUpdates()
{
	return g_Set.m_CheckForUpdates ? 1 : 0;
}
void setCheckForUpdates(int i)
{
	g_Set.m_CheckForUpdates = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getBackColor()
{
	return g_Set.m_BackColor;
}
void setBackColor(int i)
{
	g_Set.m_BackColor = (color) i;
}

/* =============================================================================
 =============================================================================== */
int getTitleColor()
{
	return g_Set.m_TitleColor;
}
void setTitleColor(int i)
{
	g_Set.m_TitleColor = (color) i;
}

/* =============================================================================
 =============================================================================== */
int getGridColor()
{
	return g_Set.m_GridColor;
}
void setGridColor(int i)
{
	g_Set.m_GridColor = (color) i;
}

/* =============================================================================
 =============================================================================== */
int getFogColor()
{
	return g_Set.m_FogColor;
}
void setFogColor(int i)
{
	g_Set.m_FogColor = (color) i;
}

/* =============================================================================
 =============================================================================== */
int getSystemTesting()
{
	return g_Set.m_SystemTesting ? 1 : 0;
}
void setSystemTesting(int i)
{
	g_Set.m_SystemTesting = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getAutoRecenter()
{
	return g_Set.m_AutoRecenter ? 1 : 0;
}
void setAutoRecenter(int i)
{
	g_Set.m_AutoRecenter = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getAutoTileFactor()
{
	return g_Set.m_AutoTileFactor ? 1 : 0;
}
void setAutoTileFactor(int i)
{
	g_Set.m_AutoTileFactor = (i == 1);
	g_World.getTerrain().setTileScale(1.0);
}

/* =============================================================================
 =============================================================================== */
int getCompressTextures()
{
	return m_TextureCompression ? 1 : 0;
}
void setCompressTextures(int i)
{
	m_TextureCompression = (i == 1);
}

/* =============================================================================
 =============================================================================== */
int getInternetEnabled()
{
	return g_Env.m_Internet ? 1 : 0;
}
void setInternetEnabled(int i)
{
	g_Env.m_Internet = (i == 1);
}

pref_t	prefs[] =
{
	{ "Screen", 0, 0, 0, false, "" },
	{ "Dashboard", getShowDB, setShowDB, getPanelText, SET_LIST, "Show and position the dashboard" },
	{ "View Panel", getShowVB, setShowVB, getPanelText, SET_LIST, "Show and position a list of user saved views" },
	{ "Compass", getShowCompass, setShowCompass, 0, SET_VIEW, "Show the compass" },
	{ "Timeline", getShowTimeline, setShowTimeline, 0, SET_VIEW, "Show the timeline control" },
	{ "Locater Map", getShowLocater, setShowLocater, 0, 0, "Show the position locater control" },
	{ "Lighting Control", getShowLighting, setShowLighting, 0, 0, "Show the lighting and shading control" },
	{ "Map View", getMapView, setMapView, 0,SET_VIEW, "Turn display into a 2D map view" },
	{ "Large Text", getShowLargeText, setShowLargeText, 0, SET_VIEW, "Switch to large fonts on the screen" },
	{ "Background Color", getBackColor, setBackColor, 0, SET_VIEW | SET_COLOR, "Set the background color" },
	{ "View Title Color", getTitleColor, setTitleColor, 0, SET_VIEW |SET_COLOR, "Set the view title color when playing views" },

	{ "Visuals", 0, 0, 0, false, "" },
	{ "Animate Camera", getAnimateCamera, setAnimateCamera, 0, 0,"Fly the camera to new locations rather than jumping" },
	{ "Camera Momentum", getMomentumCamera, setMomentumCamera, 0, 0,"Continue moving camera using momentum from last movement" },
	{ "Mark Current Position", getShowPosition, setShowPosition, 0, SET_VIEW,"Show the current position on the screen with a cross" },
	{ "Position Follows Mouse", getPosFollowsCursor, setPosFollowsCursor, 0, SET_VIEW,"The current position follows the cursor position" },
	{ "Lat Long Grid", get_show_latlon_grid, set_show_latlon_grid, 0, SET_VIEW, "Show a Lat/Long grid on the earth" },
	{ "UTM Grid", get_show_utm_grid, set_show_utm_grid, 0, SET_VIEW,"Show a UTM based grid around the current location" },
	{ "Grid Labels", getShowGridLabels, setShowGridLabels, 0, SET_VIEW, "Show labels on the displayed grids" },
	{ "Grid Factor", getGridFactor, setGridFactor, getGridText, SET_VIEW |SET_LIST, "Vary the frequency of lines in the displayed grid" },
	{ "Gridline Color", getGridColor, setGridColor, 0, SET_VIEW | SET_COLOR, "Set the grid color and transparency" },
	{ "Fog", getFogFactor, setFogFactor, getFogText, SET_VIEW |SET_LIST, "Add fog to an image to fade out terrain further away" },
	{ "Fog Color", getFogColor, setFogColor, 0, SET_VIEW | SET_COLOR, "Set fog color and transparency" },
	{ "Legend Height", getLegendHeight, setLegendHeight, getLegendHtTxt, SET_VIEW |SET_LIST, "Set the height of the legend" },

	{ "Terrain", 0, 0, 0, false, "" },
	{ "Show Terrain", getShowTerrain, setShowTerrain, 0, SET_VIEW, "Show the terrain and images" },
	{ "Terrain Outlines", getShowOutlines, setShowOutlines, 0, SET_VIEW,"Show yellow outlines around the extents of the loaded terrain sets" },
	{ "Gradient Legend", getShowTrnLegend, setShowTrnLegend, 0, SET_VIEW,"Show a depth legend if a terrain gradient is showing" },
	{ "Projection", get_projection, set_projection, getProjectionText, SET_VIEW |SET_LIST |SET_WARN, "Reproject map to selected projection" },
	{ "Geographic Center", getGeoCenter, setGeoCenter, getGeoCenterText, SET_VIEW |	SET_LIST, "Change the start offset of the geographic projection" },
	{ "Land Scale", getLandHeightFactor, setLandHeightFactor, getHeightText, SET_VIEW |	SET_LIST, "Change the scale of the land terrain" },
	{ "Ocean Scale", getOceanHeightFactor, setOceanHeightFactor, getHeightText, SET_VIEW |SET_LIST, "Change the scale of the ocean bathymetry" },
	{ "Terrain Detail", getTileFactor, setTileFactor, getTileText, SET_VIEW |SET_LIST |SET_WARN, "Change the tile resolution factor" },
	{ "Distance Units", getDistanceUnit, setDistanceUnit, getDistUnitText, SET_LIST,"Sets the units used when displaying distance" },
	{ "Position Units", getPositionUnits, setPositionUnits, getPosUnitText, SET_LIST,"Sets the units used when displaying location" },
	{ "Time Format", getDateTimeFormat, setDateTimeFormat, getDateTimeFormatText, SET_LIST, "Sets the time format" },
	{ "Local Origin", getPosOrigin, setPosOrigin, getPosOriginText, SET_LIST, "Set the origin for Local coorinates" },

	{ "Layouts", 0, 0, 0, false, "" },
	{ "Show Objects", getShowObjects, setShowObjects, 0, SET_VIEW, "Show the objects" },
	{ "Show Icons", getShowIcons, setShowIcons, 0, SET_VIEW,"Show icons instead of models for objects where available" },
	{ "Show Lines", getShowLines, setShowLines, 0, SET_VIEW, "Show the cable and lines" },
	{ "Object Names", get_show_names, set_show_names, 0, SET_VIEW, "Show object names" },
	{ "Layout Info Panel", getShowLayoutInfo, setShowLayoutInfo, 0, SET_VIEW,"Show the realtime status panel for the current sensor overlay" },
	{ "Object Size", getObjectSize, setObjectSize, getObjectSizeText, SET_VIEW |SET_LIST, "Change the scale of the objects and cables" },
//	{"Jiggle", getJiggleFactor, setJiggleFactor, getJiggleText, SET_LIST,"Set jiggle factor when clicking on screen"}

	{ "System", 0, 0, 0, false, "" },
	{ "Check for Update at Start", getCheckForUpdates, setCheckForUpdates, 0, 0,"Check for updates when system starts" },
	{ "Internet Enabled", getInternetEnabled, setInternetEnabled, 0, SET_WARN, "Use the internet to download content" },
	{ "System Testing Menu", getSystemTesting, setSystemTesting, 0, SET_WARN, "Enable the system testing menu items" },
//	{"Server Mode", getServerMode, set_server_mode, 0, SET_WARN, "Force into server mode"},
	{ "Automatic Mesh Recentering", getAutoRecenter, setAutoRecenter, 0, SET_WARN, "Automatically recenter the terrain mesh" },
	{ "Automatic Tile Factor", getAutoTileFactor, setAutoTileFactor, 0, SET_WARN, "Automatically recenter the terrain mesh" },
	{ "Hide Terrain Tile Seams", getHideTileSeams, setHideTileSeams, 0, SET_VIEW | SET_WARN, "Hide the seams between tiles" },
	{ "Texture Compression", getCompressTextures, setCompressTextures, 0, SET_WARN,	"Use texture compression when possible" },
	{ "", NULL, NULL, 0, false, "" }
};

/* =============================================================================
 =============================================================================== */
string makeTag(string pText)
{
	string	strRet = pText;
	for (int i = 0; i < pText.size(); i++)
		if (pText[i] == ' ') strRet[i] = '_';
	return strRet;
}

/* =============================================================================
 =============================================================================== */
bool CSettings::serialize_Node(XMLNode &xn1, bool bView, bool bFile)
{
	XMLNode xn2, xn3;
	char	buf[256];

	//  load global values in the settings dialog list
	for (int i = 0; prefs[i].pLabel.size(); i++)
	{
		if (!prefs[i].pGet)
			continue;
		if (bView && !(prefs[i].iFlags & SET_VIEW))			//  skip this if not a view setting and getting view
			continue;
		xn2 = xn1.addChild(makeTag(prefs[i].pLabel).c_str());
		if (prefs[i].iFlags & SET_LIST && prefs[i].pGetList) //  this is a dropdown
			xn2.addText(prefs[i].pGetList(prefs[i].pGet()).c_str());
		else if (prefs[i].iFlags & SET_COLOR)
		{
			char	buf[64];
			sprintf(buf, "0x%x", prefs[i].pGet());
			xn2.addText(buf);
		}
		else
			//  otherwise this is a checkbox
			xn2.addText(prefs[i].pGet() == 1 ? "true" : "false");
	}

	sprintf(buf, "%f", g_Draw.getLightDir());
	xn2 = xn1.addChild("LightDir");
	xn2.addText(buf);
	sprintf(buf, "%f", g_Draw.getLightAngle());
	xn2 = xn1.addChild("LightAngle");
	xn2.addText(buf);
	sprintf(buf, "%f", g_World.getShading());
	xn2 = xn1.addChild("LightLevel");
	xn2.addText(buf);

	if (bView)
		return true;

	int x, y, w, h;
	g_Set.getScreenPos(x, y, w, h);
	sprintf(buf, "%i %i %i %i", x, y, w, h);
	xn2 = xn1.addChild("ScreenPos");
	xn2.addText(buf);

	double	dx0, dy0, dx1, dy1;
	string	Path;
	g_Draw.getLocater().getData(Path, dx0, dy0, dx1, dy1);
	sprintf(buf, "%lf %lf %lf %lf", dx0, dy0, dx1, dy1);
	xn2 = xn1.addChild("LocaterPos");
	xn2.addText(buf);
	xn2 = xn1.addChild("LocaterFile");
	xn2.addText(Path.c_str());

	if (!bFile)
		return true;

	//  now load values not in the settings dialog
	xn2 = xn1.addChild("CacheFolder");
	xn2.addText(g_Env.m_LocalCachePath.c_str());
	xn2 = xn1.addChild("DataServer");
	xn2.addText(g_Env.m_COVEServerPath.c_str());
	xn2 = xn1.addChild("WorkflowServer");
	xn2.addText(g_Env.m_TridentServerPath.c_str());

	xn2 = xn1.addChild("UserName");
	xn2.addText(obscureString(g_Set.m_strUserName).c_str());
	xn2 = xn1.addChild("Password");
	xn2.addText(obscureString(g_Set.m_strPassword).c_str());
	xn2 = xn1.addChild("StartupFile");
	xn2.addText(g_World.getLink().c_str());
	xn2 = xn1.addChild("RecentFiles");
	xn2.addText(g_World.getLink().c_str());
	for (int i = 0; i < g_Set.m_RecentFiles.size(); i++)
	{
		xn3 = xn2.addChild("File");
		xn3.addText(g_Set.m_RecentFiles[i].c_str());
	}

	xn2 = xn1.addChild("Version");
	xn2.addText(g_Set.m_strVersion.c_str());
	xn2 = xn1.addChild("GLVersion");
	xn2.addText(g_Set.m_strGLVersion.c_str());
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CSettings::deserialize_Node(XMLNode &xn1, bool bFile)
{
	XMLNode		xn2, xn3;
	const char	*pchVal;

	//safety check to make sure someone isn't stuck in map view accidentally
	g_Set.m_MapView = false;

	//  load global values in the settings dialog list
	for (int i = 0; prefs[i].pLabel.size(); i++)
	{
		if (!prefs[i].pGet)
			continue;
		xn2 = xn1.getChildNode(makeTag(prefs[i].pLabel).c_str());
		if (xn2.isEmpty())
			continue;
		if (prefs[i].iFlags & SET_LIST && prefs[i].pGetList)
		{
			pchVal = xn2.getText();
			if (pchVal)
			{
				for (int j = 0; prefs[i].pGetList(j).size(); j++)
					if (lwrstr(prefs[i].pGetList(j)) == lwrstr(pchVal))
					{
						prefs[i].pSet(j);
						break;
					}
			}
		}
		else if (prefs[i].iFlags & SET_COLOR)
		{
			pchVal = xn2.getText();

			color	clr;
			if (sscanf(pchVal, "%x", &clr) == 1) 
				prefs[i].pSet((int) (clr));
		}
		else	//  otherwise this is true/false
		{
			pchVal = xn2.getText();
			if (pchVal) 
				prefs[i].pSet((lwrstr(pchVal) == "true") ? 1 : 0);
		}
	}

	//  now load values not in the settings dialog
	int x, y, w, h;
	xn2 = xn1.getChildNode("ScreenPos");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal && sscanf(pchVal, "%i %i %i %i", &x, &y, &w, &h) == 4) 
			g_Set.setScreenPos(x, y, w, h);
	}

	double	dx0, dy0, dx1, dy1;
	xn2 = xn1.getChildNode("LocaterPos");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) sscanf(pchVal, "%lf %lf %lf %lf", &dx0, &dy0, &dx1, &dy1);
	}

	xn2 = xn1.getChildNode("LocaterFile");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Draw.getLocater().loadTexture(pchVal, dx0, dy0, dx1, dy1);
	}

	float	fVal;
	xn2 = xn1.getChildNode("LightDir");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) sscanf(pchVal, "%f", &fVal);
		g_Draw.setLightDir(fVal);
	}

	xn2 = xn1.getChildNode("LightAngle");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) sscanf(pchVal, "%f", &fVal);
		g_Draw.setLightAngle(fVal);
	}

	xn2 = xn1.getChildNode("LightLevel");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) sscanf(pchVal, "%f", &fVal);
		g_World.setShading(fVal);
	}

	if (!bFile)
		return true;

	xn2 = xn1.getChildNode("CacheFolder");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Env.m_LocalCachePath = pchVal;
	}

	xn2 = xn1.getChildNode("DataServer");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Env.m_COVEServerPath = pchVal;
	}

	xn2 = xn1.getChildNode("WorkflowServer");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Env.m_TridentServerPath = pchVal;
	}

	xn2 = xn1.getChildNode("UserName");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Set.m_strUserName = unobscureString(pchVal);
	}

	xn2 = xn1.getChildNode("Password");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Set.m_strPassword = unobscureString(pchVal);
	}

	xn2 = xn1.getChildNode("StartupFile");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Set.m_StartupFile = pchVal;
	}

	xn2 = xn1.getChildNode("RecentFiles");

	int nCnt = xn2.nChildNode("File");
	g_Set.m_RecentFiles.clear();
	for (int i = 0; i < nCnt; i++)
	{
		xn3 = xn2.getChildNode("File", i);
		if (!xn3.isEmpty())
		{
			pchVal = xn3.getText();
			if (pchVal && *pchVal) 
				g_Set.m_RecentFiles.push_back(pchVal);
		}
	}

	xn2 = xn1.getChildNode("Version");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Set.m_strVersion = pchVal;
	}

	xn2 = xn1.getChildNode("GLVersion");
	if (!xn2.isEmpty())
	{
		pchVal = xn2.getText();
		if (pchVal) g_Set.m_strGLVersion = pchVal;
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CSettings::deserialize(string strText)
{
	if (strText == "")
		return false;
	XMLResults	Results;
	XMLNode		xn1 = XMLNode::parseString(strText.c_str(), "Settings", &Results);
	if (Results.error != 0)
		return false;
	return deserialize_Node(xn1);
}

/* =============================================================================
 =============================================================================== */
string CSettings::serialize(bool bView)
{
	XMLNode xBase = XMLNode::createXMLTopNode("");
	XMLNode xn1 = xBase.addChild("Settings");
	serialize_Node(xn1, bView);

	const char	*pch = xBase.createXMLString();
	string	strText = pch;
	delete pch;
	return strText;
}

/* =============================================================================
 =============================================================================== */
bool CSettings::readFile(string strPath)
{
	XMLNode xn0, xn1;

	if (strPath == "")
	{
		strPath = g_Env.m_LocalCachePath + "/settings.xml";
//		getWebFile("datasvr/app/settings.xml", strPath);
	}

	if (!fileexists(strPath))
	{
		cout << ("Could not open file " + strPath + ". Using default settings");
		return false;
	}

	XMLResults	pResults;
	xn0 = XMLNode::parseFile(strPath.c_str(), "World", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		cout << ("The settings file is an invalid format. Using default settings");
		return false;
	}

	if (xn0.isEmpty())
		return false;
	xn1 = xn0.getChildNode("Settings");
	if (xn1.isEmpty())
		return false;

	return deserialize_Node(xn1, true);
}

/* =============================================================================
 =============================================================================== */
void CSettings::writeFile(string strPath)
{
	XMLNode xBase, xn0, xn1;

	xBase = XMLNode::createXMLTopNode("");
	xn0 = xBase.addChild("World");
	xn0.addAttribute("version", "1.0");
	xn1 = xn0.addChild("Settings");

	serialize_Node(xn1, false, true);

	//  write the structure to a file
	if (strPath == "") 
		strPath = g_Env.m_LocalCachePath + "/settings.xml";
	xBase.writeToFile(strPath.c_str());
	if (g_Env.m_Verbose) 
		cout << ("File saved: " + strPath);
}
