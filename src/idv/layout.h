#pragma once

#include "xmlParser.h"
#include "layouttypes.h"
#include "layer.h"
#include "object.h"
#include "line.h"
#include <string>
#include <sstream>

//==========[ class CLayout]==================================================

class CLayout : public CLayer
{
private:
	CLayoutTypesPtr	m_LayoutTypesPtr;

	vector<CObjectPtr>	m_ObjectList;
	vector<CLinePtr>		m_LineList;

	vector<CObjectPtr>	m_SelObjects;
	vector<CLinePtr>		m_SelLines;

	bool	m_CostDirty;
	bool	m_CostSel;
	double	m_Cost;
	double	m_PowerCost;
	double	m_DataCost;
	vector<int>	m_CostObjectCnt;
	vector<double> m_CostCableCnt;

	double	m_Start;
	double	m_Finish;
	double	m_CurTime;

	string	m_KMLObjectTypeID;
	string	m_KMLLineTypeID;

	bool	m_Editing;
	bool	m_Dirty;

public:

	CLayout(CLayoutTypesPtr pTypes = 0)
	{
		m_LayoutTypesPtr = pTypes;
		m_KMLObjectTypeID = "";
		m_KMLLineTypeID = "";
		m_Start = m_Finish = m_CurTime = NO_TIME;
		m_Editing = false;

		m_CostDirty = true;
		m_Cost = m_PowerCost = m_DataCost = 0;
		m_CostSel = false;
	}

	~CLayout()
	{
		clean_objects();
	}

	void clean_objects()
	{
		clearLines();
		clearObjects();
		clean_lists();
	}
	void clean_lists()
	{
		m_SelLines.clear();
		m_SelObjects.clear();
		m_ObjectList.clear();
		m_LineList.clear();
	}
	void clean()
	{
		m_CostDirty = true;
		m_Cost = m_PowerCost = m_DataCost = 0;
		m_CostObjectCnt.clear();	std::vector< int >().swap(m_CostObjectCnt);
		m_CostCableCnt.clear();		std::vector< double >().swap(m_CostCableCnt);
		m_CostSel = false;
		m_Editing = false;
	}

	CLayout & operator = (const CLayout & t)
	{
		CLayer::operator =(t);
		setKMLObjectTypeID(t.getKMLObjectTypeID());
		setKMLLineTypeID(t.getKMLLineTypeID());
		if (t.getLink() == "")
			copyLayout(t);
		return *this;
	}

	CLayout(CLayout const &t)
	{
		*this = t;
		clean_lists();
		setLoaded(false);
	}

	void copyLayout(CLayout const & t)
	{
		for (int i = 0; i < t.getObjectCnt(); i++)
		{
			CObject Object = CObject(t.getObject(i));
			addObject(Object);
		}
		for (int i = 0; i < t.getLineCnt(); i++)
		{
			CLine Line = CLine(t.getLine(i));
			CObjectPtr pObj[2];
			for (int j = 0; j < 2; j++)
			{
				pObj[j] = getObjectPtr(t.getObjectIndex(Line.getEndObjectPtr(j)));
				Line.setEndObject(j, pObj[j]);
			}
			addLine(Line);
			Line.setEndObjects(0,0);  //so destructor doesn't call objects
		}
	}

	CLayoutTypes & getLayoutTypes() const { return *m_LayoutTypesPtr; } //return reference rather than pointer
	void setLayoutTypesPtr(CLayoutTypesPtr pTypes) { m_LayoutTypesPtr = pTypes; }
	void setLayoutTypes(CLayoutTypesPtr pTypes) 
	{
		for (int i = getObjectCnt()-1; i >= 0 ; i--)
		{
			const CObjectTypePtr pType = pTypes->getObjectTypePtr(getObject(i).getObjectType().getID());
			if (pType == NULL)
				delObject(i);
			else
				getObject(i).setObjectType(pType);
		}
		for (int i = getLineCnt()-1; i >= 0 ; i--)
		{
			const CLineTypePtr pType = pTypes->getLineTypePtr(getLine(i).getLineType().getID());
			if (pType == NULL)
				delLine(i);
			else
				getLine(i).setLineType(pType);
		}
		m_LayoutTypesPtr = pTypes;
	}

	void updateKMLObjectType()
	{
		const CObjectTypePtr pType = getLayoutTypes().getObjectTypePtr(getKMLObjectTypeID());
		for (int i = 0; i < getObjectCnt(); i++)
			getObject(i).setObjectType(pType);
	}
	void updateKMLLineType()
	{
		const CLineTypePtr pType = getLayoutTypes().getLineTypePtr(getKMLLineTypeID());
		for (int i = 0; i < getLineCnt(); i++)
			getLine(i).setLineType(pType);
	}

	bool deserialize_Node(XMLNode &xn0, string strTag, int iXmlNdx = 0, int iRecNdx = -1);
	bool serialize_Node(XMLNode &xn0, string strTag, int iRecNdx);

	bool deserialize_Record(string strText, string strTag, int iRecNdx = -1);
	string serialize_Record(string strTag, int iRecNdx);

	bool deserialize(XMLNode &xn0);
	void serialize(XMLNode &xn0, bool bSelected = false);

	bool readFile(string fileName);
	void writeFile(string fileName);

	bool readCSVFile(string fileName);
	bool readKMLFile(string fileName);
	void writeKMLFile(string fileName);

	bool setActive(bool b);

	bool getEditable() const { return m_Link == ""; }

	string getKMLObjectTypeID() const { return m_KMLObjectTypeID; }
	void setKMLObjectTypeID(string str) { m_KMLObjectTypeID = str; }
	string getKMLLineTypeID() const { return m_KMLLineTypeID; }
	void setKMLLineTypeID(string str) { m_KMLLineTypeID = str; }

	void RenderLines(bool bSelect = false, int iLayout = 0) const;
	void RenderObjects(bool bSelect = false, int iLayout = 0) const;
	void RenderNames() const;
	void Render(bool bShowObj, bool bShowCon);
	void cullObjects();
	void cullLines();

	//object list methods
	int getObjectCnt() const { return m_ObjectList.size(); }
	bool getObjectValid(int i) const { return i >= 0 && i < getObjectCnt(); }
	CObject & getObject(const CObjectPtr pObject) const { return *pObject; }
	void clearObjects()
	{
		while (getObjectCnt())
			delObject(0);
	}

	CObject & getObject(int iNdx) const
	{ 
		assert(getObjectValid(iNdx));
		return *m_ObjectList[iNdx];
	}

	int getObjectIndex(string name) const
	{ 
		for (int i = 0; i < getObjectCnt(); i++)
			if (name == getObject(i).getName())
				return i;
		return -1;
	}
	int getObjectIndex(const CObjectPtr pObject) const
	{ 
		for (int i = 0; i < getObjectCnt(); i++)
			if (pObject == getObjectPtr(i))
				return i;
		return -1;
	}
	const CObjectPtr getObjectPtr(int iNdx) const
	{ 
		if (!getObjectValid(iNdx))
			return NULL;
		return m_ObjectList[iNdx];
	}
	const CObjectPtr getObjectPtr(string name) const
	{ 
		return getObjectPtr(getObjectIndex(name));
	}


	int getObjectCnt(CObjectTypePtr pObjType) const
	{ 
		int cnt = 0;
		for (int i = 0; i < getObjectCnt(); i++)
			if (getObject(i).getObjectTypePtr() == pObjType)
				cnt++;
		return cnt;
	}

	void updateObject(CObject & Object)
	{
		if (!m_Editing)
			return;
		Object.updatePosition();
		for (int i=0; i < getLineCnt(); i++)
			if (getLine(i).getEndIndex(&Object) != -1)
				updateLine(getLine(i));
		updateTimeSpan(Object);
		setCostDirty();
	}
	bool addObject(CObject & Object, int iNdx = -1)
	{
		if (Object.getObjectTypePtr()==NULL)
			return false;
		if (!Object.getName().size())
			Object.setName(Object.getObjectType().getID());
		if (getObjectIndex(Object.getName()) != -1)
		{
			string str;
			for (int i = 1; getObjectIndex(str = getNewName(Object.getName(), i)) != -1; i++);
			Object.setName(str);
		}
		if (!getObjectValid(iNdx))
		{
			m_ObjectList.push_back(new CObject());
			iNdx = getObjectCnt()-1;
		}
		else
		{
			vector<CObjectPtr> :: iterator iter = m_ObjectList.begin();
			for (int i = 0; iter != m_ObjectList.end() && i < iNdx; iter++, i++);
			m_ObjectList.insert(iter, new CObject());
		}
		return editObject(Object, iNdx);
	}

	bool editObject(CObject & Object, int iNdx)
	{
		if (!getObjectValid(iNdx))
			return false;
		getObject(iNdx) = Object;
		updateObject(getObject(iNdx));
		return true;
	}

	bool delObject(int iNdx)
	{ 
		if (!getObjectValid(iNdx))
			return false;
		removeSelObject(iNdx);
		delete (m_ObjectList[iNdx]);
		m_ObjectList.erase(m_ObjectList.begin() + iNdx);
		setCostDirty();
		updateTimeSpan();
		return true;
	}

	//line list methods
	int getLineCnt() const { return m_LineList.size(); }
	bool getLineValid(int i) const { return i >= 0 && i < getLineCnt(); }
	CLine & getLine(const CLinePtr pLine) const { return *pLine; }
	void clearLines()
	{
		while (getLineCnt())
			delLine(0);
	}

	CLine & getLine(int iNdx) const
	{ 
		assert(getLineValid(iNdx));
		return *m_LineList[iNdx];
	}

	int getLineIndex(string name) const
	{ 
		for (int i = 0; i < getLineCnt(); i++)
			if (name == getLine(i).getName())
				return i;
		return -1;
	}
	int getLineIndex(const CLinePtr pLine) const
	{ 
		for (int i = 0; i < getLineCnt(); i++)
			if (pLine == getLinePtr(i))
				return i;
		return -1;
	}
	const CLinePtr getLinePtr(int iNdx) const
	{ 
		if (!getLineValid(iNdx))
			return NULL;
		return m_LineList[iNdx];
	}
	const CLinePtr getLinePtr(string name) const
	{ 
		return getLinePtr(getLineIndex(name));
	}


	void updateLine(CLine & Line)
	{
		updateFloatObjects(Line.getEndObjectPtr(0));
		Line.updatePosition();
		updateTimeSpan(Line);
		setCostDirty();
	}

	bool addLine(CLine & Line, int iNdx = -1)
	{
		if (Line.getLineTypePtr()==NULL || Line.getControlCnt() == 0)
			return false;
		if (!getLineValid(iNdx))
		{
			m_LineList.push_back(new CLine());
			iNdx = getLineCnt()-1;
		}
		else 
		{
			vector<CLinePtr> :: iterator iter = m_LineList.begin();
			for (int i = 0; iter != m_LineList.end() && i < iNdx; iter++, i++);
			m_LineList.insert(iter, new CLine());
		}
		return editLine(Line, iNdx);
	}

	bool editLine(CLine & Line, int iNdx)
	{
		if (!getLineValid(iNdx))
			return false;
		getLine(iNdx) = Line;
		updateLine(getLine(iNdx));
		return true;
	}

	bool delLine(int iNdx)
	{ 
		if (!getLineValid(iNdx))
			return false;
		removeSelLine(iNdx);
		delete (m_LineList[iNdx]);
		m_LineList.erase(m_LineList.begin() + iNdx);
		setCostDirty();
		updateTimeSpan();
		return true;
	}

	//object selection methods
	int getSelObjectCnt() const { return m_SelObjects.size(); }
	CObject & getSelObject(int i) const { return *(m_SelObjects[i]); }
	const CObjectPtr getSelObjectPtr(int i) const { return m_SelObjects.size()>i ? m_SelObjects[i] : NULL; }
	bool selObject(int iSel, bool bMulti);
	bool isSelected(const CObjectPtr pObj) const
	{
		for (int i = 0; i < getSelObjectCnt(); i++)
			if (pObj == m_SelObjects[i])
				return true;
		return false;
	}
	int getSelObjectIndex(const CObjectPtr pObj)
	{
		for (int i = 0; i < getSelObjectCnt(); i++)
			if (pObj == m_SelObjects[i])
				return i;
		return -1;
	}
	void selObject(CObjectPtr pObj)
	{
		if (isSelected(pObj))
			return;
		m_SelObjects.push_back(pObj);
		getObject(pObj).setSelected(true);
	}
	void removeSelObject(int idx)
	{
		CObjectPtr pObj = getObjectPtr(idx);
		for (int i=0; i < getSelObjectCnt(); i++)
			if (m_SelObjects[i] == pObj)
			{
				getObject(pObj).setSelected(false);
				m_SelObjects.erase( m_SelObjects.begin()+i );
				return;
			}
	}

	//line selection methods
	int getSelLineCnt() const { return m_SelLines.size(); }
	CLine & getSelLine(int i) const { return *(m_SelLines[i]); }
	const CLinePtr getSelLinePtr(int i) const { return m_SelLines.size()>i ? m_SelLines[i] : NULL; }
	bool selLine(int iLine, int iControl, bool bMulti);
	bool isSelected(const CLinePtr pLine)  const
	{
		for (int i = 0; i < getSelLineCnt(); i++)
			if (pLine == m_SelLines[i])
				return true;
		return false;
	}
	void selLine(CLinePtr pLine)
	{
		if (isSelected(pLine))
			return;
		m_SelLines.push_back(pLine);
		getLine(pLine).setSelected(true);
	}
	void removeSelLine(int idx)
	{
		CLinePtr pLine = getLinePtr(idx);
		for (int i=0; i < getSelLineCnt(); i++)
			if (m_SelLines[i] == pLine)
			{
				getLine(pLine).setSelected(false);
				m_SelLines.erase( m_SelLines.begin()+i );
				return;
			}
	}

	void clearSelObjects()
	{
		for (int i = 0; i < getObjectCnt(); i++)
			getObject(i).setSelected(false);
		m_SelObjects.clear();
	}
	void clearSelLines()
	{
		for (int i = 0; i < getLineCnt(); i++)
			getLine(i).setSelected(false);
		m_SelLines.clear();
	}
	void clearSelections()
	{
		clearSelLines();
		clearSelObjects();
	}
	void selectAllObjects()
	{
		m_SelObjects.clear();
		for (int i = 0; i < getObjectCnt(); i++)
		{
			m_SelObjects.push_back(getObjectPtr(i));
			getObject(i).setSelected(true);
		}
	}
	void selectAllLines()
	{
		m_SelLines.clear();
		for (int i = 0; i < getLineCnt(); i++)
		{
			m_SelLines.push_back(getLinePtr(i));
			getLine(i).setSelected(true);
		}
	}
	void selectAll()
	{
		selectAllObjects();
		selectAllLines();
	}

	void updateMinMax()
	{
		m_MinPos = (Vec3d)vl_1 * 1e8;
		m_MaxPos = (Vec3d)vl_1 * -1e8;
		for (int i = 0; i < getObjectCnt(); i++)
		{
			Vec3d pos = getObject(i).getPosition();
			for (int j = 0; j < 3; j++)
			{
				if (m_MinPos[j] > pos[j]) m_MinPos[j] = pos[j];
				if (m_MaxPos[j] < pos[j]) m_MaxPos[j] = pos[j];
			}
		}
		for (int i = 0; i < getLineCnt(); i++)
		{
			Vec3d minpos, maxpos;
			if (!getLine(i).getMinMax(minpos, maxpos))
				continue;
			for (int j = 0; j < 3; j++)
			{
				if (m_MinPos[j] > minpos[j]) m_MinPos[j] = minpos[j];
				if (m_MaxPos[j] < maxpos[j]) m_MaxPos[j] = maxpos[j];
			}
		}
		if (m_MinPos[0] == 1e8)
		{
			m_MinPos = Vec3d(-90,-180,0);
			m_MaxPos = Vec3d(90,180,0);
		}
	}
	bool getMinMax(Vec3d &min_pos, Vec3d &max_pos)
	{
		if (getActive())
			updateMinMax();
		CLayer::getMinMax(min_pos, max_pos);
		return true;
	}

private:
	CObjectPtr findBaseR(CObjectPtr pObject, vector<CObjectPtr> vObjects);
	void updateFloatObjectsR(CObjectPtr pObject, vector<CObjectPtr> vObjects);
public:
	void updateFloatObjects(CObjectPtr pObject);
	bool moveSelection(Vec3d offset);
	bool moveSelection(int iDir);
	void updateLayerPositions();

	double getLineLength( const CLineTypePtr pType) const
	{ 
		double dist = 0;
		for (int i = 0; i < getLineCnt(); i++)
			if (getLine(i).getLineTypePtr() == pType)
				dist += getLine(i).getLength();
		return dist;
	}

	bool addSelLine(int iLineIndex);

	bool moveControl(Vec3d offset);

	bool getEditing() const { return m_Editing; }
	void setEditing(bool b) { m_Editing = b; if (!m_Editing) clearSelections(); }

	void createTemplateFromSel(CTemplate & Set);
	void insertTemplate(CTemplate & Set, Vec3d pos);
	void copySelection()
	{
		CTemplate & Set = getLayoutTypes().getCopySet();
		createTemplateFromSel(Set);
	}
	bool pasteSelection(Vec3d pos)
	{
		CTemplate & Set = getLayoutTypes().getCopySet();
		insertTemplate(Set, pos);
		return true;
	}
	void addTemplate(string name)
	{
		CTemplate Set;
		createTemplateFromSel(Set);
		Set.setName(name);
		getLayoutTypes().addTemplate(Set);
	}
	void pasteTemplate(int idx, Vec3d pos)
	{
		CTemplate & Set = getLayoutTypes().getTemplate(idx);
		if (!Set.isValid())
			return;
		insertTemplate(Set, pos);
	}

	void resetCost();
	void updateCost();

	void updateLayerCost(bool bSel);

	//COSTING	
	bool getCostDirty() const { return m_CostDirty; }
	void setCostDirty(bool b = true) { m_CostDirty = b; }
	bool getCostSel() const { return m_CostSel; }
	void setCostSel(bool b) { m_CostSel = b; m_CostDirty = true;}

	double getCost() const { return m_Cost; }
	double getPowerCost() const { return m_PowerCost;	}
	double getDataCost() const { return m_DataCost; }
	int getCostObjectCnt(int i) const { return m_CostObjectCnt[i]; }
	double getCostCableCnt(int i) const { return m_CostCableCnt[i]; }

	double getStart() const { return m_Start; }
	void setStart(double t) { m_Start = t; }
	double getFinish() const { return m_Finish; }
	void setFinish(double t) { m_Finish = t; }
	double getCurTime() const { return m_CurTime; }
	void setCurTime(double t) { m_CurTime = t; }
	void recalcTimes()
	{
		m_Finish = max(m_Start, m_Finish);
		m_CurTime = min(m_Finish, max(m_Start, m_CurTime ? m_CurTime : m_Finish));
	}
	void updateTimeSpan(const CObject & Obj)
	{
		if (Obj.getStart() > 0 && (Obj.getStart() < m_Start || m_Start == NO_TIME) )
			m_Start = Obj.getStart();
		if (Obj.getFinish() > m_Finish) 
			m_Finish = Obj.getFinish();
		recalcTimes();
	}
	void updateTimeSpan(const CLine & Line)
	{
		if (Line.getStart() > 0 && (Line.getStart() < m_Start || m_Start == NO_TIME))
			m_Start = Line.getStart();
		if (Line.getFinish() > m_Finish) 
			m_Finish = Line.getFinish();
		recalcTimes();
	}
	void updateTimeSpan()
	{
		{
			m_Finish = NO_TIME;
			m_Start = NO_TIME;
			for (int i = 0; i < getObjectCnt(); i++)
				updateTimeSpan(getObject(i));
			for (int i = 0; i < getLineCnt(); i++)
				updateTimeSpan(getLine(i));
		}
		recalcTimes();
	}
};

typedef CLayout * CLayoutPtr;

class CLayoutSet
{
private:
	CLayoutTypesPtr m_LayoutTypesPtr;

	vector<CLayoutPtr> m_LayoutList;

public:

	CLayoutSet(CLayoutTypesPtr pTypes = 0)
	{
		m_LayoutTypesPtr = pTypes;
	}
	~CLayoutSet()
	{
		while (getLayoutCnt())
			delLayout(0);
	}

	void setLayoutTypes(const CLayoutTypesPtr pTypes)
	{
		m_LayoutTypesPtr = pTypes;
		for (int i = 0; i < getLayoutCnt(); i++)
			getLayout(i).setLayoutTypes(m_LayoutTypesPtr);
	}
	CLayoutTypes & getLayoutTypes() const { return *m_LayoutTypesPtr; }
	const CLayoutTypesPtr getLayoutTypesPtr() const { return m_LayoutTypesPtr; }
	void unloadMaterials()	{getLayoutTypes().unloadMaterials(); }
	void reloadMaterials()	{getLayoutTypes().reloadMaterials(); }

	bool delObjectType(int idx);
	bool delLineType(int idx);
	bool delTemplate(int idx) { return getLayoutTypes().delTemplate(idx); }

	int getLayoutCnt() const { return m_LayoutList.size(); }
	bool getLayoutValid(int i) const { return i >= 0 && i < getLayoutCnt(); }

	CLayout & getLayout(int iNdx) const
	{ 
		assert(getLayoutValid(iNdx));
		return *m_LayoutList[iNdx];
	}
	int getLayoutIndex(string name) const
	{ 
		for (int i = 0; i < getLayoutCnt(); i++)
			if (name == getLayout(i).getName())
				return i;
		return -1;
	}
	const CLayoutPtr getLayoutPtr(int iNdx) const
	{ 
		if (!getLayoutValid(iNdx))
			return NULL;
		return m_LayoutList[iNdx];
	}
	int getLayoutIndex(const CLayoutPtr hLayout) const
	{ 
		for (int i = 0; i < getLayoutCnt(); i++)
			if (hLayout == getLayoutPtr(i))
				return i;
		return -1;
	}

	bool addLayout(CLayout & Layout, int iNdx = -1)
	{
		CLayoutPtr hLayout = new CLayout(getLayoutTypesPtr());
		if (!getLayoutValid(iNdx))
		{
			m_LayoutList.push_back(hLayout);
			iNdx = getLayoutCnt()-1;
		}
		else
		{
			vector<CLayoutPtr> :: iterator iter = m_LayoutList.begin();
			for (int i = 0; iter != m_LayoutList.end() && i < iNdx; iter++, i++);
			m_LayoutList.insert(iter, hLayout);
		}
		editLayout(Layout, iNdx);
		return true;
	}

	bool editLayout(CLayout & Layout, int iNdx)
	{
		if (!getLayoutValid(iNdx))
			return false;
		Layout.setLayoutTypesPtr(getLayoutTypesPtr());
		getLayout(iNdx) = Layout;
		return true;
	}

	bool delLayout(int iNdx)
	{ 
		if (!getLayoutValid(iNdx))
			return false;
		delete (m_LayoutList[iNdx]);
		m_LayoutList.erase(m_LayoutList.begin() + iNdx);
		return true;
	}

	bool moveLayoutLayerUp(int iNdx)
	{ 
		if (!getLayoutValid(iNdx))
			return false;
		string strGroup = getLayout(iNdx).getGroup();
		int iSwap = -1;
		for (int i = 0; i < getLayoutCnt(); i++)
		{
			if (i == iNdx)	break;
			if (strGroup != getLayout(i).getGroup())	continue;
			iSwap = i;
		}
		if (iSwap == -1)
			return false;
		CLayoutPtr tmp = m_LayoutList[iNdx];
		m_LayoutList[iNdx] = m_LayoutList[iSwap];
		m_LayoutList[iSwap] = tmp;
		return true;
	}

	bool setActive(int iNdx, bool b)
	{
		return getLayout(iNdx).setActive(b);
	}


	int getEditLayoutNdx() const
	{
		for (int i = 0; i < getLayoutCnt(); i++)
			if (getLayout(i).getEditing())
				return i;
		return -1;
	}
	CLayout & getEditLayout() const
	{
		int iLayout = getEditLayoutNdx();
		assert(getLayoutValid(iLayout));
		return getLayout(iLayout);
	}
	void setEditLayout(int newNdx, bool bEdit)
	{
		int oldNdx = getEditLayoutNdx();
		if (bEdit && newNdx == oldNdx)
			return;
		if (!bEdit && newNdx != oldNdx)
			return;
		if (oldNdx >= 0)
			getLayout(oldNdx).setEditing(false);
		getLayout(newNdx).clearSelections();
		if (newNdx >= 0 && bEdit)
			getLayout(newNdx).setEditing(true);
	}

	void Render(bool bShowObj, bool bShowCon) const
	{
		for (int i = 0; i < getLayoutCnt(); i++)
			if (getLayout(i).getActive())
				getLayout(i).Render(bShowObj, bShowCon);
	}
	void RenderLines(bool bSelect = false) const
	{
		for (int i = 0; i < getLayoutCnt(); i++)
			if (getLayout(i).getActive())
				getLayout(i).RenderLines(bSelect, i);
	}
	void RenderObjects(bool bSelect = false) const
	{
		for (int i = 0; i < getLayoutCnt(); i++)
			if (getLayout(i).getActive())
				getLayout(i).RenderObjects(bSelect, i);
	}

	void RenderNames() const
	{
		for (int i = 0; i < getLayoutCnt(); i++)
			if (getLayout(i).getActive())
				getLayout(i).RenderNames();
	}
	void updateLayerPositions()
	{
		for (int i = 0; i < getLayoutCnt(); i++)
			if (getLayout(i).getActive())
				getLayout(i).updateLayerPositions();
	}
	void clearSelections()
	{
		for (int i = 0; i < getLayoutCnt(); i++)
			if (getLayout(i).getActive())
				getLayout(i).clearSelections();
	}
};
