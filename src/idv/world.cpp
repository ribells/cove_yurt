/* =============================================================================
	File: world.cpp

 =============================================================================== */

#include "utility.h"
#include "world.h"
#include "gl_draw.h"
#include "settings.h"

CWorld				g_World;
CWorld				*g_DBFilesWorld = NULL;


string				w_tag[] =
{
	"World",
	"DataSets",
	"Terrain",
	"Textures",
	"Layouts",
	"Views",
	"VisScripts",
	"Objects",
	"Lines",
	"ObjectTypes",
	"LineTypes",
	"Templates",
	"Settings",
	"DrawView",
	"Custom"
};

/* =============================================================================
 =============================================================================== */

int CWorld::getLayerSetType(string strType) const
{
	for (int i = 0; i < ID_MAX; i++)
		if (w_tag[i] == strType)
			return i;
	return -1;
}

/* =============================================================================
 =============================================================================== */
string CWorld::getLayerSetName(int i) const
{
	return i == 0 ? "Worlds" : ((i > 0 && i < ID_MAX) ? w_tag[i] : "");
}

/* =============================================================================
 =============================================================================== */
int CWorld::getRecType(string strType) const
{
	for (int i = 0; i < ID_OBJMAX; i++)
		if (w_tag[i] == strType)
			return i;
	return -1;
}

/* =============================================================================
 =============================================================================== */
string CWorld::getRecTypeName(int i) const
{
	return(i >= 0 && i < ID_OBJMAX) ? w_tag[i] : "";
}

static string	w_longtag[] = { "COVE Files", "Data Sets", "Bathymetry", "Scene Images", "Overlays", "Views", "" };

/* =============================================================================
 =============================================================================== */
int CWorld::getLayerSetTypeLong(string strType) const
{
	for (int i = 0; i < ID_MAX; i++)
		if (w_longtag[i] == strType)
			return i;
	return -1;
}

/* =============================================================================
 =============================================================================== */
string CWorld::getLayerSetLongName(int i) const
{
	return(i >= 0 && i < ID_MAX) ? w_longtag[i] : "";
}

/* =============================================================================
 =============================================================================== */
void CWorld::gotoDefaultCameraView()
{
	Vec3d	min_pos = getTerrain().getTerrainSet().getMinPos();
	Vec3d	max_pos = getTerrain().getTerrainSet().getMaxPos();
	g_Draw.gotoCameraViewRange(min_pos, max_pos);
}

/* =============================================================================
 =============================================================================== */
void CWorld::gotoViewTime(CView &View)
{
	double	start = 0, finish = 0, curtime = 0;
	curtime = View.getTime();
	start = View.getStart();
	finish = View.getFinish();
	if (finish == NO_TIME) finish = start;
	if (start == NO_TIME) getDataSet().getTimeBounds(start, finish);

	double	selstart = View.getSelStart() != NO_TIME && View.getSelStart() >= start ? View.getSelStart() : start;
	double	selfinish = View.getSelFinish() != NO_TIME && View.getSelFinish() <= finish ? View.getSelFinish() : finish;
	getTimeLine().setStart(start);
	getTimeLine().setFinish(finish);
	getTimeLine().setSelStart(selstart);
	getTimeLine().setSelFinish(selfinish);
	getTimeLine().setLead(View.getLead());
	getTimeLine().setTrail(View.getTrail());
	if (curtime == NO_TIME) curtime = start;
	getTimeLine().setCurTime(curtime);

	getDataSet().setCurTime(curtime);
	getDataSet().setSelStart(selstart);
	getDataSet().setSelFinish(selfinish);
	getDataSet().setLead(View.getLead());
	getDataSet().setTrail(View.getTrail());
	getTimeLine().resetSpeed();
}

/* =============================================================================
 =============================================================================== */
bool CWorld::addTerrain(CTerrainLayer &Terrain, int iNdx)
{
	if (!Terrain.getName().size()) 
		Terrain.setName(getFileNameBase(Terrain.getLink()));
	if (!Terrain.getName().size())
		Terrain.setName("New Terrain");
	if (getTerrainSet().getTerrainLayerIndex(Terrain.getName()) != -1)
	{
		string	str;
		for (int i = 1; getTerrainSet().getTerrainLayerIndex(str = getNewName(Terrain.getName(), i)) != -1; i++);
		Terrain.setName(str);
	}

	bool	bRet = getTerrain().addTerrain(Terrain, iNdx);
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::addTexture(CTextureLayer &Texture, int iNdx)
{
	if (!Texture.getName().size()) 
		Texture.setName(getFileNameBase(Texture.getLink()));
	if (!Texture.getName().size()) 
		Texture.setName("New Image");
	if (getTextureSet().getTextureLayerIndex(Texture.getName()) != -1)
	{
		string	str;
		for (int i = 1; getTextureSet().getTextureLayerIndex(str = getNewName(Texture.getName(), i)) != -1; i++);
		Texture.setName(str);
	}

	if ((Texture.getLink().substr(0, 11) == "datasvr/GIS") && !getDDSTileSupport())
	{
		cout << ("DDS Texture Support unavailable. Changing DDS Tiled set to gradient.");
		Texture.setID("grad");
		Texture.editGradient().addFile("system/gradients/land_tones.bmp", 0, EARTHMAX);
		Texture.editGradient().addFile("system/gradients/sea_1.bmp", EARTHMIN, 0);
		Texture.editGradient().updateMap();
	}

	bool	bRet = getTerrain().addTexture(Texture, iNdx);
	return bRet;
}

bool CWorld::editTexture(CTextureLayer &Texture, int iNdx)
{
	if ((Texture.getLink().substr(0, 11) == "datasvr/GIS") && !getDDSTileSupport())
	{
		cout << ("DDS Texture Support unavailable. Changing DDS Tiled set to gradient.");
		Texture.setID("grad");
		Texture.editGradient().addFile("system/gradients/land_tones.bmp", 0, EARTHMAX);
		Texture.editGradient().addFile("system/gradients/sea_1.bmp", EARTHMIN, 0);
		Texture.editGradient().updateMap();
	}

	bool	bRet = getTerrain().editTexture(Texture, iNdx);
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::addDataLayer(CDataLayer &DataLayer, int iNdx)
{
	if (!DataLayer.getName().size()) 
		DataLayer.setName(getFileNameBase(DataLayer.getLink()));
	if (!DataLayer.getName().size())
		DataLayer.setName("New Dataset");
	if (getDataSet().getDataLayerIndex(DataLayer.getName()) != -1)
	{
		string	str;
		for (int i = 1; getDataSet().getDataLayerIndex(str = getNewName(DataLayer.getName(), i)) != -1; i++);
		DataLayer.setName(str);
	}

	bool	bRet = getDataSet().addDataLayer(DataLayer, iNdx);
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::addLayout(CLayout &Layout, int iNdx)
{
	if (!Layout.getName().size()) 
		Layout.setName(getFileNameBase(Layout.getLink()));
	if (!Layout.getName().size()) 
		Layout.setName("New Layout");
	if (getLayoutSet().getLayoutIndex(Layout.getName()) != -1)
	{
		string	str;
		for (int i = 1; getLayoutSet().getLayoutIndex(str = getNewName(Layout.getName(), i)) != -1; i++);
		Layout.setName(str);
	}

	bool	bRet = getLayoutSet().addLayout(Layout, iNdx);
	return bRet;
}

/* =============================================================================
 =============================================================================== */
string CWorld::getActionName(int iAction) const
{
	string	str[] = { "Error", "Edit", "Add", "Delete", "Move" };
	return iAction < TASK_NONE || iAction >= TASK_MAX ? "" : str[iAction];
}

/* =============================================================================
 =============================================================================== */
int CWorld::getAction(string strAction) const
{
	string	str[] = { "Error", "Edit", "Add", "Delete" };
	for (int i = 1; i <= TASK_MAX; i++)
		if (strAction == str[i])
			return i;
	return 0;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::getRecText(int iTag, int iRec, string &strText)
{
	string	strTag = getRecTypeName(iTag);
	strText = serialize_Record(strTag, iRec, true);
	if (strText == "" && iTag == ID_SETTINGS)
		strText = g_Set.serialize(false);
	if (strText == "" && (iTag == ID_OBJECTS || iTag == ID_LINES) && getEditLayoutValid())
		strText = getEditLayout().serialize_Record(strTag, iRec);
	if (strText == "")
	{
		ostringstream	s;
		s << "!EditStack Error: invalid tag or rec number" + strTag + " : " << iRec;
		cout << (s.str());
		return false;
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::setRecText(int iTag, int iRec, string strIn)
{
	string	strTag = getRecTypeName(iTag);
	bool	bProcessed = false;
	if (!bProcessed && iTag == ID_SETTINGS)
		bProcessed = g_Set.deserialize(strIn);
	if (!bProcessed && (iTag == ID_OBJECTS || iTag == ID_LINES) && getEditLayoutValid())
		bProcessed = getEditLayout().deserialize_Record(strIn, strTag, iRec);
	if (!bProcessed)
	{
		bProcessed = deserialize_Record(strIn, strTag, iRec);
		if (bProcessed && iTag == ID_DRAWVIEW)
		{
			gotoViewTime(m_LoadView);
			g_Draw.gotoCameraView
				(
					m_LoadView.getLookAt(),
					m_LoadView.getElevation(),
					m_LoadView.getAzimuth(),
					m_LoadView.getDolly(),
					0
				);
		}
	}
	return bProcessed;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::delRec(int iTag, int iRec)
{
	bool	bSuccess = false;

	//  layer objects
	if (iTag == ID_OBJECTS)
	{
		if (getEditLayoutValid()) 
			bSuccess = getEditLayout().delObject(iRec);
	}
	else if (iTag == ID_LINES)
	{
		if (getEditLayoutValid())
			bSuccess = getEditLayout().delLine(iRec);
	}

	//  world objects
	else if (iTag == ID_TERRAIN)
		bSuccess = getTerrain().delTerrain(iRec);
	else if (iTag == ID_TEXTURES)
		bSuccess = getTerrain().delTexture(iRec);
	else if (iTag == ID_LAYOUTS)
		bSuccess = getLayoutSet().delLayout(iRec);
	else if (iTag == ID_DATASETS)
		bSuccess = getDataSet().delDataLayer(iRec);
	else if (iTag == ID_VIEWS)
		bSuccess = getViewSet().delView(iRec);
	else if (iTag != ID_SETTINGS)
		cout << (" Invalid edit tag : " + getRecTypeName(iTag));
	return bSuccess;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::moveRec(int iTag, int iRec)
{
	bool	bSuccess = false;

	//  world objects
	if (iTag == ID_TERRAIN)
		bSuccess = getTerrainSet().moveTerrainLayerUp(iRec);
	else if (iTag == ID_TEXTURES)
		bSuccess = getTextureSet().moveTextureLayerUp(iRec);
	else if (iTag == ID_LAYOUTS)
		bSuccess = getLayoutSet().moveLayoutLayerUp(iRec);
	else if (iTag == ID_DATASETS)
		bSuccess = getDataSet().moveDataLayerUp(iRec);
	else if (iTag == ID_VIEWS)
		bSuccess = getViewSet().moveViewLayerUp(iRec);

	return bSuccess;
}

/* =============================================================================
 =============================================================================== */
string CWorld::getID(int iTag) const
{
	if (iTag == ID_OBJECTS || iTag == ID_LINES)
		return getEditLayoutValid() ? getEditLayout().getName() : "ERROR";
	else
		return "";
}

/* =============================================================================
 =============================================================================== */
bool CWorld::insRec(int iTag, int iRec, string strIn)
{
	return setRecText(iTag, iRec | 0x10000, strIn);
}

/* =============================================================================
 =============================================================================== */
void CWorld::addEdit(int iAction, int iTag, int iRec, string pchPrevText, bool bSessionOnly)
{
	string	strOut;
	if (iAction > TASK_NONE && iAction < TASK_MAX)
	{
		bool	bProcessed = (iAction == TASK_DEL) || (iAction == TASK_MOVE) || getRecText(iTag, iRec, strOut);
		if (!bProcessed)
			return;

		//  add to the session stack
		if (g_Set.m_SessionCapture && (g_Set.m_SessionPlayback == PLAY_STOP))
		{
			getSessionStack().addEdit(iAction, iTag, iRec);
			getSessionStack().getCur().setOld(pchPrevText);
			getSessionStack().getCur().setNew(strOut);
		}

		if (!bSessionOnly)	//  add to the undo/redo stack
		{
			getEditStack().popOld();	//  remove items on stack that are on top of this because of redo
			getEditStack().addEdit(iAction, iTag, iRec);
			getEditStack().getCur().setOld(pchPrevText);
			getEditStack().getCur().setNew(strOut);
		}
	}

	if (g_Env.m_Verbose)
	{
		ostringstream	s;
//		s << "User: " << getActionName(iAction) << " " << getRecTypeName(iTag) << "(" << iRec << ")" << endl;
		cout << (s.str());
	}
}

/* =============================================================================
 =============================================================================== */
void CWorld::processEdit(CEdit &Edit, bool bRemove)
{
	int iAction = Edit.getAction();

	if (iAction == TASK_ADD || iAction == TASK_DEL)
	{
		if ((bRemove && iAction == TASK_ADD) || (!bRemove && iAction == TASK_DEL))
		{
			delRec(Edit.getTag(), Edit.getIndex());
			if (g_Set.m_SessionCapture && (g_Set.m_SessionPlayback == PLAY_STOP))
			{
				getSessionStack().addEdit(TASK_DEL, Edit.getTag(), Edit.getIndex());
				getSessionStack().getCur().setOld(iAction == TASK_ADD ? Edit.getNew() : Edit.getOld());
			}
		}
		else
		{
			string	strEditText = bRemove ? Edit.getOld() : Edit.getNew();
			insRec(Edit.getTag(), Edit.getIndex(), strEditText);
			if (g_Set.m_SessionCapture && (g_Set.m_SessionPlayback == PLAY_STOP))
			{
				getSessionStack().addEdit(TASK_ADD, Edit.getTag(), Edit.getIndex());
				getSessionStack().getCur().setNew(iAction == TASK_ADD ? Edit.getNew() : Edit.getOld());
			}
		}
	}
	else if (iAction == TASK_MOVE)	//  always move up
	{
		moveRec(Edit.getTag(), Edit.getIndex());
		if (g_Set.m_SessionCapture && (g_Set.m_SessionPlayback == PLAY_STOP))
			getSessionStack().addEdit(TASK_MOVE, Edit.getTag(), Edit.getIndex());
	}
	else	//  edit
	{
		string	strEditTextOld = bRemove ? Edit.getNew() : Edit.getOld();
		string	strEditTextNew = bRemove ? Edit.getOld() : Edit.getNew();
		setRecText(Edit.getTag(), Edit.getIndex(), strEditTextNew);
		if (g_Set.m_SessionCapture && (g_Set.m_SessionPlayback == PLAY_STOP))
		{
			getSessionStack().addEdit(TASK_EDIT, Edit.getTag(), Edit.getIndex());
			getSessionStack().getCur().setOld(strEditTextOld);
			getSessionStack().getCur().setNew(strEditTextNew);
		}
	}
}

/* =============================================================================
 =============================================================================== */
bool CWorld::canRedo() const
{
	return g_Set.m_SessionPlayback ? getSessionStack().canRedo() : getEditStack().canRedo();
}

/* =============================================================================
 =============================================================================== */
bool CWorld::canUndo() const
{
	return g_Set.m_SessionPlayback ? getSessionStack().canUndo() : getEditStack().canUndo();
}

/* =============================================================================
 =============================================================================== */
void CWorld::undo()
{
	if (!canUndo())
		return;

	CEditStack	&Stack = g_Set.m_SessionPlayback ? getSessionStack() : getEditStack();
	bool		bLoop = (Stack.getCur().getGroup() == END_GROUP);
	bool		bEndLoop = false;
	do
	{
		bEndLoop = (Stack.getCur().getGroup() == START_GROUP);
		processEdit(Stack.getCur(), true);
		Stack.undo();
	} while(bLoop && !bEndLoop);
}

/* =============================================================================
 =============================================================================== */
void CWorld::redo()
{
	if (!canRedo())
		return;

	CEditStack	&Stack = g_Set.m_SessionPlayback ? getSessionStack() : getEditStack();
	Stack.redo();

	bool	bLoop = (Stack.getCur().getGroup() == START_GROUP);
	Stack.undo();

	bool	bEndLoop = false;
	do
	{
		Stack.redo();
		bEndLoop = (Stack.getCur().getGroup() == END_GROUP);
		processEdit(Stack.getCur(), false);
	} while(bLoop && !bEndLoop);
}

/* =============================================================================
 =============================================================================== */
bool CWorld::getSaveNeeded() const
{
	if (!m_EditStack->getSaveNeeded())
		return false;

	bool	bSave = false;
	for (int i = m_EditStack->getIndex(); i != -1 && !bSave; i = m_EditStack->getLastEdit(i))
	{
		if (m_EditStack->getEdit(i).getTag() != ID_SETTINGS) 
			bSave = true;
	}

	return bSave;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::gotoView(int ndx, float fTime, bool bUpdateCamera)
{
	if (ndx == -1) ndx = getViewSet().getCur();
	if (!getViewSet().isValid(ndx))
		return false;

	getViewSet().setCur(ndx);

	CView	&View = getView(ndx);
	applyViewActiveLayers(View.getActiveLayers());
	g_Set.deserialize(View.getSettings());

	//  after all data is loaded so we can set up the timeline
	gotoViewTime(View);
	g_Set.m_ShowTimeline = (getDataSet().getActiveCnt() > 0 || getTextureSet().getMovieActive());
	g_World.updateTerrain();
	g_Draw.drawGL();

	if (bUpdateCamera)
		g_Draw.gotoCameraView(View.getLookAt(), View.getElevation(), View.getAzimuth(), View.getDolly(), fTime);

	return true;
}

/* =============================================================================
 =============================================================================== */
bool CWorld::setNewView(string strView, double &dStart, double &dFinish, bool bUpdateCamera)
{
	if (strView == "")
		return true;

	int iView = -1;
	if (strView[0] >= '0' && strView[0] <= '9')
		iView = atoi(strView.c_str());
	else
		iView = getViewSet().getView(strView);

	if (!getViewSet().isValid(iView))
	{
		cout << ("Unable to find view: " + strView);
		return false;
	}

	if (iView != getViewSet().getCur()) 
		gotoView(iView, 0, bUpdateCamera);

	dStart = getTimeLine().getStart();
	dFinish = getTimeLine().getFinish();
	if (dStart > 0.0)
	{
		getTimeLine().setStart(dStart);
		getTimeLine().setCurTime(dStart);
		if (dFinish > 0.0 || getTimeLine().getFinish() < dStart)
		{
			dFinish = max(dStart, dFinish);
			getTimeLine().setFinish(dFinish);
		}

		updateTime();
	}

	return true;
}
