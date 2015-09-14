#pragma once

#include <float.h>

#ifdef WIN32
#define isnan(x) _isnan((x))
#define finite(x) _finite((x))
#else
#define stricmp strcasecmp 
#define strnicmp strncasecmp 
#endif

#define MAX_INT			(0x7fffffff)

#define MetersPerDegree	(111180.0)
#define KmPerDegree		(111.180)
#define NODATA			(1.00000000000000e-034)
#define	ISNODATA(f)		((bool)((float)f==(float)NODATA))

#define FPS_RATE		(30.0f)
#define FRAME_TIME		(1.0f / FPS_RATE)

#define EARTHMAX		(9000)
#define EARTHMIN		(-11000)

#define MAX_DOUBLE (1.79769e+308)

#ifndef MAX_PATH
#define MAX_PATH (260)
#endif

typedef unsigned int color;
#define CLR_WHITE (0xffffffff)
#define CLR_OFFWHITE (0xfff0f0f0)
#define CLR_RED (0xff0000ff)
#define CLR_BLUE (0xff00ff00)
#define CLR_GREEN (0xffff0000)

#define DB_H	46
#define VB_H	46

#pragma warning(disable : 4267)

#define NO_TIME (-1.0)

#define DUR_SEC (1.0)
#define DUR_MIN (60.0)
#define DUR_HOUR (60.0*DUR_MIN)
#define DUR_DAY (24.0*DUR_HOUR)
#define DUR_WEEK (7.0*DUR_DAY)
#define DUR_MONTH (4.0*DUR_WEEK)

typedef double dtime;


#ifdef WIN32
#ifdef _DEBUG
//uncomment this to turn on mem tracking in debug version
//#define TRACK_MEM_WIN 1
//#define TRACK_MEM_VLD 1
#ifdef TRACK_MEM_WIN
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW 
#endif
#endif
#endif
