/* =============================================================================
	File: data_layer_set.cpp

 =============================================================================== */

#include "data_layer_set.h"
#include "timer.h"

CObjectPtr	SetObjectFromName(string strName);
void		SetObjectPosition(CObjectPtr pObject, Vec3f pos, Vec3f dir);

/* =============================================================================
 =============================================================================== */

void CTimeLine::setCurTime(double tm)
{
	m_Time = tm;
	m_Dirty = true;
	resetTimer();
}

/* =============================================================================
 =============================================================================== */
string CTimeLine::getTimeString(double dTime)
{
	return getNiceDateTimeString(dTime, m_Speed * 30);
}

/* =============================================================================
 =============================================================================== */
void CTimeLine::resetTimer()
{
	if (!m_Play) {
		return;
	}
	m_TimerBase = m_Time + m_FrameTime;
	g_Timers.startTimer(VIDEO_TIMER);
}

/* =============================================================================
 =============================================================================== */
void CTimeLine::setPlay(bool bPlay, bool bReverse)
{
	m_Play = bPlay;
	m_Reverse = bReverse;
	resetTimer();
}

/* =============================================================================
 =============================================================================== */
void CTimeLine::setFaster()
{
	m_Speed *= 1.02;
	resetTimer();
}

/* =============================================================================
 =============================================================================== */
void CTimeLine::setSlower()
{
	m_Speed /= 1.02;
	resetTimer();
}

/* =============================================================================
 =============================================================================== */
bool CTimeLine::updateCurTime()
{
	if (!m_Dirty && !m_Play) {
		return false;
	}
	if (m_Play)	//  update the timer time
	{
		double	dTimerTime = g_Timers.pollTimer(VIDEO_TIMER) * m_Speed;
		m_Time = m_Reverse ? m_TimerBase - ceil(dTimerTime) : m_TimerBase + floor(dTimerTime);
		m_FrameTime = m_Reverse ? ceil(dTimerTime) - dTimerTime : dTimerTime - floor(dTimerTime);
	}

	if ((!m_Reverse && m_Time > m_SelFinish) || (m_Reverse && m_Time < m_SelStart))
	{
		if (m_Loop)
			setCurTime(m_Reverse ? m_SelFinish : m_SelStart);
		else
		{
			setPlay(false);
			setCurTime(min(m_SelFinish, max(m_SelStart, m_Time)));
		}
	}

	m_Dirty = false;
	return true;
}

/* =============================================================================
 =============================================================================== */
void CTimeLine::updateBounds(double start, double finish)
{
	if (start == NO_TIME) start = finish < m_Start ? finish : m_Start;
	if (finish == NO_TIME) finish = start > m_Finish ? start : m_Finish;

	m_Start = start;
	m_Finish = max(start, finish);
	m_SelStart = start;
	m_SelFinish = finish;

	if (m_Time < m_Start) setCurTime(m_Start);
	if (m_Time > m_Finish) setCurTime(m_Finish);
	if (m_Finish == m_Start) m_Lead = m_Trail = NO_TIME;
	resetSpeed();
}

/* =============================================================================
 =============================================================================== */
void CTimeLine::resetSpeed(double fSeconds)
{
	m_Speed = 1.0;
	if (fSeconds > 0)
	{
		if (m_SelFinish > m_SelStart)
			m_Speed = (m_SelFinish - m_SelStart) / fSeconds;
		else if (m_Finish > m_Start)
			m_Speed = (m_Finish - m_Start) / fSeconds;
	}
}

/* =============================================================================
    CDATASET
 =============================================================================== */
void CDataSet::getTimeBounds(double &start, double &finish)
{

	//  figure out data start and finish based on data sets
	start = 0;
	finish = 0;
	for (int i = 0; i < getDataLayerCnt(); i++)
	{
		if (getDataLayer(i).getActive())
		{
			if (getDataLayer(i).getStart() < start || start == 0) 
				start = getDataLayer(i).getStart();
			if (getDataLayer(i).getFinish() > 
				finish) finish = getDataLayer(i).getFinish();
		}
	}

	//  figure out default speed
	for (int i = 0; i < getDataLayerCnt(); i++)
	{
		/*
		if (getDataLayer(i).getActive() && getDataLayer(i).getMovieActive())
		{

			//  add to end to show all of video
			int vlen = getDataLayer(i).getMovie().getMovieLength();
			if (finish < start + vlen) finish = start + vlen;
			break;
		}
		*/
	}
}

vector<string>	*getUDPRecords();

/* =============================================================================
 =============================================================================== */

bool CDataSet::PrepareDataSets(Vec3f &pos, Vec3f &dir, int &cam_type)
{
	//  draw out the data
	bool	bCamAttached = false;
	for (int i = 0; i < getDataLayerCnt(); i++)
	{
		CDataLayer	&hLayer = getDataLayer(i);
		if (!hLayer.getActive())
			continue;

		//  figure out data positions
		hLayer.editModel().setShading(m_Shading);
		hLayer.editModel().setScale(m_Scale);

		hLayer.initData(m_Time, m_FrameTime);

		//  attach objects to data if necessary
		if (hLayer.getAttachObjAttached() || hLayer.getShowVideo())
		{
			if (!hLayer.getAttachObjPtr() && hLayer.getAttachObjID().size())
				hLayer.setAttachObj(SetObjectFromName(hLayer.getAttachObjID()));
			if (hLayer.getAttachObjPtr())
				SetObjectPosition(hLayer.getAttachObjPtr(), hLayer.getLastInterpPos(), hLayer.getLastInterpDir());
			else if (!hLayer.getShowVideo())
				continue;
			if (!bCamAttached && hLayer.getCamAttached())
			{
				pos = hLayer.getLastInterpPos();
				dir = hLayer.getLastInterpDir();
				cam_type = hLayer.getCamType();
				bCamAttached = true;
			}
		}
	}

	return bCamAttached;
}

/* =============================================================================
 =============================================================================== */
void CDataSet::Render(double dSpeed, bool bSelect) const
{
	for (int i = 0; i < getDataLayerCnt(); i++)
	{
		CDataLayer	&hLayer = getDataLayer(i);
		if (!hLayer.getActive()) {
			continue;
		}
		//  draw data
		hLayer.RenderData(dSpeed, bSelect ? i : -1);
	}
}
