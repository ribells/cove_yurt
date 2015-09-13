 /* =============================================================================
	File: timer.cpp

  =============================================================================== */

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include "stdio.h"
#include "string.h"
#include "utility.h"
#include "timer.h"

CTimerSet g_Timers;

//  TIMER CLASS
/* =============================================================================
 =============================================================================== */
CTimer::CTimer(bool bHighPerfTimer)
{
	
	m_HighPerfTimer = bHighPerfTimer;

	//  set the performance timer's freqency
#ifdef WIN32
	if (m_HighPerfTimer)
	{
		LARGE_INTEGER	performanceFreq;
		QueryPerformanceFrequency(&performanceFreq);

		unsigned int	loFreq = (unsigned int) performanceFreq.LowPart;
		mTimerFrequency = 1.0 / double(loFreq);
		mStartTime = 0.0f;
		mCurrentTime = 0.0f;
		sameCount = 0;
		firstCount = true;
		return;
	}

	mTimerFrequency = 1000.0;
#else
	mTimerFrequency = 10000.0;
#endif
	mStartTime = getCurTime();
	mCurrentTime = mStartTime;
}

/* =============================================================================
 =============================================================================== */
double CTimer::getCurTime()
{
	double	tmpTime;
#ifdef WIN32
	if (m_HighPerfTimer)
	{
		unsigned int	tempCount;
		LARGE_INTEGER	performanceCount;
		QueryPerformanceCounter(&performanceCount);
		tempCount = (unsigned int) performanceCount.LowPart;
		if (firstCount)
		{
			oldCount = tempCount;
			firstCount = false;
		}
		else
		{
			if (tempCount < oldCount)

				//  make sure that count continues when timer resets
				oldCount = tempCount;
			else
			{
				mCurrentTime += double(tempCount - oldCount) * mTimerFrequency;
				oldCount = tempCount;

				if (mCurrentTime == mLastTime)
				{
					sameCount++;
					if (sameCount >= 100000)
					{

						//  just in case...
						mCurrentTime += 1.0;
						sameCount = 0;
					}
				}
				else
					sameCount = 0;

				mLastTime = mCurrentTime;
			}
		}

		return mCurrentTime;
	}

	tmpTime = GetTickCount();
#else
	struct timeval	tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	tmpTime = tv.tv_usec / 100 + (tv.tv_sec) * 10000;
#endif
	mCurrentTime = tmpTime / mTimerFrequency;
	return mCurrentTime;
}

/* =============================================================================
 =============================================================================== */
void CTimer::resetTimer()
{
	mStartTime = getCurTime();
	mCurrentTime = mStartTime;
}

/* =============================================================================
 =============================================================================== */
double CTimer::elapsedTime()
{
	getCurTime();
	return mCurrentTime - mStartTime;
}

double CTimer::lastTime()
{
	return mCurrentTime - mStartTime;
}

//  SYSTEM TIMERS
/* =============================================================================
 =============================================================================== */

void CTimerSet::setHighPerfTimer(bool bSet)
{
	for (int i = 0; i < NUM_TIMERS; i++) 
		m_Timer[i] = CTimer(bSet);
}

/* =============================================================================
 =============================================================================== */
void CTimerSet::startTimer(TimerType t)
{
	m_Timer[t].resetTimer();
}

/* =============================================================================
 =============================================================================== */
double CTimerSet::pollTimer(TimerType t)
{
	return m_Timer[t].elapsedTime();
}

double CTimerSet::lastTimerTime(TimerType t)
{
	return m_Timer[t].lastTime();
}

/* =============================================================================
 =============================================================================== */

void CTimerSet::setFrameTime(double dt)
{
	m_LastFrame = max(1e-4, dt);
	m_FrameTimes[m_FrameCounter] = m_LastFrame;
	m_Framefps = 0;
	for (int i = 0; i < FPS_BUFFER; i++) 
	{
		m_Framefps += m_FrameTimes[i];
	}
	m_Framefps = m_Framefps == 0.0 ? 0.0 : (double) FPS_BUFFER / m_Framefps;
	m_FrameCounter = (m_FrameCounter + 1) % FPS_BUFFER;
}

/* =============================================================================
 =============================================================================== */
double CTimerSet::getFrameRate()
{
	return m_Framefps;
}

/* =============================================================================
 =============================================================================== */
double CTimerSet::getLastFrameRate()
{
	return 1.0 / m_LastFrame;
}

/* =============================================================================
 =============================================================================== */

void CTimerSet::resetPerfData()
{
	for (int i = 0; i < NUM_PERF; i++) 
		m_PerfTimes[i] = 0;
}

/* =============================================================================
 =============================================================================== */

void CTimerSet::resetPerfData(TimerType t)
{
	m_PerfTimes[t] = 0;
}

/* =============================================================================
 =============================================================================== */
double CTimerSet::getPerfTimer(TimerType t)
{
	if (t >= NUM_PERF)
		return 0;
	return m_PerfTimes[t];
}

/* =============================================================================
 =============================================================================== */
void CTimerSet::setPerfTimer(TimerType t, double dval)
{
	if (t >= NUM_PERF)
		return;
	m_PerfTimes[t] = dval;
}

/* =============================================================================
 =============================================================================== */
string CTimerSet::getPerfDataString()
{
	static string Perf_Name[NUM_PERF] = { "GPU   ", "CPU   ", "IO    ", "NET   ", "WFIO  ", "WFNET ", "VISNET" };

	ostringstream	s;
	s.precision(2);
	s.setf(ios::fixed, ios::floatfield);
	s << "  Performance Data" << endl;

	double	tot = 0;
	for (int i = 0; i < NUM_PERF; i++)
	{
		s << "    " << Perf_Name[i] << ": " << m_PerfTimes[i] << " seconds" << endl;
		tot += m_PerfTimes[i];
	}

	s << "    Total: " << tot << " seconds" << endl;
	return s.str();
}

/* =============================================================================
 =============================================================================== */
void CTimerSet::startPerfTimer(TimerType t)
{
	startTimer(t);
}

/* =============================================================================
 =============================================================================== */
void CTimerSet::stopPerfTimer(TimerType t)
{
	double	dt = pollTimer(t);
	m_PerfTimes[t] += dt;
	if (t == GPU_PERF_TIMER) 
		setFrameTime(dt);
}

/* =============================================================================
 =============================================================================== */

void CTimerSet::resetProfileData()
{
	memset(m_ProfTimes, 0, sizeof(m_ProfTimes));
	memset(m_ProfCnts, 0, sizeof(m_ProfCnts));
}

/* =============================================================================
 =============================================================================== */
void CTimerSet::startProfileSection()
{
	m_Timer[PROFILE_TIMER].resetTimer();
}

/* =============================================================================
 =============================================================================== */
void CTimerSet::endProfileSection(int i)
{
	if (i >= MAX_PROF)
	{
		ostringstream	s;
		s << "Profile section " << i << " not valid" << endl;
		cout << s.str();
		return;
	}

	double dtime = m_Timer[PROFILE_TIMER].elapsedTime();
	m_ProfTimes[i] += dtime;
	m_ProfCnts[i]++;
	m_Timer[PROFILE_TIMER].resetTimer();
}

/* =============================================================================
 =============================================================================== */
string CTimerSet::getProfileDataString()
{
	ostringstream	s;
	s << "Profile Data" << endl;
	for (int i = 0; i < MAX_PROF; i++)
		s << "Section " << i << ": runcount " << m_ProfCnts[i] << " " << m_ProfTimes[i] << endl;

	return s.str();
}
