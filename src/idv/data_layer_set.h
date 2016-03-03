#pragma once

#include "xmlParser.h"
#include "utility.h"

#include "object.h"
#include "data_layer.h"

//==========[ class CDataSet ]==================================================

class CTimeLine
{
private:

	double	m_Time;		//current time
	double	m_FrameTime;	//offset from second for frame accurate timing

	double	m_Start;	//start time
	double	m_Finish;	//finish time
	double	m_SelStart;
	double	m_SelFinish;

	double	m_Lead;
	double	m_Trail;

	double	m_Speed;	//conversion from timer time to data time - number of data seconds for each timer second

	double	m_TimerBase;	//system timer base time in seconds

	bool	m_Play;
	bool	m_Reverse;
	bool	m_Loop;
	bool	m_Dirty;

public:

	CTimeLine()
	{
		m_Dirty = true;
		m_Play = false;
		m_Reverse = false;
		m_Loop = true;
		m_TimerBase = 0;
		m_Speed = 1.0f;
		m_Start = m_Finish = m_SelStart = m_SelFinish = m_Time = getCurrentTime();
		m_Lead = NO_TIME;
		m_Trail = 0;
		m_FrameTime = 0;
	}
	~CTimeLine() {}

	double getTime() const { return m_Time; }
	void setTime(double t) { m_Time = t; m_Dirty = true; }
	double getFrameTime() const { return m_FrameTime; }
	double getSpeed() { return m_Speed; }

	double getStart() const { return m_Start; }
	void setStart(double d) { m_Start = d; }
	double getFinish() const { return m_Finish; }
	void setFinish(double d) { m_Finish = d; }

	double getSelStart() const { return m_SelStart; }
	void setSelStart(double d) { m_SelStart = d; }
	double getSelFinish() const { return m_SelFinish; }
	void setSelFinish(double d) { m_SelFinish = d; }

	double getLead() const { return m_Lead; }
	void setLead(double t) { m_Lead = t; }
	double getTrail() const { return m_Trail; }
	void setTrail(double t) { m_Trail = t; }

	void updateTime();

	void updateBounds(double start, double finish);
	void resetSpeed(double fSeconds = 30.0);

	void resetTimer();
	void setPlay(bool bPlay, bool bReverse = false);
	bool getPlaying() const { return m_Play; }
	bool getReverse() const { return m_Reverse; }
	void setCurTime(double tm);
	bool updateCurTime();	//figure out the current time
	void setFaster();
	void setSlower();
	bool getLoop() const { return m_Loop; }
	void setLoop(bool b) { m_Loop = b; }

	string getTimeString(double dTime);
};

typedef CDataLayer * CDataLayerPtr;

class CDataSet
{
private:
	vector<CDataLayerPtr> m_DataLayerList;

	double	m_Time;		//current time in data display in seconds
	double	m_FrameTime;	//offset in seconds from start time - to allow per frame resolution

	t_scale	m_Scale;
	double	m_Shading;

public:

	CDataSet() : m_Time(0), m_FrameTime(0)
	{
		m_Scale.land = m_Scale.sea = 2.0;
		m_Shading = 1.0;
	}
	~CDataSet() 
	{
		while (getDataLayerCnt())
			delDataLayer(0);
	}

	void setCurTime(double t, double s = 0) { m_Time = t; m_FrameTime = s; }

	bool PrepareDataSets(Vec3f &pos, Vec3f &dir, int &cam_type);
	void Render(double dSpeed, bool bSelect = false) const;
	string getInfoText(int iSet, int iSel) const
	{
		if (iSet >= getDataLayerCnt())
			return "bad point";
		if (iSel >= getDataLayer(iSet).getDataMax())
			return "bad point";
		return getDataLayer(iSet).getInfoText(iSel);
	}

	int getDataLayerCnt() const { return m_DataLayerList.size(); }
	bool getDataLayerValid(int i) const { return i >= 0 && i < getDataLayerCnt(); }
	CDataLayer & getDataLayer(CDataLayerPtr pDataLayer) const { return *pDataLayer; }
	void resetLayerPtrs()
	{
		for (int i = 0; i < getDataLayerCnt(); i++)
		{
			if (i == 0)
				m_DataLayerList[i]->setPrevLayer(NULL);
			else
				m_DataLayerList[i]->setPrevLayer(m_DataLayerList[i-1]);
			if (i == m_DataLayerList.size()-1)
				m_DataLayerList[i]->setNextLayer(NULL);
			else
				m_DataLayerList[i]->setNextLayer(m_DataLayerList[i+1]);
		}
	}

	CDataLayer & getDataLayer (int iNdx) const
	{ 
		assert(getDataLayerValid(iNdx));
		return *m_DataLayerList[iNdx];
	}
	int getDataLayerIndex(string name) const
	{ 
		for (int i = 0; i < getDataLayerCnt(); i++)
			if (name == getDataLayer(i).getName())
				return i;
		return -1;
	}
	const CDataLayerPtr getDataLayerPtr(int iNdx) const
	{ 
		if (!getDataLayerValid(iNdx))
			return NULL;
		return m_DataLayerList[iNdx];
	}
	int getDataLayerIndex(const CDataLayerPtr pDataLayer) const
	{ 
		for (int i = 0; i < getDataLayerCnt(); i++)
			if (pDataLayer == m_DataLayerList[i])
				return i;
		return -1;
	}

	int getActiveCnt() const
	{
		int iActive = 0;
		for (int i = 0; i < getDataLayerCnt(); i++)
			if (getDataLayer(i).getActive())
				iActive++;
		return iActive;
	}

	bool addDataLayer(CDataLayer & DataLayer, int iNdx = -1)
	{
		if (!DataLayer.getName().size())
			DataLayer.setName("New");
		if (!getDataLayerValid(iNdx))
		{
			m_DataLayerList.push_back(new CDataLayer());
			iNdx = getDataLayerCnt()-1;
		}
		else 
		{
			vector<CDataLayerPtr> :: iterator iter = m_DataLayerList.begin();
			for (int i = 0; iter != m_DataLayerList.end() && i < iNdx; iter++, i++);
			m_DataLayerList.insert(iter, new CDataLayer());
		}
		return editDataLayer(DataLayer, iNdx);
	}

	bool editDataLayer(CDataLayer & DataLayer, int iNdx)
	{
		if (!getDataLayerValid(iNdx))
			return false;
		getDataLayer(iNdx) = DataLayer;
		resetLayerPtrs();
		return true;
	}

	bool delDataLayer(int iNdx)
	{ 
		if (!getDataLayerValid(iNdx))
			return false;
		delete (m_DataLayerList[iNdx]);
		m_DataLayerList.erase(m_DataLayerList.begin() + iNdx);
		resetLayerPtrs();
		return true;
	}

	bool moveDataLayerUp(int iNdx)
	{ 
		if (!getDataLayerValid(iNdx))
			return false;
		string strGroup = getDataLayer(iNdx).getGroup();
		int iSwap = -1;
		for (int i = 0; i < getDataLayerCnt(); i++)
		{
			if (i == iNdx)	
				break;
			if (strGroup != getDataLayer(i).getGroup())	
				continue;
			iSwap = i;
		}
		if (iSwap == -1)
			return false;
		CDataLayerPtr tmp = m_DataLayerList[iNdx];
		m_DataLayerList[iNdx] = m_DataLayerList[iSwap];
		m_DataLayerList[iSwap] = tmp;
		resetLayerPtrs();
		return true;
	}

	void getTimeBounds(double &start, double &finish);

	bool setActive(int ndx, bool b) 
	{ 
		if (!getDataLayer(ndx).setActive(b))
			return false;
		return true;
	}
	int getFirstActive(int iStart = 0)  const
	{ 
		for (int i = iStart; i < getDataLayerCnt(); i++)
		{
			if (getDataLayer(i).getActive())
				return i;
		}
		return -1;
	}

	void redrawDataLayers() 
	{ 
		for (int i = 0; i < getDataLayerCnt(); i++)
			getDataLayer(i).setDirty();
	}

	void unloadMaterials()
	{
		for (int i = 0; i < getDataLayerCnt(); i++)
			if (getDataLayer(i).getActive())
				getDataLayer(i).editGradient().clean();
	}
	void reloadMaterials()
	{
		for (int i = 0; i < getDataLayerCnt(); i++)
			if (getDataLayer(i).getActive())
				getDataLayer(i).setGradientTex();
	}
	void setDirty() 
	{
		for (int i = 0; i < getDataLayerCnt(); i++)
			getDataLayer(i).setDirty();
	}
	void setScale(t_scale scale)
	{
		m_Scale = scale;
		setDirty();
	}
	void setShading(float fval)
	{
		m_Shading = fval;
		setDirty();
	}
	void setLead(double tm)
	{
		for (int i = 0; i < getDataLayerCnt(); i++)
		{
			getDataLayer(i).setLead(tm);
			getDataLayer(i).setDirty();
		}
	}
	void setTrail(double tm)
	{
		for (int i = 0; i < getDataLayerCnt(); i++)
		{
			getDataLayer(i).setTrail(tm);
			getDataLayer(i).setDirty();
		}
	}
	void setSelStart(double tm)
	{
		for (int i = 0; i < getDataLayerCnt(); i++)
		{
			getDataLayer(i).setSelStart(tm);
			getDataLayer(i).setDirty();
		}
	}
	void setSelFinish(double tm)
	{
		for (int i = 0; i < getDataLayerCnt(); i++)
		{
			getDataLayer(i).setSelFinish(tm);
			getDataLayer(i).setDirty();
		}
	}

};
