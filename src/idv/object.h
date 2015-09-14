#pragma once

#include "terrain.h"
#include <string>
#include <sstream>

//==========[ class CObject ]==================================================

enum { PRIM, MODEL };

enum { POS_SEAFLOOR, POS_SURFACE, POS_FLOAT, POS_MOBILE };

class CLineType;

typedef CLineType * CLineTypePtr;

class CSensor
{
private:
	string	m_Name;
	string	m_Description;
	string	m_ID;
	Vec3d	m_Pos;	//in lat/lon/depth format

	string	m_Units;
	int		m_SampleSize;
	double	m_Rate;
	double	m_Range[2];

public:
	CSensor() : m_Rate(0), m_SampleSize(0) 
	{
		m_Range[0] = m_Range[1] = 0;
		m_Pos = Vec3d(0,0,0);
	}
	~CSensor() {}

	CSensor(CSensor const &t)
	{
		*this = t;
	}

	CSensor & operator = (const CSensor &t)
	{
		m_Name = t.m_Name;
		m_Description = t.m_Description;
		m_ID = t.m_ID;
		m_Pos = t.m_Pos;

		m_Units = t.m_Units;
		m_SampleSize = t.m_SampleSize;
		m_Rate = t.m_Rate;
		m_Range[0] = t.m_Range[0];
		m_Range[1] = t.m_Range[1];
		return *this;
	}

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	string getID() const { return m_ID; }
	void setID(string str) { m_ID = str; }
	string getDescription() const { return m_Description; }
	void setDescription(string str) { m_Description = str; }

	string getUnits() const { return m_Units; }
	void setUnits(string str) { m_Units = str; }
	double getRate() const { return m_Rate; }
	void setRate(double f) { m_Rate = f; }
	int getSampleSize() const { return m_SampleSize; }
	void setSampleSize(int i) { m_SampleSize = i; }
	double getRange(int i) const { return m_Range[i]; }
	void setRange(int i, double d) { m_Range[i] = d; }

	Vec3d getPosition() const { return m_Pos; }
	void setPosition(Vec3d pos) { m_Pos = pos; }
};


class CObjectType : public CLayer
{
private:
	string	m_ID;
	string	m_WebPage;

	double	m_Cost;
	double	m_InstallCost;

	double	m_PowerLimit;	//if 0 then a sink, else a power source
	double	m_PowerCost;	//power needed to run system
	double	m_DataLimit;
	double	m_DataCost;

	vector<CSensor> m_SensorList;

	int		m_PrimType;
	int		m_PosType;
	string	m_ModelLink;
	C3DModel m_Model;	//transform stored in model
	string	m_IconLink;
	int		m_IconTexture;  //used for icons

	Vec3d	m_Trans, m_Rot, m_Scale;
	color	m_Color, m_TextColor;
	int		m_TextSize;

	double	m_ViewMin;	//in km
	double	m_ViewMax;
	bool	m_Magnify;
	bool	m_Shrink;

	Vec3f	m_BBoxCenter;
	float	m_BBoxRadius;

public:
	CObjectType()
	{
		m_IconTexture = -1;
		m_PrimType = 2;
		m_Trans = vl_0; m_Rot = vl_0; m_Scale = vl_1;
		m_Color = CLR_WHITE;
		m_TextColor = 0xff30d0d0;
		m_TextSize = 10;
		m_ViewMin = m_ViewMax = -1; //no limit
		m_Magnify = true;
		m_Shrink = false;
		m_BBoxCenter = vl_0;
		m_BBoxRadius = 1.0f;
		m_Cost = 0;
		m_InstallCost = 0;
		m_PosType = POS_SEAFLOOR;
		m_PowerLimit = m_PowerCost = 0;
		m_DataLimit = m_DataCost = 0;
		m_InstallCost = 0;
	}
	~CObjectType()
	{ 
		clearIconTexture();
	}

	CObjectType(CObjectType const &t)
	{
		*this = t;
		editModel().clearDataPtrs();
		m_IconTexture = -1;
	}

	CObjectType & operator = (const CObjectType &t)
	{
		CLayer::operator =(t);
		deleteTexture(m_IconTexture);
		m_Model.clean();

		m_ID = t.m_ID;
		m_WebPage = t.m_WebPage;

		m_Cost = t.m_Cost;
		m_InstallCost = t.m_InstallCost;
		m_PowerLimit = t.m_PowerLimit;
		m_PowerCost = t.m_PowerCost;
		m_DataLimit = t.m_DataLimit;
		m_DataCost = t.m_DataCost;

		m_SensorList = t.m_SensorList;

		m_PrimType = t.m_PrimType;
		m_PosType = t.m_PosType;
		m_ModelLink = t.m_ModelLink;
		m_IconLink = t.m_IconLink;

		m_Trans = t.m_Trans;
		m_Rot = t.m_Rot;
		m_Scale = t.m_Scale;
		m_Color = t.m_Color;
		m_TextColor = t.m_TextColor;
		m_TextSize = t.m_TextSize;

		m_ViewMin = t.m_ViewMin;	//in km
		m_ViewMax = t.m_ViewMax;
		m_Magnify = t.m_Magnify;
		m_Shrink = t.m_Shrink;
		m_BBoxCenter = t.m_BBoxCenter;
		m_BBoxRadius = t.m_BBoxRadius;

		setModel();
		setIcon();
		return *this;
	}

	string getID() const { return m_ID; }
	void setID(string str) { m_ID = str; }
	string getWebPage() const { return m_WebPage; }
	void setWebPage(string str) { m_WebPage = str; }

	string getInfoText() const;

	int getPrimType() const { return m_PrimType; }
	void setPrimType(int i) { m_PrimType = i; }
	void setPrimType(string str) {	m_PrimType = C3DSceneMgr::getPrimType(str);	}

	string getModelLink() const { return m_ModelLink; }
	void setModelLink(string str) { m_ModelLink = str; }
	string getIconLink() const { return m_IconLink; }
	void setIconLink(string str) { m_IconLink = str; }

	void getTransform(Vec3d &t, Vec3d &r, Vec3d &s) const	{ t = m_Trans; r = m_Rot; s = m_Scale; }
	void setTransform(Vec3d t, Vec3d r, Vec3d s)	{
		m_Trans = t; m_Rot = r; m_Scale = s;
		setBoundingSphere();
	}
	Vec3d getScale() const { return m_Scale; }
	color getColor() const { return m_Color; }
	void setColor(color clr) { m_Color = clr; }
	color getTextColor() const { return m_TextColor; }
	void setTextColor(color clr) { m_TextColor = clr; }
	int getTextSize() const { return m_TextSize; }
	void setTextSize(int i) { m_TextSize = i; }

	const C3DModel & getModel() { return m_Model; }
	C3DModel & editModel() { return m_Model; }

	int getIconTexture() const { return m_IconTexture; }
	void clearIconTexture() { deleteTexture(m_IconTexture); m_IconTexture = -1; }
	void setModel();
	void setIcon();

	void Render(bool bEditing, bool bSelected, double dDist, double dScale, int iSelectID, color clr);
	void setBoundingSphere();

	double getCost() const { return m_Cost; }
	void setCost(double f) { m_Cost = f; }
	double getPowerLimit() const { return m_PowerLimit; }
	void setPowerLimit(double f) { m_PowerLimit = f; }
	double getPowerCost() const { return m_PowerCost; }
	void setPowerCost(double f) { m_PowerCost = f; }
	double getDataLimit() const { return m_DataLimit; }
	void setDataLimit(double f) { m_DataLimit = f; }
	double getDataCost() const { return m_DataCost; }
	void setDataCost(double f) { m_DataCost = f; }
	double getInstallCost() const { return m_InstallCost; }
	void setInstallCost(double f) { m_InstallCost = f; }

	Vec3f getBBCenter() const { return m_BBoxCenter; }
	float getBBRadius() const { return m_BBoxRadius; }

	double getViewMin() const { return m_ViewMin; }
	void setViewMin(double val) { m_ViewMin = val; }
	double getViewMax() const { return m_ViewMax; }
	void setViewMax(double val) { m_ViewMax = val; }

	bool getMagnify() const { return m_Magnify; }
	void setMagnify(bool bSet) { m_Magnify = bSet; }
	bool getShrink() const { return m_Shrink; }
	void setShrink(bool bSet) { m_Shrink = bSet; }

	int getPosType() const { return m_PosType; }
	void setPosType(int i) { m_PosType = i;}

	int getSensorCnt() const { return (int)m_SensorList.size(); }
	bool getSensorValid(int i) const { return i >= 0 && i < getSensorCnt(); }

	CSensor & getSensor(int iNdx) const
	{ 
		assert(getSensorValid(iNdx));
		return const_cast<CSensor &>(m_SensorList[iNdx]);
	}
	int getSensorIndex(string id) const
	{ 
		for (int i = 0; i < getSensorCnt(); i++)
			if (id == getSensor(i).getID())
				return i;
		return -1;
	}

	bool addSensor(CSensor & Sensor, int iNdx)
	{
		if (!Sensor.getID().size())
			Sensor.setID("SENS");
		if (getSensorIndex(Sensor.getID()) != -1)
		{
			string str;
			for (int i = 1; getSensorIndex(str = getNewName(Sensor.getID(), i)) != -1; i++);
			Sensor.setID(str);
		}
		if (!Sensor.getName().size())
			Sensor.setName("Sensor " + Sensor.getID());
		if (!getSensorValid(iNdx))
		{
			m_SensorList.push_back(CSensor());
			iNdx = getSensorCnt()-1;
		}
		else
		{
			vector<CSensor> :: iterator iter = m_SensorList.begin();
			for (int i = 0; iter != m_SensorList.end() && i < iNdx; iter++, i++);
			m_SensorList.insert(iter, CSensor());
		}
		return editSensor(Sensor, iNdx);
	}
	bool editSensor(CSensor & Sensor, int iNdx)
	{
		if (!getSensorValid(iNdx))
			return false;
		getSensor(iNdx) = Sensor;
		return true;
	}
	bool delSensor(int iNdx)
	{ 
		if (!getSensorValid(iNdx))
			return false;
		m_SensorList.erase( m_SensorList.begin() + iNdx ); 
		return true;
	}
	void clearSensors()
	{
		while (getSensorCnt())
			delSensor(0);
	}
};

typedef CObjectType * CObjectTypePtr;

class CObject
{
private:
	string	m_Name;
	string	m_Description;
	string	m_WebPage;

	double	m_Size;
	Vec3d	m_Rot;
	bool	m_Recolor;
	color	m_Color;
	double	m_InstallCost;

	dtime	m_Start;
	dtime	m_Finish;

	Vec3d	m_Pos;	//in lat/lon/depth format
	Vec3f	m_SceneTx;

	bool	m_Hide; //set by culling
	bool	m_Selected;
	int		m_SelectID;
	Vec3f	m_BBoxCenter;
	float	m_BBoxRadius;

	CObjectTypePtr m_ObjectTypePtr;

public:
	CObject()
	{
		m_ObjectTypePtr = 0;
		m_Selected = false;
		m_Hide = false;
		m_Pos = Vec3d(0,0,0);
		m_Start = m_Finish = NO_TIME;
		m_Size = 1.0;
		m_Rot = vl_0;
		m_Recolor = false;
		m_Color = CLR_WHITE;
		m_SelectID = -1;
		m_InstallCost = 0;
	}
	~CObject() {}

	CObject(CObject const &t)
	{
		*this = t;
	}
	CObject & operator = (const CObject &t)
	{
		m_Name = t.m_Name;
		m_Description = t.m_Description;
		m_WebPage = t.m_WebPage;

		m_Size = t.m_Size;
		m_Rot = t.m_Rot;
		m_Recolor = t.m_Recolor;
		m_Color = t.m_Color;
		m_InstallCost = t.m_InstallCost;

		m_Start = t.m_Start;
		m_Finish = t.m_Finish;

		m_Pos = t.m_Pos;	//in lat/lon/depth format
		m_SceneTx = t.m_SceneTx;

		m_Hide = false; //set by culling
		m_Selected = false;
		m_SelectID = -1;
		m_BBoxCenter = t.m_BBoxCenter;
		m_BBoxRadius = t.m_BBoxRadius;

		m_ObjectTypePtr = t.m_ObjectTypePtr;

		return *this;
	}


	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	string getDescription() const { return m_Description; }
	void setDescription(string str) { m_Description = str; }
	string getWebPage() const { return m_WebPage; }
	void setWebPage(string str) { m_WebPage = str; }

	string getInfoText();

	double getSize() const { return m_Size; }
	void setSize(double size) { m_Size = size; }
	Vec3d getRotation() const { return m_Rot; }
	void setRotation(Vec3d rot) { m_Rot = rot; }
	bool getRecolor() const { return m_Recolor; }
	void setRecolor(bool b) { m_Recolor = b; }
	color getColor() const { return m_Color; }
	void setColor(color clr) { m_Color = clr; }

	Vec3d getPosition() const { return m_Pos; }
	void setPosition(Vec3d pos) { m_Pos = pos; }
	void updatePosition();
	Vec3f getSceneTx() const { return m_SceneTx; }
	void setSceneTx(Vec3f tx) { m_SceneTx = tx; setBoundingSphere(); }

	const CObjectType & getObjectType() const { return (*m_ObjectTypePtr); }
	CObjectType & editObjectType() { return (*m_ObjectTypePtr); }
	const CObjectTypePtr getObjectTypePtr() const { return m_ObjectTypePtr; }
	void setObjectType(CObjectTypePtr type) { m_ObjectTypePtr = type; }

	int  getSelected() const { return m_Selected; }
	void setSelected(bool b) { m_Selected = b; }
	bool getHide() const { return m_Hide; }
	void setHide(bool b) { m_Hide = b; }

	bool distCull();
	bool bboxCull();
	bool timeCull(double curTime);
	void Render(bool bEditing);
	void RenderName() const;
	void setBoundingSphere() ;

	double getCost() const { return getObjectType().getCost(); }
	double getPowerCost() const { return getObjectType().getPowerCost(); }
	double getPowerLimit() const { return getObjectType().getPowerLimit(); }
	double getDataCost() const { return getObjectType().getDataCost(); }
	double getDataLimit() const { return getObjectType().getDataLimit(); }

	double getInstallCostSetting() const { return m_InstallCost; }
	double getInstallCost() const { return m_InstallCost<=0 ? getObjectType().getInstallCost() : m_InstallCost; }
	void setInstallCost(double f) { m_InstallCost = f; }

	Vec3f getBBCenter() const { return m_BBoxCenter; }
	float getBBRadius() const { return m_BBoxRadius; }
	void setSelectID(int i) { m_SelectID = i;}

	double getStart() const { return m_Start; }
	void setStart(double t) { m_Start = t; }
	double getFinish() const { return m_Finish; }
	void setFinish(double t) { m_Finish = t; }
};

typedef CObject * CObjectPtr;
