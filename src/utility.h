#pragma once

#include <string>
#include <sstream>
#include <assert.h>
#include <stdio.h>

#include "const.h"

using namespace std;

#include <vector>

#include "vl/VLf.h"
#include "vl/VLd.h"

enum {TIME_FMT_MDY, TIME_FMT_DMY, TIME_FMT_DEFAULT};

inline double getdNAN()
{
	unsigned long nan[2] = {0xffffffff, 0x7fffffff};
	return *( double* )nan;
}

inline float getfNAN()
{
	unsigned long nan[1] = {0x7fffffff};
	return *( float* )nan;
}

inline bool isPowerOfTwo (unsigned int x)
{
  return ((x != 0) && !(x & (x - 1)));
}

//void dlg_out(string str_out);

//global app variables
class CEnvironment
{
public:
	string	m_Version;
	string	m_DefaultStartFile;
	string	m_DefaultDataStore;
	string	m_DefaultPath;
	string	m_DefaultWFPath;

	string	m_AppPath;
	string	m_SystemPath;
	string	m_LocalDataPath;
	string	m_LocalCachePath;
	string	m_COVEServerPath;
	string	m_COVEVisServerPath;
	string	m_TridentServerPath;
	string	m_CurFilePath;
	string	m_WorldVersion;
	string	m_DownloadName;

	bool	m_Verbose;
	bool	m_Internet;
	int		m_TimeFormat;
	bool	m_SystemInterrupt;
	bool	m_RerunWF;

	CEnvironment()
	{
		m_Version = "BETA 1.0.0.9400";
		m_DefaultStartFile = "datasvr/worlds/Default.cov";
		m_DefaultDataStore = "cove_data";
		m_DefaultPath = "http://cove.ocean.washington.edu";
		m_DefaultWFPath = "proxy"; // http://trident.ocean.washington.edu/TridentWebServiceSetup/TridentService.asmx";

		m_AppPath = "";
		m_SystemPath = "";
		m_LocalDataPath = "";
		m_LocalCachePath = "";
		m_COVEServerPath = "http://cove.ocean.washington.edu";
		m_COVEVisServerPath = "http://cove.ocean.washington.edu";
		m_TridentServerPath = "";
		m_CurFilePath = "";
		m_WorldVersion = "1.2";
		m_DownloadName = "";

		m_Internet = true;
		m_TimeFormat = TIME_FMT_MDY;
		m_SystemInterrupt = false;
		m_RerunWF = false;
#ifdef _DEBUG
		m_Verbose = true;
#else
		m_Verbose = false;
#endif
	}
};

extern CEnvironment g_Env;

void setDataPath();

// color functions - unsigned int to vec
int clrR(int clr);
int clrG(int clr);
int clrB(int clr);
int clrA(int clr);
color clrRGBA(int r, int g, int b, int a = 255);
color clrRGBA(Vec3f v, float fa = 1.0);
Vec3f clrVec(color c);
float clrTrans(color c);

unsigned char * makeRGBA(const unsigned char *pData, int w, int h);
unsigned char * makeRGB(const unsigned char *pData, int w, int h);
unsigned char * makeBGR(const unsigned char *pData, int w, int h);

//file name handling functions
bool fileexists(string strPath);
dtime filetime(string strPath);
bool renfile(string strPath, string strNewPath);
bool delfile(string strPath);
bool deldir (string dir);
bool changedir(string strPath);
bool findfile(string &pchPath, string strSupported[]);
bool makedir (string newdir);
bool removedir (string dir);
int filesize(string fileName);
bool check_write_access(string fileName);
bool readfile(string fileName, char * &data, int &iLen);
bool writefile(string fileName, const char *data, int iLen);
bool appendfile(string fileName, const char *data, int iLen);
vector<string> listfiles(string strPath, bool bFolders = false);

string lwrstr(string str);
string trimstr(string str);
string cleanstr(string str);

string obscureString(string str);
string unobscureString(string str);
string encode_base64(string src);

string normalizePath(string filePath);
string dosPath(string filePath);
string getFilePath(string fileName);
string getFileName(string fileName);
string getFileNameBase(string file);
string getNewName(string strName, int i);
string getNewFileName(string fileName, int i);

string getExt(string fileName);
string remExt(string fileName);
bool checkExt(string fileName, string ext);
void setExt(string &fileName, string ext);

string getDataServerPath(string strPath);
string getLocalCachePath(string strPath);
bool getIsLocalFile(string strFile);
string getValidFile(string strPath);
string getRandFile(string strBase, string strExt);
string getTempFile(string strExt);

//file parsing functions
bool getCSVTokens(char *buf, vector< vector<string> > &csvTokens);
vector<string> Tokenize(const string& str,const string& delimiters);

//time and position functions
void initTime();
string getNiceDateTimeString(dtime dClock, dtime dRange=0);
string getDateString(dtime dClock);
string getGMTDateTimeString(dtime dClock);
dtime scanDateTime(string strTime, int iFormat = TIME_FMT_DEFAULT);
dtime scanDecimalTime(string strTime);
dtime getDateTime(int year, int month, int day, int hour, int min, int sec);
dtime getCurrentTime();

string getStringFromPosition(Vec3d pos, bool bHTML);
bool getPositionFromString(string strPos, Vec3d &pos);
string getDistanceString(double dist);
string getDegMinSecString(double pos, bool bSec, bool bHTML, bool bPretty = false);
string davesCommas(double f, bool bDecimal = false);
double getKMfromGPS(Vec3d pnt0, Vec3d pnt1);
double getGPSTriangleArea(Vec3d a, Vec3d b, Vec3d c);
bool getUTMfromLL(double lat, double lon, double &zx, double &zy, double &E, double &N);
bool getLLfromUTM(double zx, double zy, double E, double N, double &lat, double &lon);

//Culling functions
void ExtractFrustum();
Vec3f getRealCameraPos();
bool SphereInFrustum( Vec3f center, float radius );
bool getFacingAway(Vec3f cam, Vec3f pnt);

//3d geometry functions
Vec3f closest_Point_on_Line( Vec3f Pnt, Vec3f P0, Vec3f P1, bool bSeg = false);
float dist_Point_to_Line( Vec3f Pnt, Vec3f P0, Vec3f P1, bool bSeg = false);
void CatmullRomPnt(Vec3f &pnt, Vec3f &tgt, double t, Vec3f p[4]);
void CatmullRomValue(float &val, double t, float p[4]);

//converting data from motorola to intel
bool getIntel();
bool swapEndian(bool bIntelData, void * pData, int iSize, int iCnt);

//vector transform functions
Vec3d getVec3d(const float *v);
Vec3f getVec3f(const float *v);
Vec3d getVec3d(Vec3f v);
Vec3f getVec3f(Vec3d v);

inline vector<double> vec2vectord(Vec3d vec)
{
	vector<double> v;  v.resize(3);
	v[0] = vec[0]; v[1] = vec[1]; v[2] = vec[2];
	return v;
}
inline vector<double> vec2vectord(vector<Vec3d> vvec)
{
	int iSize = vvec.size();
	vector<double> v;
	if (iSize == 0)
		return v;
	try {
		v.resize(iSize*3);
	} catch (std::bad_alloc const&) {
		return v;
	}
	double * pv = &v[0];
	for (int i = 0; i < iSize; i++)
		for (int j = 0; j < 3; j++)
			*pv++ = vvec[i][j];
	return v;
}

inline vector<Vec3d> vectord2vec(vector<double> v)
{
	int iSize = v.size()/3;
	vector<Vec3d> vvec;
	try {
		vvec.resize(iSize);
	} catch (std::bad_alloc const&) {
		return vvec;
	}
	double * pv = &v[0];
	for (int i = 0; i < iSize; i++)
		for (int j = 0; j < 3; j++)
			vvec[i][j] = *pv++;
	return vvec;
}

//for memory allocation and testing
void mem_alloc_user_message();
void mem_alloc_failure(const char *pchFile, int iLine, int iSize = 0);
template<class Value>
bool mem_try(Value* &pData, int iSize, const char *pchFile, int iLine)
{
	pData = NULL;
	if (iSize==0) 
		return true;
	try { 
		pData = new Value[iSize];
	} catch (std::bad_alloc x) {
		mem_alloc_failure(pchFile, iLine, iSize);
	}
	return pData != NULL;
}
#define mem_alloc(x, i) (mem_try(x, i, __FILE__,__LINE__))

class Triangulate
{
public:
	// triangulate a contour/polygon, places results in STL vector as series of triangles.
  static bool Process(const vector<Vec3d> &contour, vector<int> &result);
	// compute area of a contour/polygon
  static float Area(const vector<Vec3d> &contour);
  // decide if point Px/Py is inside triangle defined by (Ax,Ay) (Bx,By) (Cx,Cy)
  static bool InsideTriangle(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Px, float Py);

private:
  static bool Snip(const vector<Vec3d> &contour,int u,int v,int w,int n,int *V);
};
