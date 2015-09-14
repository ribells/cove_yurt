#pragma once

#include "object.h"

//==========[ class CLine ]==================================================


enum { LINE_STYLE_LINE, LINE_STYLE_SHAPE, LINE_STYLE_CABLE, LINE_STYLE_SURVEY };

class CLineType : public CLayer
{
private:
	string	m_ID;
	string	m_WebPage;

	double	m_Cost;
	double	m_InstallCost;
	double	m_Resistance;

	double	m_Swath;
	double	m_Height;

	double	m_Size;
	color	m_Color;
	color	m_ShapeColor;
	int		m_Style;
	int		m_Dash;

	double	m_ViewMin;	//in km
	double	m_ViewMax;

public:
	CLineType()
	{
		m_Cost = m_InstallCost = m_Resistance = 0;
		m_Swath = 0;
		m_Height = 0;
		m_Size = .25;
		m_Color = 0xff7f7f7f;
		m_ShapeColor = 0x803f3f3f;
		m_Style = LINE_STYLE_LINE;
		m_ViewMin = m_ViewMax = -1;
		m_Dash = 0;
	}
	~CLineType() {}

	CLineType(CLineType const &t)
	{ 
		*this = t;
	}
	CLineType & operator = (const CLineType &t)
	{
		CLayer::operator =(t);
		m_ID = t.m_ID;
		m_WebPage = t.m_WebPage;

		m_Cost = t.m_Cost;
		m_InstallCost = t.m_InstallCost;
		m_Resistance = t.m_Resistance;

		m_Swath = t.m_Swath;
		m_Height = t.m_Height;

		m_Size = t.m_Size;
		m_Color = t.m_Color;
		m_ShapeColor = t.m_ShapeColor;
		m_Style = t.m_Style;
		m_Dash = t.m_Dash;

		m_ViewMin = t.m_ViewMin;	//in km
		m_ViewMax = t.m_ViewMax;

		return *this;
	}

	string getID() const { return m_ID; }
	void setID(string str) { m_ID = str; }
	string getWebPage() const { return m_WebPage; }
	void setWebPage(string str) { m_WebPage = str; }

	string getInfoText() const;

	void setSize(double size) { m_Size = size; }
	double getSize() const { return m_Size; }
	void setColor(color clr) { m_Color = clr; }
	color getColor() const { return m_Color; }
	void setShapeColor(color clr) { m_ShapeColor = clr; }
	color getShapeColor() const { return m_ShapeColor; }
	void setDash(int dash) { m_Dash = dash; }
	int getDash() const { return m_Dash; }
	void setStyle(int style) { m_Style = style; }
	int getStyle() const { return m_Style; }
	bool isLine() { return m_Style == LINE_STYLE_LINE; }
	bool isShape() const { return m_Style == LINE_STYLE_SHAPE; }
	bool isCable() { return m_Style == LINE_STYLE_CABLE; }
	bool isSurvey() const { return m_Style == LINE_STYLE_SURVEY; }

	void RenderLine(vector<Vec3f> *pnts, vector<float> *dist, bool bSelected, double dDist, double dSize, int iSelectID, color clr) const;
	void RenderControl(Vec3f pnt, bool bSelected, double dDist, double dSize, int iSelectID, color clr) const;
	void drawCable(Vec3f *pnts, int pntCnt, double dSize, int iSelectID) const;

	double getCost() const { return m_Cost; }
	void setCost(double f) { m_Cost = f; }
	double getInstallCost() const { return m_InstallCost; }
	void setInstallCost(double f) { m_InstallCost = f; }

	double getViewMin() const { return m_ViewMin; }
	void setViewMin(double val) { m_ViewMin = val; }
	double getViewMax() const { return m_ViewMax; }
	void setViewMax(double val) { m_ViewMax = val; }

	double getResistance() const { return m_Resistance; }
	void setResistance(double f) { m_Resistance = f; }

	double getSwath() const { return m_Swath; }
	void setSwath(double f) { m_Swath = f; }
	double getHeight() const { return m_Height; }
	void setHeight(double f) { m_Height = f; }
};

const int MAX_CONNECT_SEG=512;

class CSegment
{
private:
	Vec3d	m_StartPos, m_EndPos;

	vector<Vec3f>	m_PointList;
	vector<float>	m_PointDist;
	Vec3f	m_BBoxCenter;
	float	m_BBoxRadius;

public:
	CSegment() {}
	~CSegment() {}

	void setStartPos(Vec3d start) { m_StartPos = start; }
	Vec3d getStartPos() const { return m_StartPos; }
	void setEndPos(Vec3d end) { m_EndPos = end; }
	Vec3d getEndPos() const { return m_EndPos; }

	void setPointList(vector<Vec3f> Pnts){ m_PointList = Pnts; }
	void setPointDist(vector<float> Dist) { m_PointDist = Dist; }
	Vec3f getPoint(int i) const { return m_PointList[i]; }
	vector<Vec3f> * getPointList() { return &m_PointList; }
	float getDist(int i) const { return m_PointDist[i]; }
	vector<float> * getPointDist() { return &m_PointDist; }
	int getPointCnt() const { return m_PointList.size(); }

	double getLength() const;

	void setBoundingSphere() ;
	Vec3f getBBCenter() const { return m_BBoxCenter; }
	float getBBRadius() const { return m_BBoxRadius; }
};

class CControlPnt
{
private:
	string	m_Name;
	Vec3d	m_Position;

public:

	CControlPnt() { m_Position = vl_0; }
	~CControlPnt() {}
	string getDefaultName() const;

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }

	Vec3d getPosition() const { return m_Position; }
	void setPosition(Vec3d pos) { m_Position = pos; }

	string getInfoText() const;
};

typedef CLineType * CLineTypePtr;

class CLine
{
private:
	string	m_Name;
	string	m_Description;
	string	m_WebPage;

	double	m_Size;
	bool	m_Recolor;
	color	m_Color;
	double	m_InstallCost;

	dtime	m_Start;
	dtime	m_Finish;

	double	m_Length, m_Depth, m_Area;

	bool	m_Hide; //set by culling
	bool	m_Selected;
	int		m_SelectID;
	int		m_SelControl;

	CLineTypePtr m_LineTypePtr;
	CObjectPtr   m_ObjectPtr[2];

	vector<CControlPnt> m_Control;
	vector<CSegment> m_Segment;

public:
	CLine(): m_LineTypePtr(0), m_Selected(false), m_Hide(false), m_SelControl(-1)
	{
		m_ObjectPtr[0] = m_ObjectPtr[1] = 0;
		m_Start = m_Finish = NO_TIME;
		m_Length = m_Depth = m_Area = 0;
		m_InstallCost = 0;
		m_Size = 1.0;
		m_Recolor = false;
		m_Color = CLR_WHITE;
	}

	~CLine() { clean(); }

	void clearSegmentList() { m_Segment.clear();   std::vector< CSegment >().swap(m_Segment);}

	void clean () { clearSegmentList();	}

	CLine(CLine const &t)
	{
		*this = t;
		clean();
	}
	CLine & operator = (const CLine &t)
	{
		clean();

		m_Name = t.m_Name;
		m_Description = t.m_Description;
		m_WebPage = t.m_WebPage;

		m_Size = t.m_Size;
		m_Recolor = t.m_Recolor;
		m_Color = t.m_Color;
		m_InstallCost = t.m_InstallCost;

		m_Start = t.m_Start;
		m_Finish = t.m_Finish;

		m_Length = t.m_Length;
		m_Depth = t.m_Depth;
		m_Area = t.m_Area;

		m_Hide = false; //set by culling
		m_Selected = false;
		m_SelectID = -1;
		m_SelControl = -1;

		m_LineTypePtr = t.m_LineTypePtr;
		m_ObjectPtr[0] = t.m_ObjectPtr[0];
		m_ObjectPtr[1] = t.m_ObjectPtr[1];
		m_Control = t.m_Control;

		rebuildSegments();
		return *this;
	}

	string getDefaultName() const;
	string getPointString(bool bHtml, bool bHeading) const;
	string getPointStringCnt() const;

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	string getDescription() const { return m_Description; }
	void setDescription(string str) { m_Description = str; }
	string getWebPage() const { return m_WebPage; }
	void setWebPage(string str) { m_WebPage = str; }

	double getSize() const { return m_Size; }
	void setSize(double size) { m_Size = size; }
	bool getRecolor() const { return m_Recolor; }
	void setRecolor(bool b) { m_Recolor = b; }
	color getColor() const { return m_Color; }
	void setColor(color clr) { m_Color = clr; }

	string getInfoText() const;
	string getInfoTextPoints(bool bHeading) const;

	Vec3d getEndObjectPos(int i) const { return getEndObjectPtr(i) ? getEndObject(i).getPosition() : vl_0; }

	CObject & getEndObject (int i) const { return *(m_ObjectPtr[i]); }
	CObjectPtr getEndObjectPtr(int i)  const { return m_ObjectPtr[i]; }
	void setEndObject(int i, CObjectPtr pObj) { m_ObjectPtr[i] = pObj; }
	Vec3d getEndPos(int i)  const { return i == 0 ? getControlPos(0) : getControlPos(getControlCnt()-1); }
	int getEndIndex(CObjectPtr pObj)  const { return m_ObjectPtr[0] == pObj ? 0 : m_ObjectPtr[1] == pObj ? 1 : -1; }

	void rebuildSegments()
	{
		//reset end positions from objects if necessary
		if (getEndObjectPtr(0)) 
			m_Control[0].setPosition(getEndObjectPos(0));
		if (getEndObjectPtr(1)) 
			m_Control[getControlCnt()-1].setPosition(getEndObjectPos(1));
		//make new segments
		clearSegmentList();
		CSegment Seg;
		Seg.setStartPos( m_Control[0].getPosition());
		Seg.setEndPos(m_Control[getControlCnt()-1].getPosition());
		m_Segment.push_back(Seg);
		for (int i = 1; i < getControlCnt()-1; i++)
			addControlSeg(m_Control[i].getPosition(), i-1);
		updatePosition();
	}

	void setEndObjects(CObjectPtr pObj0, CObjectPtr pObj1)
	{
		m_ObjectPtr[0] = pObj0; m_ObjectPtr[1] = pObj1;
		if (m_Name == "" || m_Name == getDefaultName()) 
			setName(getDefaultName());
		rebuildSegments();
	}

	double getStart() const { return m_Start; }
	void setStart(double t) { m_Start = t; }
	double getFinish() const { return m_Finish; }
	void setFinish(double t) { m_Finish = t; }
	double getCalcStart() const
	{
		double start = 0;
		if (m_Start) 
			start = m_Start;
		else if (getEndObjectPtr(0) && getEndObjectPtr(1)) 
			start = max(getEndObject(0).getStart(), getEndObject(1).getStart());
		else if (getEndObjectPtr(0)) 
			start = getEndObject(0).getStart();
		else if (getEndObjectPtr(1)) 
			start = getEndObject(1).getStart();
		return start;
	}
	double getCalcFinish() const 
	{
		double finish = 0;
		if (m_Finish) 
			finish = m_Finish;
		else if (getEndObjectPtr(0) && getEndObjectPtr(1)) 
			finish = max(getEndObject(0).getFinish(), getEndObject(1).getFinish());
		else if (getEndObjectPtr(0)) 
			finish = getEndObject(0).getFinish();
		else if (getEndObjectPtr(1))
			finish = getEndObject(1).getFinish();
		return finish;
	}

	void updatePosition();
	bool isMooringLine()  const 
	{
		if (!getEndObjectPtr(0) || !getEndObjectPtr(1)) return false;
		return ((getEndObject(0).getObjectTypePtr() 
				&& (getEndObject(0).getObjectType().getPosType()==POS_FLOAT || getEndObject(0).getObjectType().getPosType()==POS_MOBILE))
			|| (getEndObject(1).getObjectTypePtr() 
				&& (getEndObject(1).getObjectType().getPosType()==POS_FLOAT) || getEndObject(1).getObjectType().getPosType()==POS_MOBILE)); }

	bool addControl(Vec3d pos, int iNdx)
	{
		CControlPnt handle;
		handle.setPosition(pos);
		if (iNdx == -1 || iNdx >= m_Control.size())
			m_Control.push_back(handle);
		else
			m_Control.insert(m_Control.begin()+iNdx, handle);
		return true;
	}
	bool addControl(const CControlPnt & Ctrl, int iNdx)
	{
		if (iNdx == -1 || iNdx >= m_Control.size())
			m_Control.push_back(Ctrl);
		else
			m_Control.insert(m_Control.begin()+iNdx, Ctrl);
		return true;
	}
	void addControlSeg(Vec3d pos, int iSeg);
	int getNearestSegment(Vec3d pos) const;
	int getNearestEnd(Vec3d pos) const;
	bool getNearestEnds(CLine & Line1, int & iEnd0, int & iEnd1) const;

	void placeControl(Vec3d pos, int cnt);
	void delControl(int iCtrl);
	void updateControl(int ndx, Vec3d pos);
	int  getControlCnt() const { return m_Control.size(); }
	void setControlPos(int i, Vec3d pos) { m_Control[i].setPosition(pos); }

	CLineType & getLineType() const { return (*m_LineTypePtr); }
	void setLineType(CLineTypePtr type) { m_LineTypePtr = type; }
	CLineTypePtr getLineTypePtr() { return m_LineTypePtr; }

	bool getSelected() const { return m_Selected; }
	void setSelected(bool b) { m_Selected = b; m_SelControl = -1; }
	int	getSelControl() const { return m_SelControl; }
	void setSelControl(int i) { m_SelControl = i; }
	bool getControlValid(int i) const { return i >= 0 && i < getControlCnt(); }
	CControlPnt & getControl(int i) const { return const_cast<CControlPnt &>(m_Control[i]); }
	CSegment & getSegment(int i) const { return const_cast<CSegment &>(m_Segment[i]); }

	Vec3d getControlPos(int i) const { return getControlValid(i) ? getControl(i).getPosition() : vl_0; }

	bool getHide() const { return m_Hide; }
	void setHide(bool b) { m_Hide = b; }

	int getSegmentCnt() const { return m_Segment.size(); }

	bool bboxCull();
	bool distCull();
	bool timeCull(double curTime);
	void Render() const;
	void RenderName() const;

	double getCost();
	double getLength() const {return m_Length; }
	double getDepth() const {return m_Depth; }
	double getArea() const {return m_Area; }
	double getVolume() const {return m_Depth*m_Area; }
	double getInstallCostSetting() const { return m_InstallCost; }
	double getInstallCost() const { return m_InstallCost<0 ? m_Length*getLineType().getInstallCost() : m_InstallCost; }
	void setInstallCost(double f) { m_InstallCost = f; }

	void setSelectID(int i) { m_SelectID = i;}

	vector<Vec3d> getGPSPointList(bool bControls) const;

	bool getMinMax(Vec3d &min_pos, Vec3d &max_pos) const
	{
		if (m_Segment.size()==0) 
			return false;
		min_pos = max_pos = m_Segment[0].getStartPos();
		for (int i = 0; i < getSegmentCnt(); i++)
		{
			Vec3d pos = m_Segment[i].getEndPos();
			for (int j = 0; j < 3; j++)
			{
				if (min_pos[j] > pos[j]) min_pos[j] = pos[j];
				if (max_pos[j] < pos[j]) max_pos[j] = pos[j];
			}
		}
		return true;
	}

	void reverseOrder()
	{
		vector<CControlPnt> m_TmpControl = m_Control;
		m_Control.clear();
		for (int i = m_TmpControl.size()-1; i >= 0; i--)
			m_Control.push_back(m_TmpControl[i]);
		setEndObjects(m_ObjectPtr[1], m_ObjectPtr[0]);
	}
};

typedef CLine * CLinePtr;
