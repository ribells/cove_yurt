#pragma once

#include "xmlParser.h"
#include "object.h"
#include "line.h"
#include <string>
#include <sstream>

//==========[ class CLayout]==================================================

class CTemplate : public CLayer
{
private:
	vector<CObjectPtr> m_ObjectList;
	vector<CLinePtr>   m_LineList;

public:
	CTemplate() {}
	~CTemplate() { clean(); }

	void clean()
	{
		clearLines();
		clearObjects();
	}

	CTemplate(CTemplate const &t)
	{
		*this = t;
	}
	CTemplate & operator = (const CTemplate &t)
	{
		CLayer::operator =(t);
		m_ObjectList.clear();
		m_LineList.clear();
		for (int i = 0; i < t.getObjectCnt(); i++) 
			addObject(t.getObject(i));
		for (int i = 0; i < t.getLineCnt(); i++)
		{
			addLine(t.getLine(i));

			CLine	&Line = getLine(i);

			//  fix up the ends
			for (int j = 0; j < 2; j++)
			{
				if (Line.getEndObjectPtr(j) == 0)
					continue;

				int ndx = t.getObjectIndex(Line.getEndObjectPtr(j));
				if (ndx == -1)
					continue;
				Line.setEndObject(j, getObjectPtr(ndx));
			}
		}
		return *this;
	}

	string getInfoText();

	//object list methods
	int getObjectCnt() const { return m_ObjectList.size(); }
	bool getObjectValid(int i) const { return i >= 0 && i < getObjectCnt(); }

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

	bool addObject(CObject & Object, int iNdx = -1)
	{
		CObjectPtr pObject = new CObject();
		*pObject = Object;
		m_ObjectList.push_back(pObject);
		return true;
	}
	bool delObject(int iNdx)
	{ 
		if (!getObjectValid(iNdx))
			return false;
		delete (m_ObjectList[iNdx]);
		m_ObjectList.erase(m_ObjectList.begin() + iNdx);
		return true;
	}

	//line list methods
	int getLineCnt() const { return m_LineList.size(); }
	bool getLineValid(int i) const { return i >= 0 && i < getLineCnt(); }

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

	bool addLine(CLine & Line, int iNdx = -1)
	{
		CLinePtr pLine = new CLine();
		*pLine = Line;
		m_LineList.push_back(pLine);
		return true;
	}
	bool delLine(int iNdx)
	{ 
		if (!getLineValid(iNdx))
			return false;
		delete (m_LineList[iNdx]);
		m_LineList.erase(m_LineList.begin() + iNdx);
		return true;
	}

	bool isValid() const { return getObjectCnt()>0 || getLineCnt()>0; }

	double getCost() const
	{ 
		double cost = 0;
		for (int i = 0; i < getObjectCnt(); i++)
			cost += getObject(i).getObjectType().getCost();
		return cost;
	}
	double getInstallCost() const
	{ 
		double cost = 0;
		for (int i = 0; i < getObjectCnt(); i++)
			cost += getObject(i).getObjectType().getInstallCost();
		return cost;
	}
	double getDataCost() const
	{ 
		double cost = 0;
		for (int i = 0; i < getObjectCnt(); i++)
			cost += getObject(i).getObjectType().getDataCost();
		return cost;
	}
	double getPowerCost() const
	{ 
		double cost = 0;
		for (int i = 0; i < getObjectCnt(); i++)
			cost += getObject(i).getObjectType().getPowerCost();
		return cost;
	}
};

typedef CTemplate * CTemplatePtr;

class CLayoutTypes
{
private:
	string	m_Name;
	string	m_Description;
	string	m_Link;
	string	m_LocalFilePath;

	bool	m_Dirty;

	vector<CObjectTypePtr>	m_ObjectTypeList;
	vector<CLineTypePtr>	m_LineTypeList;
	vector<CTemplatePtr>	m_TemplateList;

	CTemplate m_CopySet;

public:

	CLayoutTypes() : m_Dirty(false) {}
	~CLayoutTypes()
	{
		clearLineTypes();
		clearObjectTypes();
		clearTemplates();
	}

	bool deserialize_Node(XMLNode &xn0,  string strTag, int iXmlNdx = 0, int iRecNdx = -1);
	bool serialize_Node(XMLNode &xn0,  string strTag, int iRecNdx);

	bool deserialize_Record(string strText, string strTag, int iRecNdx = -1);
	string serialize_Record(string strTag, int iRecNdx);

	bool deserialize(XMLNode &xn0);
	void serialize(XMLNode &xn0);

	bool readFile(string fileName);
	void writeFile(string fileName);

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	string getDescription() const { return m_Description; }
	void setDescription(string str) { m_Description = str; }
	string getLink() const { return m_Link; }
	void setLink(string str) { m_Link = str; }
	string getLocalFilePath() const { return m_LocalFilePath; }
	void setLocalFilePath(string str) { m_LocalFilePath = str; }

	bool getDirty() const { return m_Dirty; }
	void setDirty(bool b) { m_Dirty = b; }

	int getObjectTypeCnt() const { return m_ObjectTypeList.size(); }
	int getObjectTypeValid(int i) const { return i >= 0 && i < getObjectTypeCnt(); }
	void clearObjectTypes()
	{
		while (getObjectTypeCnt())
			delObjectType(0);
	}

	CObjectType & getObjectType(int iNdx) const
	{ 
		assert(getObjectTypeValid(iNdx));
		return *m_ObjectTypeList[iNdx];
	}

	int getObjectNameIndex(string name) const
	{ 
		for (int i = 0; i < getObjectTypeCnt(); i++)
			if (lwrstr(name) == lwrstr(getObjectType(i).getName()))
				return i;
		return -1;
	}
	int getObjectTypeIndex(string id) const
	{ 
		for (int i = 0; i < getObjectTypeCnt(); i++)
			if (lwrstr(id) == lwrstr(getObjectType(i).getID()))
				return i;
		return -1;
	}
	int getObjectTypeIndex( const CObjectTypePtr pObjectType) const
	{ 
		for (int i = 0; i < getObjectTypeCnt(); i++)
			if (pObjectType == getObjectTypePtr(i))
				return i;
		return -1;
	}

	const CObjectTypePtr getObjectTypePtr(int iNdx) const
	{ 
		if (!getObjectTypeValid(iNdx))
			return NULL;
		return m_ObjectTypeList[iNdx];
	}
	const CObjectTypePtr getObjectTypePtr(string id) const
	{ 
		return getObjectTypePtr(getObjectTypeIndex(id));
	}

	bool addObjectType(CObjectType & ObjectType, int iNdx = -1)
	{
		if (!ObjectType.getID().size())
			ObjectType.setID("OTYPE");
		if (getObjectTypeIndex(ObjectType.getID()) != -1)
		{
			string str;
			for (int i = 1; getObjectTypeIndex(str = getNewName(ObjectType.getID(), i)) != -1; i++);
			ObjectType.setID(str);
		}
		if (getObjectNameIndex(ObjectType.getName()) != -1)
		{
			string str;
			for (int i = 1; getObjectNameIndex(str = getNewName(ObjectType.getName(), i)) != -1; i++);
			ObjectType.setName(str);
		}
		if (!ObjectType.getName().size())
			ObjectType.setName("ObjectType " + ObjectType.getID());
		if (iNdx == -1 || iNdx >= m_ObjectTypeList.size())
		{
			m_ObjectTypeList.push_back(new CObjectType());
			iNdx = getObjectTypeCnt()-1;
		}
		else {
			vector<CObjectTypePtr> :: iterator iter = m_ObjectTypeList.begin();
			for (int i = 0; iter != m_ObjectTypeList.end() && i < iNdx; iter++, i++);
			m_ObjectTypeList.insert(iter, new CObjectType());
		}
		return editObjectType(ObjectType, iNdx);
	}

	bool editObjectType(CObjectType & ObjectType, int iNdx)
	{
		if (!getObjectTypeValid(iNdx))
			return false;
		getObjectType(iNdx) = ObjectType;
		return true;
	}

	bool delObjectType(int iNdx)
	{ 
		if (!getObjectTypeValid(iNdx))
			return false;
		delete (m_ObjectTypeList[iNdx]);
		m_ObjectTypeList.erase(m_ObjectTypeList.begin() + iNdx);
		return true;
	}

	bool moveObjectType(int iNdx, bool bUp)
	{ 
		if (!getObjectTypeValid(iNdx))
			return false;
		string strGroup = getObjectType(iNdx).getGroup();
		int iSwap = -1;
		for (int i = 0; i < getObjectTypeCnt(); i++)
		{
			if (bUp && i == iNdx)	break;
			if (strGroup != getObjectType(i).getGroup())	continue;
			iSwap = i;
			if (!bUp && i > iNdx)	break;
		}
		if (iSwap == -1)
			return false;
		CObjectTypePtr tmp = m_ObjectTypeList[iNdx];
		m_ObjectTypeList[iNdx] = m_ObjectTypeList[iSwap];
		m_ObjectTypeList[iSwap] = tmp;
		return true;
	}

	void unloadMaterials()
	{
		for (int i = 0; i < getObjectTypeCnt(); i++)
		{
			getObjectType(i).editModel().unloadMaterials();
			deleteTexture(getObjectType(i).getIconTexture());
		}
		clearQuadrics();
	}

	void reloadMaterials()
	{
		for (int i = 0; i < getObjectTypeCnt(); i++)
		{
			getObjectType(i).setModel();
			getObjectType(i).setIcon();
		}
	}


	int getLineTypeCnt() const { return m_LineTypeList.size(); }
	int getLineTypeValid(int i) const { return i >= 0 && i < getLineTypeCnt(); }
	void clearLineTypes()
	{
		while (getLineTypeCnt())
			delLineType(0);
	}

	CLineType & getLineType(int iNdx) const
	{ 
		assert(getLineTypeValid(iNdx));
		return *m_LineTypeList[iNdx];
	}

	int getLineNameIndex(string name) const
	{ 
		for (int i = 0; i < getLineTypeCnt(); i++)
			if (lwrstr(name) == lwrstr(getLineType(i).getName()))
				return i;
		return -1;
	}
	int getLineTypeIndex(string id) const
	{ 
		for (int i = 0; i < getLineTypeCnt(); i++)
			if (lwrstr(id) == lwrstr(getLineType(i).getID()))
				return i;
		return -1;
	}
	int getLineTypeIndex( const CLineTypePtr pLineType) const
	{ 
		for (int i = 0; i < getLineTypeCnt(); i++)
			if (pLineType == getLineTypePtr(i))
				return i;
		return -1;
	}
	const CLineTypePtr getLineTypePtr(int iNdx) const
	{ 
		if (!getLineTypeValid(iNdx))
			return NULL;
		return m_LineTypeList[iNdx];
	}
	const CLineTypePtr getLineTypePtr(string id) const
	{ 
		return getLineTypePtr(getLineTypeIndex(id));
	}

	bool addLineType(CLineType & LineType, int iNdx = -1)
	{
		if (!LineType.getID().size())
			LineType.setID("LTYPE");
		if (getLineTypeIndex(LineType.getID()) != -1)
		{
			string str;
			for (int i = 1; getLineTypeIndex(str = getNewName(LineType.getID(), i)) != -1; i++);
			LineType.setID(str);
		}
		if (getLineNameIndex(LineType.getName()) != -1)
		{
			string str;
			for (int i = 1; getLineNameIndex(str = getNewName(LineType.getName(), i)) != -1; i++);
			LineType.setName(str);
		}
		if (!LineType.getName().size())
			LineType.setName("LineType " + LineType.getID());
		if (iNdx == -1 || iNdx >= m_LineTypeList.size())
		{
			m_LineTypeList.push_back(new CLineType());
			iNdx = getLineTypeCnt()-1;
		}
		else
		{
			vector<CLineTypePtr> :: iterator iter = m_LineTypeList.begin();
			for (int i = 0; iter != m_LineTypeList.end() && i < iNdx; iter++, i++);
			m_LineTypeList.insert(iter, new CLineType());
		}
		return editLineType(LineType, iNdx);
	}

	bool editLineType(CLineType & LineType, int iNdx)
	{
		if (!getLineTypeValid(iNdx))
			return false;
		getLineType(iNdx) = LineType;
		return true;
	}

	bool delLineType(int iNdx)
	{ 
		if (!getLineTypeValid(iNdx))
			return false;
		delete (m_LineTypeList[iNdx]);
		m_LineTypeList.erase(m_LineTypeList.begin() + iNdx);
		return true;
	}

	bool moveLineType(int iNdx, bool bUp)
	{ 
		if (!getLineTypeValid(iNdx))
			return false;
		string strGroup = getLineType(iNdx).getGroup();
		int iSwap = -1;
		for (int i = 0; i < getLineTypeCnt(); i++)
		{
			if (bUp && i == iNdx)	break;
			if (strGroup != getLineType(i).getGroup())	continue;
			iSwap = i;
			if (!bUp && i > iNdx)	break;
		}
		if (iSwap == -1)
			return false;
		CLineTypePtr tmp = m_LineTypeList[iNdx];
		m_LineTypeList[iNdx] = m_LineTypeList[iSwap];
		m_LineTypeList[iSwap] = tmp;
		return true;
	}

	CTemplate & getCopySet() { return m_CopySet; }
	void setCopySet(CTemplate & pSet) { m_CopySet = pSet; }

	int getTemplateCnt() const { return m_TemplateList.size(); }
	int getTemplateValid(int i) const { return i >= 0 && i < getTemplateCnt(); }
	void clearTemplates()
	{
		while (getTemplateCnt())
			delTemplate(0);
	}

	CTemplate & getTemplate(int iNdx) const
	{ 
		assert(getTemplateValid(iNdx));
		return *m_TemplateList[iNdx];
	}

	int getTemplateIndex(string name) const
	{ 
		for (int i = 0; i < getTemplateCnt(); i++)
			if (name == getTemplate(i).getName())
				return i;
		return -1;
	}
	int getTemplateIndex( const CTemplatePtr hTemplate) const
	{ 
		for (int i = 0; i < getTemplateCnt(); i++)
			if (hTemplate == getTemplatePtr(i))
				return i;
		return -1;
	}
	const CTemplatePtr getTemplatePtr(int iNdx) const
	{ 
		if (!getTemplateValid(iNdx))
			return NULL;
		return m_TemplateList[iNdx];
	}
	const CTemplatePtr getTemplatePtr(string name) const
	{ 
		return getTemplatePtr(getTemplateIndex(name));
	}

	bool addTemplate(CTemplate & Template, int iNdx = -1)
	{
		if (!Template.getName().size())
			Template.setName("Template");
		if (getTemplateIndex(Template.getName()) != -1) 
		{
			string str;
			for (int i = 1; getTemplateIndex(str = getNewName(Template.getName(), i)) != -1; i++);
			Template.setName(str);
		}
		if (iNdx == -1 || iNdx >= m_TemplateList.size()) 
		{
			m_TemplateList.push_back(new CTemplate());
			iNdx = getTemplateCnt()-1;
		}
		else
		{
			vector<CTemplatePtr> :: iterator iter = m_TemplateList.begin();
			for (int i = 0; iter != m_TemplateList.end() && i < iNdx; iter++, i++);
			m_TemplateList.insert(iter, new CTemplate());
		}
		return editTemplate(Template, iNdx);
	}

	bool editTemplate(CTemplate & Template, int iNdx)
	{
		if (!getTemplateValid(iNdx))
			return false;
		getTemplate(iNdx) = Template;
		return true;
	}

	bool delTemplate(int iNdx)
	{ 
		if (!getTemplateValid(iNdx))
			return false;
		delete (m_TemplateList[iNdx]);
		m_TemplateList.erase(m_TemplateList.begin() + iNdx);
		return true;
	}

	bool moveTemplate(int iNdx, bool bUp)
	{ 
		if (!getTemplateValid(iNdx))
			return false;
		string strGroup = getTemplate(iNdx).getGroup();
		int iSwap = -1;
		for (int i = 0; i < getTemplateCnt(); i++)
		{
			if (bUp && i == iNdx)	break;
			if (strGroup != getTemplate(i).getGroup())	continue;
			iSwap = i;
			if (!bUp && i > iNdx)	break;
		}
		if (iSwap == -1)
			return false;
		CTemplatePtr tmp = m_TemplateList[iNdx];
		m_TemplateList[iNdx] = m_TemplateList[iSwap];
		m_TemplateList[iSwap] = tmp;
		return true;
	}

};

typedef CLayoutTypes * CLayoutTypesPtr;
