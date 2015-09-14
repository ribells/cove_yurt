#pragma once

#include <string>
using namespace std;

#include "vl/VLf.h"
#include "vl/VLd.h"
#include "const.h"

class CLayer
{
protected:
	string	m_Name;
	string	m_Description;
	string	m_Link;
	string	m_LocalFilePath;
	string	m_Group;
	string	m_Tags;

	bool	m_Active;
	bool	m_Reload;
	bool	m_TreeUpdate;

	Vec3d	m_MinPos;
	Vec3d	m_MaxPos;

	Vec3d	m_PosOffset;

	//used when uploading and downloading
	string	m_Author;
	dtime	m_CreateTime;

public:
	CLayer()
	{ 
		m_Active = false;
		m_Reload = false;
		m_TreeUpdate = true;
		m_PosOffset = vl_0;
		m_MinPos = Vec3d(-90,-180,0);
		m_MaxPos = Vec3d(90,180,0);
		m_CreateTime = NO_TIME;
	}

	CLayer & operator = (const CLayer & t)
	{
		setTreeUpdate (t.getName()!=getName() || t.getGroup()!=getGroup() || t.getDescription()!=getDescription());
		setReload(t.getLink()!=getLink());

		m_Name = t.m_Name;
		m_Description = t.m_Description;
		m_Link = t.m_Link;
		m_Group = t.m_Group;
		m_Tags = t.m_Tags;
		m_LocalFilePath = t.m_LocalFilePath;

		m_Active = t.m_Active;
		m_MinPos = t.m_MinPos;
		m_MaxPos = t.m_MaxPos;
		m_PosOffset = t.m_PosOffset;
		m_Author = t.m_Author;
		m_CreateTime = t.m_CreateTime;

		return *this;
	}

	bool getActive() const { return m_Active; }
	virtual bool setActive(bool b) { m_Active = b; return b; }
	void setLoaded(bool b) { m_Active = b; m_Reload = false; }

	bool getReload() const { return m_Reload; }
	void setReload(bool b) { m_Reload = b; }
	bool getTreeUpdate() const { return m_TreeUpdate; }
	void setTreeUpdate(bool b) { m_TreeUpdate = b; }
	bool clearTreeUpdate() { bool b = m_TreeUpdate; m_TreeUpdate = false; return b; }

	string getName() const { return m_Name; }
	void setName(string str) { m_Name = str; }
	string getDescription() const { return m_Description; }
	void setDescription(string str) { m_Description = str; }
	string getLink() const { return m_Link; }
	void setLink(string str) { m_Link = str; }
	string getLocalFilePath() const { return m_LocalFilePath; }
	void setLocalFilePath(string str) { m_LocalFilePath = str; }
	string getTags() const { return m_Tags; }
	void setTags(string str) { m_Tags = str; }
	string getGroup() const { return m_Group; }
	void setGroup(string str) { m_Group = str; }
	string getMenuGroup() const 
	{ 
		string str = m_Group;
		for (int c=0; c < str.size(); c++) 
			if (str[c]==':') 
				str[c]='/';
		return str;
	}

	Vec3d getMinPos() const {return m_MinPos;}
	void setMinPos(Vec3d pos) { m_MinPos = pos; }
	Vec3d getMaxPos() const {return m_MaxPos;}
	void setMaxPos(Vec3d pos) { m_MaxPos = pos; }
	virtual bool getMinMax(Vec3d &min_pos, Vec3d &max_pos) const
	{
		min_pos = m_MinPos;
		max_pos = m_MaxPos;
		return true;
	}

	Vec3d getPosOffset() const {return m_PosOffset;}
	void setPosOffset(Vec3d pos) { m_PosOffset = pos; }

	string getAuthor() const { return m_Author; }
	void setAuthor(string str) { m_Author = str; }
	double getCreateTime() const { return m_CreateTime; }
	void setCreateTime(double tm) { m_CreateTime = tm; }
};
