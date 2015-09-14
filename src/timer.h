#pragma once

#include <vector>
#include <string>
using namespace std;

#define NUM_BASE_PERF 4
#define NUM_PERF 7

typedef enum TimerType
{ 
	GPU_PERF_TIMER,		//total gpu usage
	CPU_PERF_TIMER,		//total cpu usage
	IO_PERF_TIMER,		//total io overhead
	NET_PERF_TIMER,		//total network overhead
	WFIO_PERF_TIMER,	//total workflow io overhead
	WFNET_PERF_TIMER,	//total workflow network overhead
	VISNET_PERF_TIMER,	//total visual network overhead

	COVE_TIMER,			//timer started at beginning of COVE
	TRN_RENDER_TIMER,	//time since starting terrain render
	DATA_RENDER_TIMER,	//time since starting data render
	CAMERA_TIMER,		//time since starting current camera operation
	FADE_TIMER,			//time since starting fading between screens
	VIDEO_TIMER,		//timer for dataset video
	MOUSE_MOVE_TIMER,	//time since mouse last moved
	PROFILE_TIMER,		//performance profiling
	NUM_TIMERS
} TimerType;

class CTimer 
{
private:
	double	mTimerFrequency;	// frequency of the performance counter
	double	mCurrentTime;		// current time as of last call to getCurrentTime()
	double	mStartTime;		// current time as of last call to resetTimer()

	double	mLastTime;		// current time as of second to last call to getCurrentTime()
	unsigned int	sameCount;
	unsigned int	oldCount;
	bool			firstCount;

	bool			m_HighPerfTimer;

public:

	CTimer(bool bHighPerfTimer = true);

	double getCurTime();
	void resetTimer();
	double elapsedTime();
	double lastTime();
};

#define FPS_BUFFER  10
#define MAX_PROF	10

class CTimerSet
{
	CTimer		m_Timer[NUM_TIMERS];
		//  Frame Timer
	double		m_FrameTimes[FPS_BUFFER];
	int			m_FrameCounter;
	double		m_Framefps;
	double		m_LastFrame;
		//  Task performance times
	double		m_PerfTimes[NUM_PERF];
		//  Profiling
	double		m_ProfTimes[MAX_PROF];	//  total times in each section
	int			m_ProfCnts[MAX_PROF];	//  number of times each section entered

private:
	void setFrameTime(double dt);

public:
	CTimerSet() : m_Framefps(0), m_LastFrame(0), m_FrameCounter(0)
	{
		for (int i = 0; i < FPS_BUFFER; i++)
			m_FrameTimes[i] = 0.0;
		for (int i = 0; i < NUM_PERF; i++)
			m_PerfTimes[i] = 0.0;
		for (int i = 0; i < MAX_PROF; i++)
		{
			m_ProfTimes[i] = 0.0;
			m_ProfCnts[i] = 0;
		}
	}
		//timer start and stop
	void setHighPerfTimer(bool bSet);
	void startTimer(TimerType t);
	double pollTimer(TimerType t);
	double lastTimerTime(TimerType t);

		//performance profiling
	void startProfileSection();
	void endProfileSection(int i);
	void resetProfileData( );
	string getProfileDataString();

		//frame rate monitoring
	double getFrameRate();
	double getLastFrameRate();
	double getWorstFrameRate();

		//task specific performance monitoring
	void resetPerfData();
	void resetPerfData(TimerType t);
	double getPerfTimer(TimerType t);
	void setPerfTimer(TimerType t, double dval);
	string getPerfDataString();
	void startPerfTimer(TimerType t);
	void stopPerfTimer(TimerType t);
};

extern CTimerSet	g_Timers;

