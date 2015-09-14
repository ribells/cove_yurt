#pragma once

#include "xmlParser.h"
#include "terrain.h"
#include "layer.h"
#include "layout.h"
#include "data_layer_set.h"
#include "settings.h"

#include <time.h>
#include <string>
#include <iostream>

//==========[ class CWorld ]==================================================

class CVisScript
{
public:
	string	m_Name;

	bool	m_Movie;
	bool	m_ImageLayers;
	int		m_MovieSpeed; // fps
	int		m_MovieTime;
	bool	m_MovieRotate;
	bool	m_WorldRotate;
	int		m_RotateSpeed; // .01 degrees/sec
	bool	m_MovieTransition;
	bool	m_MovieTrans_Fade;
	bool	m_MovieTrans_Camera;
	int		m_MovieTransTime;
	bool	m_MovieJPEGS;
	int		m_Quality;
	bool	m_CurScreenSize;
	int		m_Width;
	int		m_Height;
	bool	m_UpdateCamera;
	bool	m_WaterMark;
	bool	m_TimeStamp;
	vector<string> m_Views;

public:
	CVisScript() 
	{
		m_Movie = false;
		m_ImageLayers = false;
		m_MovieSpeed = 30;
		m_MovieTime = 5;
		m_MovieRotate = false;
		m_WorldRotate = false;
		m_RotateSpeed = 10;
		m_MovieTransition = true;
		m_MovieTrans_Fade = true;
		m_MovieTrans_Camera = true;
		m_MovieTransTime = 2;
		m_MovieJPEGS = false;
		m_Quality = 100;
		m_CurScreenSize = true;
		m_Width = 640;
		m_Height = 480;
		m_UpdateCamera = true;
		m_WaterMark = true;
		m_TimeStamp = true;
		m_Views.clear();
	}
	~CVisScript() {}

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
};

class CVisSet
{
private:
	vector<CVisScript> m_Vis;
	int		m_Cur;

public:

	CVisSet() : m_Cur(-1) {}
	~CVisSet() {}

	int getVis(string name) const { 
		for (int i = 0; i < m_Vis.size(); i++)
			if (name == m_Vis[i].getName())
				return i;
		return -1;
	}
	int getCnt() const { return m_Vis.size(); }
	int getCur() const { return m_Cur; }
	void setCur(int idx) { m_Cur = (idx < 0 || idx >= m_Vis.size()) ? -1 : idx ; }

	bool isValid(int i) const { return (i >= 0 && i < getCnt()); }
	CVisScript & getVisScript(int i) const { assert(isValid(i)); return const_cast<CVisScript &>(m_Vis[i]); }

	bool addVisScript(CVisScript & Vis, int iNdx = -1)
	{
		if (Vis.getName() == "")
			Vis.setName("Vis");
		if (getVis(Vis.getName()) != -1)
		{
			string str;
			for (int i = 1; getVis(str = getNewName(Vis.getName(), i)) != -1; i++);
			Vis.setName(str);
		}
		if (!isValid(iNdx))
			m_Vis.push_back(Vis);
		else
			m_Vis.insert(m_Vis.begin()+iNdx, Vis);
		return true;
	}
	bool editVisScript(CVisScript & Vis, int iNdx)
	{
		if (!isValid(iNdx))
			return false;
		m_Vis[iNdx] = Vis;
		return true;
	}

	bool delVisScript(int iNdx)
	{ 
		if (!isValid(iNdx))
			return false;
		m_Vis.erase( m_Vis.begin()+iNdx ); 
		if (m_Cur > iNdx)
			m_Cur--;
		return true;
	}
};


class CView : public CLayer
{
private:
	Vec3d	m_LookAt;
	double	m_Elevation;
	double	m_Azimuth;
	double	m_Dolly;

	string	m_ActiveLayers;
	string	m_Settings;

	bool	m_UseDur;
	double	m_Duration;

	double	m_Start;
	double	m_Finish;
	double	m_SelStart;
	double	m_SelFinish;
	double	m_Lead;
	double	m_Trail;
	double	m_Time;

public:
	CView() : m_Elevation(0), m_Azimuth(0), m_Dolly(0), m_UseDur(false), m_Duration(20), 
		m_Start(NO_TIME), m_Finish(NO_TIME), m_SelStart(NO_TIME), m_SelFinish(NO_TIME), 
		m_Lead(NO_TIME), m_Trail(0), m_Time(NO_TIME) 
	{ 
		m_LookAt = vl_0;
		setActive(true);
	}
	~CView() {}

	CView(CView const &t)
	{
		*this = t;
	}
	CView & operator = (const CView &t)
	{
		CLayer::operator =(t);
		m_LookAt = t.m_LookAt;
		m_Elevation = t.m_Elevation;
		m_Azimuth = t.m_Azimuth;
		m_Dolly = t.m_Dolly;

		m_ActiveLayers = t.m_ActiveLayers;
		m_Settings = t.m_Settings;

		m_UseDur = t.m_UseDur;
		m_Duration = t.m_Duration;

		m_Start = t.m_Start;
		m_Finish = t.m_Finish;
		m_SelStart = t.m_SelStart;
		m_SelFinish = t.m_SelFinish;
		m_Lead = t.m_Lead;
		m_Trail = t.m_Trail;
		m_Time = t.m_Time;
		return *this;
	}

	string getSettings() const { return m_Settings; }
	void setSettings(string str) { m_Settings = str; }

	string getActiveLayers() const { return m_ActiveLayers; }
	void setActiveLayers(string str) { m_ActiveLayers = str; }

	Vec3d getLookAt() const { return m_LookAt; }
	void setLookAt(Vec3d pos) { m_LookAt = pos; }
	double getElevation() const { return m_Elevation; }
	void setElevation(double d) { m_Elevation = d; }
	double getAzimuth() const { return m_Azimuth; }
	void setAzimuth(double d) { m_Azimuth = d; }
	double getDolly() const { return m_Dolly; }
	void setDolly(double d) { m_Dolly = d; }

	bool getUseDur() const { return m_UseDur; }
	void setUseDur(bool b) { m_UseDur = b; }
	double getDuration() const { return m_Duration; }
	void setDuration(double d) { m_Duration = d; }

	double getStart() const { return m_Start; }
	void setStart(double d) { m_Start = d; }
	double getFinish() const { return m_Finish; }
	void setFinish(double d) { m_Finish = d; }
	double getSelStart() const { return m_SelStart; }
	void setSelStart(double d) { m_SelStart = d; }
	double getSelFinish() const { return m_SelFinish; }
	void setSelFinish(double d) { m_SelFinish = d; }

	double getLead() const { return m_Lead; }
	void setLead(double d) { m_Lead = d; }
	double getTrail() const { return m_Trail; }
	void setTrail(double d) { m_Trail = d; }
	double getTime() const { return m_Time; }
	void setTime(double d) { m_Time = d; }
};



class CViewSet
{
private:
	vector<CView> m_Views;
	int		m_Cur;
	bool	m_Dirty;
	bool	m_Rebuild;

public:

	CViewSet() : m_Cur(-1), m_Dirty(true), m_Rebuild(true) {}
	~CViewSet() {}

	int getView(string name) const { 
		for (int i = 0; i < m_Views.size(); i++)
			if (name == m_Views[i].getName())
				return i;
		return -1;
	}
	int getCnt() const { return m_Views.size(); }
	int getCur() const { return m_Cur; }
	void setCur(int idx)
	{	int size = (int)m_Views.size();	m_Cur = (size==0 ? -1 : idx < 0 ? size-1 : idx%size); }

	bool getDirty() const { return m_Dirty; }
	void setDirty(bool b) { m_Dirty = b; }
	bool getRebuild() const { return m_Rebuild; }
	void setRebuild(bool b) { m_Rebuild = b; }

	bool isValid(int i) const { return (i >= 0 && i < getCnt()); }
	CView & getView(int i) const { assert(isValid(i)); return const_cast<CView &>(m_Views[i]); }

	bool addView(CView & View, int iNdx = -1)
	{
		if (!View.getName().size())
			View.setName("View");
		if (getView(View.getName()) != -1)
		{
			string str;
			for (int i = 1; getView(str = getNewName(View.getName(), i)) != -1; i++);
			View.setName(str);
		}
		if (!isValid(iNdx))
			m_Views.push_back(View);
		else
			m_Views.insert(m_Views.begin()+iNdx, View);
		//		setDirty(true);
		return true;
	}

	bool editView(CView & View, int iNdx)
	{
		if (!isValid(iNdx))
			return false;
		string strOldLink = m_Views[iNdx].getLink();
		m_Views[iNdx] = View;
		if (strOldLink != m_Views[iNdx].getLink())
			setDirty(true);
		return true;
	}

	bool delView(int iNdx)
	{ 
		if (!isValid(iNdx))
			return false;
		m_Views.erase( m_Views.begin()+iNdx ); 
		if (m_Cur > iNdx)
			m_Cur--;
		setDirty(true);
		return true;
	}

	bool moveViewLayerUp(int iNdx)
	{ 
		if (!isValid(iNdx) || iNdx == 0)
			return false;
		CView tmp = m_Views[iNdx];
		m_Views[iNdx] = m_Views[iNdx-1];
		m_Views[iNdx-1] = tmp;
		setDirty(true);
		return true;
	}

	void updateView(int iNdx, Vec3d lookat, double elev, double azim, double dolly, string name = "")
	{
		if (!isValid(iNdx))
			return;
		m_Views[iNdx].setLookAt(lookat);
		m_Views[iNdx].setElevation(elev);
		m_Views[iNdx].setAzimuth(azim);
		m_Views[iNdx].setDolly(dolly);
		if (name!="")
			m_Views[iNdx].setName(name);
	}

	void updateViewTime(int iNdx, double start, double finish, double selstart, double selfinish, double lead, double trail, double curtime)
	{
		if (!isValid(iNdx))
			return;
		m_Views[iNdx].setStart(start);
		m_Views[iNdx].setFinish(finish);
		m_Views[iNdx].setSelStart(selstart);
		m_Views[iNdx].setSelFinish(selfinish);
		m_Views[iNdx].setLead(lead);
		m_Views[iNdx].setTrail(trail);
		m_Views[iNdx].setTime(curtime);
	}
};

class CWorldLink : public CLayer
{
public:
	bool setActive(bool b) { return false; }
};

class CLayoutTypeLink : public CLayer
{
public:
	bool setActive(bool b) { return false; }
};

enum { NO_GROUP, START_GROUP, END_GROUP};
enum { TASK_NONE, TASK_EDIT, TASK_ADD, TASK_DEL, TASK_MOVE, TASK_MAX};

class CEdit
{
private:
	int		m_Action;
	int		m_Tag;
	int		m_Index;

	string	m_Old;
	string	m_New;
	long	m_Time;

	int		m_Group;

public:
	CEdit(): m_Action(TASK_NONE), m_Index(0), m_Time(0), m_Group(0), m_Tag(-1) {}
	CEdit(int iAction, int iTag, int iNdx, long lTime): m_Index(0), m_Time(0), m_Group(0)
	{
		m_Action = iAction;
		m_Tag = iTag;
		m_Index = iNdx;
		if (lTime == 0)
		{
			time_t tm;
			time( &tm );
			m_Time = tm;
		}
		else
			m_Time = lTime;
	}
	~CEdit() {}

	int getAction() const { return m_Action; }
	void setAction(int i) { m_Action = i; }
	int getTag() const { return m_Tag; }
	void setTag(int i) { m_Tag = i; }
	string getOld() const { return m_Old; }
	void setOld(string str) { m_Old = str; }
	string getNew() const { return m_New; }
	void setNew(string str) { m_New = str; }

	int getGroup() const { return m_Group; }
	void setGroup(int i) { m_Group = i; }
	void toggleGroup(int i) { m_Group = (m_Group ? 0 : i); }

	int getIndex() const {return m_Index;}
	void setIndex(int i) { m_Index = i;}
	long getTime() const {return m_Time;}
	void setTime(long t) { m_Time = t;}
};

class CEditStack
{
private:
	vector<CEdit> m_Stack;
	int		m_Index;
	bool	m_GroupEdits;
	int		m_LastSave;

public:
	CEditStack(): m_Index(-1), m_LastSave(-1), m_GroupEdits(false) {}
	~CEditStack() {}

	int getSize() const { return m_Stack.size(); }
	int getIndex() const { return m_Index; }
	void setIndex(int i) { m_Index = i; }
	bool getSaveNeeded() const { return (m_Index != m_LastSave); }
	void setSaved() { m_LastSave = m_Index; }
	bool getIsValid(int i) const { return (i >= 0 && i < getSize()); }

	bool getLastSave() const { return m_LastSave; }
	int getLastEdit(int ndx) const
	{
		if (ndx == m_LastSave) 
			return -1;
		ndx += (m_Index > m_LastSave ? -1 : 1);
		return !getIsValid(ndx) || ndx == m_LastSave ? -1 : ndx;
	}

	void popOld()
	{
		for (int i = m_Stack.size()-1; i > m_Index; i--)
			m_Stack.pop_back();
	}

	bool canUndo() const { return m_Index >= 0; }
	void undo() { assert(canUndo());  m_Index--; }
	bool canRedo() const { return getSize() > 0 && m_Index < getSize()-1; }
	void redo() { assert(canRedo());  m_Index++; }

	void addEdit(int iAction, int iTag, int index, long lTime = 0)
	{
		CEdit Edit(iAction, iTag, index, lTime);
		for (int i = m_Stack.size()-1; i > m_Index; i--)
			m_Stack.erase(m_Stack.begin()+i);
		if (m_GroupEdits)
		{
			Edit.toggleGroup(START_GROUP);
			m_GroupEdits = false;
		}
		m_Stack.push_back(Edit);
		m_Index++;
	}
	void addEdit(CEdit &Edit)
	{
		int idx = 0;
		for (int i = 0; i < m_Stack.size(); i++, idx++)
			if (Edit.getTime() < m_Stack[idx].getTime())
				break;
		if (idx == m_Stack.size())
			m_Stack.push_back(Edit);
		else
			m_Stack.insert(m_Stack.begin()+idx, Edit);
	}

	void startGroup() { m_GroupEdits = true; }
	void endGroup() { if (getIsValid(m_Index)) m_Stack[m_Index].toggleGroup(END_GROUP); }

	CEdit & getEdit(int i) { assert(canUndo()); return m_Stack[i]; }
	CEdit & getCur() { assert(canUndo()); return m_Stack[m_Index]; }
	CEdit & getNext() { assert(canRedo()); return m_Stack[m_Index+1]; }
};

enum {ID_NONE = -1, ID_WORLD, ID_DATASETS, ID_TERRAIN, ID_TEXTURES, ID_LAYOUTS, ID_MAX};
enum {ID_VIEWS = ID_MAX, ID_VISSCRIPTS, ID_OBJECTS, ID_LINES, ID_OBJECT_TYPES, ID_LINE_TYPES, ID_TEMPLATES, ID_SETTINGS, ID_DRAWVIEW, ID_CUSTOM, ID_OBJMAX};


class CWorld : public CLayer
{
private:
	bool	m_Invalid;	//invalid at startup and when loading
	bool	m_Hide;
	bool	m_Dirty;
	bool	m_IncludeSettings;
	bool	m_IncludeFiles;
	bool	m_IncludeOnlyActive;

	bool	m_LoadViewActive;
	CView	m_LoadView;
	string	m_CurViewName;
	string	m_CurVisName;

	string	m_LastError;

	vector<CWorldLink> m_WorldLinks;
	vector<CLayoutTypeLink> m_LayoutTypeLinks;

	CTerrain		* m_Terrain;
	CLayoutTypes	* m_LayoutTypes;
	CLayoutSet		* m_LayoutSet;
	CDataSet		* m_DataSet;

	CTimeLine		* m_TimeLine;
	CVisSet			* m_VisSet;
	CViewSet		* m_ViewSet;
	CEditStack		* m_EditStack;
	CEditStack		* m_SessionStack;

public:

	CWorld()
	{
		m_Hide = false;
		m_Invalid = true;
		m_Dirty = false;
		m_IncludeSettings = true;
		m_IncludeFiles = false;
		m_IncludeOnlyActive = false;
		m_LoadViewActive = false;
		m_Terrain = new CTerrain();
		m_LayoutTypes = new CLayoutTypes();
		m_LayoutSet = new CLayoutSet(getLayoutTypesPtr());
		m_DataSet = new CDataSet();
		m_TimeLine = new CTimeLine();
		m_VisSet = new CVisSet();
		m_ViewSet = new CViewSet();
		m_EditStack = new CEditStack();
		m_SessionStack = new CEditStack();
	}

	~CWorld()
	{ 
		delete m_Terrain;
		delete m_LayoutTypes;
		delete m_LayoutSet;
		delete m_DataSet;
		delete m_TimeLine;
		delete m_VisSet;
		delete m_ViewSet;
		delete m_EditStack;
		delete m_SessionStack;
	}

	void clean()
	{
		getTimeLine().setPlay(false);
		//getTimeLine().updateBounds(0,0);
		//cache settings
		t_scale Scale = getScale();
		float fShading = getShading();

		delete m_Terrain;
		m_Terrain = new CTerrain();

		delete m_DataSet;
		m_DataSet = new CDataSet();

		//reset terrain settings
		setShading(fShading);
		setScale(Scale);

		delete m_LayoutTypes;
		m_LayoutTypes = new CLayoutTypes();
		delete m_LayoutSet;
		m_LayoutSet = new CLayoutSet(getLayoutTypesPtr());

		delete m_TimeLine;
		m_TimeLine = new CTimeLine();
		delete m_VisSet;
		m_VisSet = new CVisSet();
		delete m_ViewSet;
		m_ViewSet = new CViewSet();
		delete m_EditStack;
		m_EditStack = new CEditStack();
		delete m_SessionStack;
		m_SessionStack = new CEditStack();

		m_WorldLinks.clear();
		m_LayoutTypeLinks.clear();

		m_LoadViewActive = false;
		m_LoadView = CView();
		m_CurViewName = "";
		m_CurVisName = "";
		setDirty(false);
	}

	//PROPERTIES
	bool getHide() const { return m_Hide; }
	void setHide(bool b) { m_Hide = b; }
	bool getInvalid() const { return m_Invalid; }
	void setInvalid(bool b) { m_Invalid = b; }
	bool getDirty() const { return m_Dirty; }
	void setDirty(bool b) { m_Dirty = b; }

	bool getIncludeSettings() const { return m_IncludeSettings; }
	void setIncludeSettings(bool b) { m_IncludeSettings = b; }
	bool getIncludeFiles() const { return m_IncludeFiles; }
	void setIncludeFiles(bool b) { m_IncludeFiles = b; }
	bool getIncludeOnlyActive() const { return m_IncludeOnlyActive; }
	void setIncludeOnlyActive(bool b) { m_IncludeOnlyActive = b; }

	string	getLastError() { return m_LastError; }

	//FILE IO
	bool deserialize_CLayer(XMLNode &xn2, CLayer & Layer);
	bool serialize_CLayer(XMLNode &xn2, CLayer & Layer);

	bool deserialize_Node(XMLNode &xn0, string strTag, int iXmlNdx = 0, int iRecNdx = -1);
	bool serialize_Node(XMLNode &xn0, string strTag, int iRecNdx, bool bUndo = false);

	bool deserialize_Record(string strText, string strTag, int iRecNdx = -1);
	string serialize_Record(string strTag, int iRecNdx, bool bUndo = false);

	bool deserialize(XMLNode &xn0);
	void serialize(XMLNode &xn0);

	bool readCoveFile(string fileName, bool bImport = false);
	bool writeCoveFile(string fileName);

	bool readKMLImageFile(string fileName);
	bool writeKMLImageFile(string fileName, vector<string> strImageFile, vector<Vec3d> minpos, vector<Vec3d> maxpos);

	bool writeViewsFile(string fileName, vector<int> iViews);
	void applyViewActiveLayers(string strText, bool bNoDeactivate = false);

	bool deserialize_ViewLayers(XMLNode &xn0);
	string serialize_ViewLayers(bool bEntireNode = false);

	bool cleanDBFileList();
	string cache_DBFile();

	//LAYOUT TYPES
	CLayoutTypes & getLayoutTypes() const { return * m_LayoutTypes; }
	const CLayoutTypesPtr getLayoutTypesPtr() const { return m_LayoutTypes; }
private:
	void setLayoutTypesPtr(const CLayoutTypesPtr pTypes) { m_LayoutTypes = pTypes; getLayoutSet().setLayoutTypes(pTypes); }
	void clearLayoutTypesPtr() { m_LayoutTypes = NULL; }

public:
	string getTypesLink() const { return getLayoutTypes().getLink(); }
	void setTypesLink(string strLink)
	{
		if (strLink.size()==0)
			return;
		if (getTypesLink() == strLink)
			return;
		if (getCOVELocalPath(getTypesLink()) == strLink)
		{
			m_LayoutTypes->setLink(getCOVELocalPath(getTypesLink()));
			return;
		}

		CLayoutTypesPtr pNewTypes = new CLayoutTypes();
		CLayoutTypesPtr pOldTypes = m_LayoutTypes;
		if (!pNewTypes->readFile(strLink))
		{
			cout << " - NOT loading: layout type file\n";
			return;
		}
		getLayoutSet().setLayoutTypes(pNewTypes);
		m_LayoutTypes = pNewTypes;
		delete pOldTypes;
	}
	bool delLineType(int iNdx)
	{
		CLineTypePtr pType = getLayoutTypes().getLineTypePtr(iNdx);
		for (int i = 0; i < getLayoutSet().getLayoutCnt(); i++)
		{
			CLayout &Layout = getLayoutSet().getLayout(i);
			for (int j = 0; j < Layout.getLineCnt(); j++)
				if (Layout.getLine(j).getLineTypePtr() == pType)
					return false;
		}
		return getLayoutTypes().delLineType(iNdx);
	}
	bool delObjectType(int iNdx)
	{
		CObjectTypePtr pObjType = getLayoutTypes().getObjectTypePtr(iNdx);
		for (int i = 0; i < getLayoutSet().getLayoutCnt(); i++)
		{
			CLayout &Layout = getLayoutSet().getLayout(i);
			for (int j = 0; j < Layout.getObjectCnt(); j++)
				if (Layout.getObject(j).getObjectTypePtr() == pObjType)
					return false;
		}
		return getLayoutTypes().delObjectType(iNdx);
	}

	//DRAWING & VIEWS
	bool getNeedRedraw() { return (getTerrain().getNeedRedraw()) || updateTime(); }
	void gotoDefaultCameraView();
	void gotoViewTime(CView & View);
	void unloadMaterials()
	{
		getLayoutSet().unloadMaterials();
		getDataSet().unloadMaterials();
		getTerrain().unloadMaterials();
	}
	void reloadMaterials()
	{
		getLayoutSet().reloadMaterials();
		getDataSet().reloadMaterials();
	}

	// LAYERSETS
	int getLayerSetType(string strType) const; 
	string getLayerSetName(int i) const;
	int getLayerSetTypeLong(string strType) const; 
	string getLayerSetLongName(int i) const;

	CTerrain & getTerrain() const { return * m_Terrain; }
	CTerrainSet & getTerrainSet() const { return getTerrain().getTerrainSet(); }
	CTerrainLayer & getTerrainLayer(int i) const { return getTerrain().getTerrain(i); }
	CTextureSet & getTextureSet() const { return getTerrain().getTextureSet(); }
	CTextureLayer & getTextureLayer(int i) const { return getTerrain().getTexture(i); }

	bool addTerrain(CTerrainLayer & Terrain, int iNdx = -1);
	bool addTexture(CTextureLayer & Texture, int iNdx = -1);
	bool editTexture(CTextureLayer &Texture, int iNdx);
	void updateTerrain()
	{ 
		if (getTerrain().updateTerrain())
			getLayoutSet().updateLayerPositions();
		getTerrain().updateTextures();
		getDataSet().redrawDataLayers();
	}
	t_scale getScale() const
	{ 
		return getTerrain().getScale();
	}
	void setScale(t_scale scale)
	{ 
		getTerrain().setScale(scale);
		getDataSet().setScale(scale);
	}
	float getShading() const
	{ 
		return getTerrain().getShading();
	}
	void setShading(float fval)
	{ 
		getTerrain().setShading(fval);
		getDataSet().setShading(fval);
	}
	void recenterTerrain(Vec3d pos)
	{
		getTerrain().setMeshCenterGPS(pos);
		getDataSet().redrawDataLayers();
		cout << (" - Updated mesh center to " + getStringFromPosition(getTerrain().getMeshCenterGPS(), true));
	}

	CDataSet & getDataSet() const { return * m_DataSet; }
	bool getDataLayerValid(int i) const { return i >= 0 && i < getDataSet().getDataLayerCnt(); }
	CDataLayer & getDataLayer(int i) const { return getDataSet().getDataLayer(i); }
	bool addDataLayer(CDataLayer & DataLayer, int iNdx = -1);

	CLayoutSet & getLayoutSet() const { return * m_LayoutSet; }
	CLayout & getLayout(int i) const { return getLayoutSet().getLayout(i); }
	CLayout & getEditLayout() const { return getLayoutSet().getEditLayout(); }
	int getEditLayoutNdx() const { return getLayoutSet().getEditLayoutNdx(); }
	int getEditLayoutValid() const { return getLayoutSet().getEditLayoutNdx() != -1; }
	bool addLayout(CLayout & Layout, int iNdx = -1);

	// VIS
	CVisSet & getVisSet() const { return * m_VisSet; }
	CVisScript & getVisScript(int i) const { return getVisSet().getVisScript(i); }
	void clearVisSet() { delete m_VisSet; m_VisSet = new CVisSet(); }

	// VIEWS
	CViewSet & getViewSet() const { return * m_ViewSet; }
	CView & getView(int i) const { return getViewSet().getView(i); }
	bool gotoView(int ndx, float fTime, bool bUpdateCamera);
	bool setNewView(string strView, double &dStart, double &dFinish, bool bUpdateCamera);
	void clearViewSet() { delete m_ViewSet; m_ViewSet = new CViewSet(); }

	// TIMELINE
	CTimeLine & getTimeLine() const { return * m_TimeLine; }
	bool updateTime()
	{ 
		bool bChange = getTimeLine().updateCurTime();
		if (!bChange) 
			return false;
		getDataSet().setCurTime(getTimeLine().getTime(), getTimeLine().getFrameTime());
		return true;
	}

	// WORLD LINKS
	CWorldLink & getWorldLink(int i) const { return  const_cast<CWorldLink &>(m_WorldLinks[i]); }
	int getWorldLinkCnt() const { return m_WorldLinks.size(); }
	bool addWorldLink(CWorldLink WorldLink, int iNdx)
	{
		if (iNdx == -1 || iNdx >= m_WorldLinks.size())
			m_WorldLinks.push_back(WorldLink);
		else
			m_WorldLinks.insert(m_WorldLinks.begin()+iNdx, WorldLink);
		return true;
	}
	bool delWorldLink(int iNdx)
	{ 
		if (iNdx < 0 || iNdx >= m_WorldLinks.size())
			return false;
		vector<CWorldLink> :: iterator iter = m_WorldLinks.begin();
		for (int i = 0; iter != m_WorldLinks.end() && i < iNdx; iter++, i++);
		m_WorldLinks.erase(iter);
		return true;
	}

	// LAYOUT TYPE LINKS
	CLayoutTypeLink & getLayoutTypeLink(int i) const { return  const_cast<CLayoutTypeLink &>(m_LayoutTypeLinks[i]); }
	int getLayoutTypeLinkCnt() const { return m_LayoutTypeLinks.size(); }
	bool addLayoutTypeLink(CLayoutTypeLink LayoutTypeLink, int iNdx)
	{
		if (iNdx == -1 || iNdx >= m_LayoutTypeLinks.size())
			m_LayoutTypeLinks.push_back(LayoutTypeLink);
		else
			m_LayoutTypeLinks.insert(m_LayoutTypeLinks.begin()+iNdx, LayoutTypeLink);
		return true;
	}

	// UNDO & REDO
	CEditStack & getSessionStack() const { return * m_SessionStack; }

	CEditStack & getEditStack() const { return * m_EditStack; }
	int getEditCnt() const { return getEditStack().getSize(); }

	void addEdit(int iAction, int iTag, int iRec, string pchPrevText = "", bool bSessionOnly = false);
	void addEdit(CEdit &Edit, int idx);
	void startEditGroup() { getEditStack().startGroup(); getSessionStack().startGroup(); }
	void endEditGroup() { getEditStack().endGroup(); getSessionStack().endGroup(); }

	bool getRecText(int iTag, int iRec, string &strOut);
	bool setRecText(int iTag, int iRec, string strIn);
	bool delRec(int iTag, int iRec);
	bool moveRec(int iTag, int iRec);
	bool insRec(int iTag, int iRec, string strIn);

	int getRecType(string strType) const; 
	string getRecTypeName(int i) const;

	bool canRedo() const;
	bool canUndo() const;
	void undo();
	void redo();

	string getID(int iTag) const;
	string getActionName(int iAction) const;
	int getAction(string strAction) const;
	bool getSaveNeeded() const;
	bool getEdited() const { return getSaveNeeded() || getDirty(); }
	void setSaved() { getEditStack().setSaved(); setDirty(false);}

	bool serialize_Edit(XMLNode &xn1, int i);
	bool deserialize_Edit(XMLNode &xn1);
	bool writeSessionFile(string fileName);
	bool readSessionFile(string fileName);

	void clearEditStack()
	{
		delete m_EditStack;
		m_EditStack = new CEditStack();
	}
	void clearSessionStack()
	{
		delete m_SessionStack;
		m_SessionStack = new CEditStack();
	}

private:
	void processEdit(CEdit & pEdit, bool bRemove);

};

extern CWorld	g_World;
extern CWorld	*g_DBFilesWorld;
