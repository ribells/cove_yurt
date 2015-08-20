 /* =============================================================================
	File: utility.cpp

  =============================================================================== */

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#include <time.h>
#include <direct.h>
#include <io.h>
#include <errno.h>

#define mkdir(x, y)		_mkdir((x))
#define access(x, y)	_access((x), (y))
#define rmdir(x)		_rmdir(x)
#else
#include <sys/time.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#endif
#include <algorithm>
#include <iomanip>
#include "utility.h"


CEnvironment g_Env;


#ifdef UWWF_EXPORTS
void cout << (string str_in) { cout << str_in << endl; }
void mem_alloc_user_message() { }
#endif


/* =============================================================================
 =============================================================================== */
void setDataPath()
{
	g_Env.m_LocalDataPath = g_Env.m_AppPath;
#ifdef WIN32

	// on windows check if we can write to app folder and if not set data location elsewhere
	if (writefile("temp.txt", "cove", 4))
		delfile("temp.txt");
	else
	{
		TCHAR	szPath[MAX_PATH];

		//  CSIDL_COMMON_APPDATA or CSIDL_COMMON_DOCUMENTS
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
		{
			g_Env.m_LocalDataPath = normalizePath(szPath) + "/COVE";
			if (!fileexists(g_Env.m_LocalDataPath) && mkdir(g_Env.m_LocalDataPath.c_str(), 0755) == -1) 
				g_Env.m_LocalDataPath = "";
		}
	}
#endif
}

/* =============================================================================
 =============================================================================== */
Vec3d getVec3d(const float *v)
{
	return Vec3d(v[0], v[1], v[2]);
}

/* =============================================================================
 =============================================================================== */
Vec3f getVec3f(const float *v)
{
	return Vec3f(v[0], v[1], v[2]);
}

/* =============================================================================
 =============================================================================== */
Vec3d getVec3d(Vec3f v)
{
	return Vec3d(v[0], v[1], v[2]);
}

/* =============================================================================
 =============================================================================== */
Vec3f getVec3f(Vec3d v)
{
	return Vec3f(v[0], v[1], v[2]);
}

/* =============================================================================
 =============================================================================== */
int clrR(int clr)
{
	return 0xff & (clr >> 18);
}

/* =============================================================================
 =============================================================================== */
int clrG(int clr)
{
	return 0xff & (clr >> 16);
}

/* =============================================================================
 =============================================================================== */
int clrB(int clr)
{
	return 0xff & clr;
}

/* =============================================================================
 =============================================================================== */
int clrA(int clr)
{
	return 0xff & (clr >> 24);
}

/* =============================================================================
 =============================================================================== */
color clrRGBA(int r, int g, int b, int a)
{
	return r + (g << 8) + (b << 16) + (a << 24);
}

/* =============================================================================
 =============================================================================== */
color clrRGBA(Vec3f v, float fa)
{
	v *= 255.0;

	int a = (fa * 255.0);
	return clrRGBA((int) v[0], (int) v[1], (int) v[2], a);
}

/* =============================================================================
 =============================================================================== */
Vec3f clrVec(unsigned int c)
{
	return Vec3f(clrR(c), clrG(c), clrB(c)) / 255.0;
}

/* =============================================================================
 =============================================================================== */
float clrTrans(unsigned int c)
{
	return clrA(c) / 255.0;
}

/* =============================================================================
 =============================================================================== */
bool fileexists(string strPath)
{
	return(access(strPath.c_str(), 0) == 0);
}

/* =============================================================================
 =============================================================================== */
bool renfile(string strPath, string strNewPath)
{
	return(rename(strPath.c_str(), strNewPath.c_str()) != 0);
}

/* =============================================================================
 =============================================================================== */
bool delfile(string strPath)
{
	return(remove(strPath.c_str()) == 0);
}

/* =============================================================================
 =============================================================================== */
bool deldir(string strPath)
{
	return(rmdir(strPath.c_str()) == 0);
}

/* =============================================================================
 =============================================================================== */
bool changedir(string strPath)
{
#ifdef WIN32
	return(_chdir(strPath.c_str()) == 0);
#else
	return(chdir(strPath.c_str()) == 0);
#endif
}

/* =============================================================================
 =============================================================================== */
bool makedir(string newdir)
{
	char	*buffer;
	char	*p;
	int		len = (int) newdir.size();

	if (len <= 0)
		return false;

	buffer = (char *) malloc(len + 1);
	strcpy(buffer, newdir.c_str());

	if (buffer[len - 1] == '/') 
		buffer[len - 1] = '\0';

	if (mkdir(buffer, 0755) == 0)
	{
		free(buffer);
		return true;
	}

	p = buffer + 1;
	while(1)
	{
		char	hold;

		while(*p && *p != '\\' && *p != '/') p++;
		hold = *p;
		*p = 0;
		if (mkdir(buffer, 0755) == -1)
		{
			printf("could not create directory %s\n", buffer);
			free(buffer);
			return false;
		}

		if (hold == 0)
			break;
		*p++ = hold;
	}

	free(buffer);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool removedir(string dir)
{
	if (!fileexists(dir))
		return true;

	vector<string>	strDir = listfiles(dir, true);
	for (int i = 0; i < strDir.size(); i++)
		if (!removedir(dir + "/" + strDir[i]))
			return false;

	vector<string>	strFiles = listfiles(dir, false);
	for (int i = 0; i < strFiles.size(); i++)
		if (!delfile(dir + "/" + strFiles[i]))
			return false;
	return deldir(dir);
}

/* =============================================================================
 =============================================================================== */
bool findfile(string &FileName, string strSupported[])	//  adds extension of found file
{
	if (getExt(FileName).size()) //  full pathname so just try to load it
	{
		return(fileexists(FileName));
	}
	else if (strSupported)		//  add extensions and search for file
	{
		for (int i = 0; strSupported[i].size(); i++)
		{
			setExt(FileName, strSupported[i]);
			if (fileexists(FileName))
				return true;
		}
	}

	FileName = remExt(FileName);
	return false;
}

/* =============================================================================
 =============================================================================== */
int filesize(string fileName)
{
	FILE	*pFileHdr = NULL;
	if ((pFileHdr = fopen(fileName.c_str(), "rb")) == NULL)
		return -1;
	fseek(pFileHdr, 0, SEEK_END);

	int iLen = ftell(pFileHdr);
	fclose(pFileHdr);
	return iLen;
}

/* =============================================================================
 =============================================================================== */
bool readfile(string fileName, char * &data, int &iLen)
{
	FILE	*pFileHdr = NULL;
	if ((pFileHdr = fopen(fileName.c_str(), "rb")) == NULL)
		return false;
	fseek(pFileHdr, 0, SEEK_END);
	iLen = ftell(pFileHdr);
	fseek(pFileHdr, 0, SEEK_SET);
	if (!mem_alloc(data, iLen + 1))
		return false;
	bool bRet = fread(data, iLen, 1, pFileHdr);
	fclose(pFileHdr);
	data[iLen] = '\0';
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool check_write_access(string fileName)
{
	FILE	*pFileHdr = NULL;
	pFileHdr = fopen(fileName.c_str(), "wb");
	fclose(pFileHdr);
	return pFileHdr != NULL;
}

/* =============================================================================
 =============================================================================== */
bool writefile(string fileName, const char *data, int iLen)
{
	FILE	*pFileHdr = NULL;
	if ((pFileHdr = fopen(fileName.c_str(), "wb")) == NULL) 
		return false;
	if (fwrite(data, 1, iLen, pFileHdr) != iLen) 
		return false;
	fclose(pFileHdr);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool appendfile(string fileName, const char *data, int iLen)
{
	FILE	*pFileHdr = NULL;
	if ((pFileHdr = fopen(fileName.c_str(), "ab")) == NULL) 
		return false;
	if (fwrite(data, 1, iLen, pFileHdr) != iLen) 
		return false;
	fclose(pFileHdr);
	return true;
}

/* =============================================================================
 =============================================================================== */
vector<string> listfiles(string strPath, bool bFolders)
{
	vector<string>	fList;
#ifdef WIN32
	WIN32_FIND_DATA ffd;
	HANDLE			hFind;
	strPath += "/*.*";
	hFind = FindFirstFile(strPath.c_str(), &ffd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!bFolders && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				fList.push_back(ffd.cFileName);
			else if ( bFolders
					&&	(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					&&	string(ffd.cFileName) != "."
					&&	string(ffd.cFileName) != "..")
				fList.push_back(ffd.cFileName);
		} while(FindNextFile(hFind, &ffd) != 0);
		FindClose(hFind);
	}

#else
	struct dirent	*de = NULL;
	DIR				*d = NULL;
	d = opendir(strPath.c_str());
	if (d != NULL)
	{
		while(de = readdir(d))
		{
			bool	bIsDir = opendir((strPath + "/" + de->d_name).c_str());
			if (!bFolders && !bIsDir)
				fList.push_back(de->d_name);
			else if (bFolders && bIsDir && string(de->d_name) != "." && string(de->d_name) != "..")
				fList.push_back(de->d_name);
		}

		closedir(d);
	}
#endif
	sort(fList.begin(), fList.end());
	return fList;
}

/* =============================================================================
 =============================================================================== */
dtime filetime(string strPath)
{
	if (!fileexists(strPath)) 
		return NO_TIME;

	dtime	tm = NO_TIME;
#ifdef WIN32

	//  Retrieve the file times for the file.
	WIN32_FIND_DATA ffd;
	HANDLE			hFile = FindFirstFile(strPath.c_str(), &ffd);
	if (hFile != INVALID_HANDLE_VALUE) 
		return false;

	FILETIME	ftCreate, ftAccess, ftWrite;
	SYSTEMTIME	stUTC;
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
		return FALSE;

	//  Convert the last-write time to local time.
	FileTimeToSystemTime(&ftWrite, &stUTC);
	tm = getDateTime(stUTC.wYear, stUTC.wMonth, stUTC.wDay, stUTC.wHour, stUTC.wMinute, stUTC.wSecond);
#else
	struct stat stat_info;
	stat(strPath.c_str(), &stat_info);
	tm = stat_info.st_mtime;
#endif
	return tm;
}

/* =============================================================================
 =============================================================================== */
string trimstr(string str)
{
	size_t	iStart = str.find_first_not_of(" \t");
	if (iStart == string::npos) 
		return "";

	size_t	iEnd = str.find_last_not_of(" \t");
	return str.substr(iStart, iEnd - iStart + 1);
}

/* =============================================================================
 =============================================================================== */
string cleanstr(string str)
{
	size_t	iPos;
	while((iPos = str.find('%')) != string::npos)
	{
		int		ch = 0;
		char	buf[3], buf2[2] = " ";
		strcpy(buf, str.substr(iPos + 1, 2).c_str());
		sscanf(buf, "%x", &ch);
		buf2[0] = (char) ch;
		str = str.substr(0, iPos) + (string) buf2 + str.substr(iPos + 3);
	}

	while((iPos = str.find('+')) != string::npos) str[iPos] = ' ';
	return str;
}

/* =============================================================================
 =============================================================================== */
string normalizePath(string str)
{
	size_t	iPos;
	while((iPos = str.find('\\')) != string::npos) str = str.substr(0, iPos) + '/' + str.substr(iPos + 1);
	return str;
}

/* =============================================================================
 =============================================================================== */
string dosPath(string str)
{
	size_t	iPos;
	while((iPos = str.find('/')) != string::npos) str = str.substr(0, iPos) + '\\' + str.substr(iPos + 1);
	return str;
}

/* =============================================================================
 =============================================================================== */
string getFileName(string fileName)
{
	fileName = normalizePath(fileName);

	int iStart = fileName.rfind('/');
	iStart = (iStart == string::npos) ? 0 : iStart + 1;
	return fileName.substr(iStart);
}

/* =============================================================================
 =============================================================================== */
string getFilePath(string fileName) //  with final slash
{
	string	tempstr = normalizePath(fileName);
	int		iPos = tempstr.rfind('/');
	return(iPos == string::npos) ? "" : fileName.substr(0, iPos + 1);
}

/* =============================================================================
 =============================================================================== */
string getNewName(string strName, int i)
{
	ostringstream	s;
	s.str("");
	s << strName << i;
	return s.str();
}

/* =============================================================================
 =============================================================================== */
string getNewFileName(string fileName, int i)
{
	string	strExt = getExt(fileName);
	fileName = remExt(fileName);

	ostringstream	s;
	s << fileName << "_" << i << strExt;
	return s.str();
}

/* =============================================================================
 =============================================================================== */
string getFileNameBase(string fileName)
{
	if (!fileName.size())
		return "";
	fileName = getFileName(fileName);
	fileName = remExt(fileName);
	return fileName;
}

/* =============================================================================
 =============================================================================== */
string getExt(string fileName)
{
	int iExt = fileName.rfind('.');
	int iFolderExt = fileName.rfind('/');
	return iExt == string::npos || iExt < iFolderExt ? "" : lwrstr(fileName.substr(iExt));
}

/* =============================================================================
 =============================================================================== */
string remExt(string fileName)
{
	int iExt = fileName.rfind('.');
	int iFolderExt = fileName.rfind('/');
	return iExt == string::npos || iExt < iFolderExt ? fileName : fileName.substr(0, iExt);
}

/* =============================================================================
 =============================================================================== */
bool checkExt(string fileName, string ext)
{
	return ext == getExt(fileName);
}

/* =============================================================================
 =============================================================================== */
void setExt(string &fileName, string ext)
{
	if (checkExt(fileName, ext))
		return;
	fileName = remExt(fileName);
	fileName += ext;
}

/* =============================================================================
 =============================================================================== */
string getDataServerPath(string strPath)
{
	if (strPath.substr(0, 7) == "datasvr" && g_Env.m_COVEServerPath != "")
		strPath = g_Env.m_COVEServerPath + "/" + strPath.substr(8);
	return strPath;
}

/* =============================================================================
 =============================================================================== */
string getLocalCachePath(string strPath)
{
	if (strPath.substr(0, 7) == "datasvr" && g_Env.m_COVEServerPath != "")
		strPath = g_Env.m_LocalCachePath + "/" + strPath.substr(8);
	return strPath;
}

bool getIsLocalFile(string strFile)
{
	return (strFile.substr(0, 5) != "http:" && strFile.substr(0, 7) != "datasvr" && strFile.substr(0, 5) != "wfsvr");
}

/* =============================================================================
 =============================================================================== */
string getValidFile(string strPath)
{
	if (!fileexists(getLocalCachePath(strPath)))
		return strPath;

	string	strExt = getExt(strPath);
	string	strBase = remExt(strPath);
	for (int i = 0; i < 10000; i++)	//  arbitrary limit to halt runaway
	{
		strPath = getNewName(strBase, i) + strExt;
		if (!fileexists(getLocalCachePath(strPath)))
			return strPath;
	}

	return "";
}

/* =============================================================================
 =============================================================================== */
string getRandFile(string strBase, string strExt)
{
	string	strLink;
	do
	{
		int				iVal = rand();
		ostringstream	s;
		s << strBase << setfill('0') << setw(5) << iVal << strExt;
		strLink = s.str();
	} while(fileexists(getLocalCachePath(strLink)));
	return strLink;
}

/* =============================================================================
 =============================================================================== */
string getTempFile(string strExt)
{
	string	strPath = g_Env.m_LocalCachePath + "/temp/";
	mkdir(strPath.c_str(), 0755);
	return getRandFile(strPath, strExt);
}

/* =============================================================================
 =============================================================================== */
char *trimstr(char *token)
{
	token += strspn(token, " \t\"");

	char	*pch = token + strlen(token) - 1;
	for (; pch >= token; pch--)
		if (*pch == ' ' || *pch == '\t' || *pch == '\r')
			*pch = '\0';
		else
			break;
	if (pch < token)
		*token = '\0';
	else if (*pch == '\"')
		*pch = '\0';
	return token;
}

/* =============================================================================
 =============================================================================== */
string lwrstr(string str)
{
	for (int i = 0; i < str.size(); 
		str[i] = tolower(str[i]), i++);
	return str;
}

/* =============================================================================
 =============================================================================== */
bool getCSVTokens(char *buf, vector<vector<string> > &csvTokens)
{
	vector<string>	row;

	//  load in the data
	bool			bEndTable = false;
	bool			bEndLine = false;
	bool			bEscaped = false;
	char			*token = buf;
	for (; *buf && !bEndTable; buf++)
	{
		switch(*buf)
		{
		case '\n':
			bEndLine = true;
			if (bEscaped)	//  hit end of line while escaped
				return false;

		case '\t':
		case ',':
			if (bEscaped)
				continue;
			*buf = '\0';
			token = trimstr(token);
			row.push_back(token);
			token = buf + 1;

			if (bEndLine)
			{
				if (row.size() == 0)
					bEndTable = true;
				else
					csvTokens.push_back(row);
				row.clear();
				bEndLine = false;
			}
			break;

		case '"':
			bEscaped = !bEscaped;
			break;
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
vector<string> Tokenize(const string &str, const string &delimiters)
{
	vector<string>		tokens;
	string::size_type	delimPos = 0, tokenPos = 0, pos = 0;

	if (str.length() < 1)
		return tokens;
	while(1)
	{
		delimPos = str.find_first_of(delimiters, pos);
		tokenPos = str.find_first_not_of(delimiters, pos);

		if (string::npos != delimPos)
		{
			if (string::npos != tokenPos)
			{
				if (tokenPos < delimPos)
				{
					tokens.push_back(str.substr(pos, delimPos - pos));
				}
				else
				{
					tokens.push_back("");
				}
			}
			else
			{
				tokens.push_back("");
			}

			pos = delimPos + 1;
		}
		else
		{
			if (string::npos != tokenPos)
			{
				tokens.push_back(str.substr(pos));
			}
			else
			{
				tokens.push_back("");
			}
			break;
		}
	}

	return tokens;
}

/* =============================================================================
 =============================================================================== */
void mem_alloc_failure(const char *pchFile, int iLine, int iSize)
{
	if (pchFile)
	{
		ostringstream	s;
		s << "Memory allocation failure!: " << pchFile << ", line " << iLine << " (size = " << iSize << ")";
		cout << s.str();
	}

	cout << "Memory allocation failure in utility.cpp";
}

/* =============================================================================
 =============================================================================== */
unsigned char *makeRGBA(const unsigned char *pData, int w, int h)
{
	unsigned char	*pNew = NULL;
	if (!mem_alloc(pNew, w * h * 4)) 
		return NULL;

	unsigned char	*p = pNew;
	for (int i = 0; i < w * h; *p++ = 0xff, i++)
		for (int j = 0; j < 3; j++) 
			*p++ = *pData++;
	return pNew;
}

/* =============================================================================
 =============================================================================== */
unsigned char *makeRGB(const unsigned char *pData, int w, int h)
{
	unsigned char	*pNew = NULL;
	if (!mem_alloc(pNew, w * h * 3))
		return NULL;

	unsigned char	*p = pNew;
	for (int i = 0; i < w * h; pData++, i++)
		for (int j = 0; j < 3; j++) 
			*p++ = *pData++;
	return pNew;
}

/* =============================================================================
 =============================================================================== */
unsigned char *makeBGR(const unsigned char *pData, int w, int h)
{
	unsigned char	*pNew = NULL;
	if (!mem_alloc(pNew, w * h * 3))
		return NULL;

	unsigned char	*p = pNew;
	for (int y = 0; y < h; y++)
	{
		const unsigned char *pRow = pData + (h - y - 1) * w * 4;
		for (int x = 0; x < w; x++, p += 3, pRow++)
		{
			p[2] = *pRow++;
			p[1] = *pRow++;
			p[0] = *pRow++;
		}
	}

	return pNew;
}

/* =============================================================================
 =============================================================================== */
void initTime()
{
#ifdef WIN32
	_putenv_s("TZ", "GMT0");
	_tzset();
#else
	setenv("TZ", "GMT0", 1);
	tzset();
#endif
}

/* =============================================================================
 =============================================================================== */
dtime scanDateTime(string strTime, int iFormat)
{
	if (atof(strTime.c_str()) > 5000)
		return scanDecimalTime(strTime);

	int ivals[6] = { 0 };
	int icnt = 0;
	for (int ipos = 0; ipos < strTime.size() && icnt < 6; ipos++)
	{
		int startPos = strTime.find_first_of("0123456789", ipos);
		if (startPos == string::npos)
			break;
		ipos = strTime.find_first_not_of("0123456789", startPos);
		if (ipos == string::npos)
			ipos = strTime.size();

		string	strToken = strTime.substr(startPos, ipos - startPos);
		ivals[icnt++] = atoi(strToken.c_str());
	}

	struct tm	tmval = { 0 };
	int	*tm_list_MDY[6] =
	{
		&(tmval.tm_mon),&(tmval.tm_mday),&(tmval.tm_year),&(tmval.tm_hour),&(tmval.tm_min),&(tmval.tm_sec)
	};
	int	*tm_list_DMY[6] =
	{
		&(tmval.tm_mday),&(tmval.tm_mon),&(tmval.tm_year),&(tmval.tm_hour),&(tmval.tm_min),&(tmval.tm_sec)
	};
	int	*tm_list_YMD[6] =
	{
		&(tmval.tm_year),&(tmval.tm_mon),&(tmval.tm_mday),&(tmval.tm_hour),&(tmval.tm_min),&(tmval.tm_sec)
	};

	int	**tm_list = tm_list_MDY;
	if (ivals[0] > 1900 || strTime.find('Z') != string::npos)
		tm_list = tm_list_YMD;
	else if ((iFormat == TIME_FMT_DEFAULT && g_Env.m_TimeFormat == TIME_FMT_DMY) || iFormat == TIME_FMT_DMY)
		tm_list = tm_list_DMY;

	for (int i = 0; i < icnt; i++) 
		*(tm_list[i]) = ivals[i];
	if (tmval.tm_year > 1900)
		tmval.tm_year -= 1900;
	else if (tmval.tm_year < 50)
		tmval.tm_year += 100;
	tmval.tm_mon--;
	return mktime(&tmval);
}

/* =============================================================================
 =============================================================================== */
dtime scanDecimalTime(string strTime)
{
	int iYear = -1, iDay = -1;
	if (strTime.size() == 14)	// sosus time is always 14 characters
	{
		iDay = atoi(strTime.substr(4, 3).c_str());
		iYear = atoi(strTime.substr(0, 4).c_str());
	}

	if (iYear < 1950 || iYear > 2050 || iDay > 366) 
		return atof(strTime.c_str());	// return number of seconds

	// this looks like sosustime
	int cnt[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int lcnt[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int iMonth = 0;
	for (iMonth = 0; iMonth < 11; iMonth++)
	{
		int len = (iYear & 0x3) ? cnt[iMonth] : lcnt[iMonth];
		if (iDay < len)
			break;
		iDay -= len;
	}

	struct tm	tmNew = { 0 };
	tmNew.tm_mon = iMonth;
	tmNew.tm_mday = iDay;
	tmNew.tm_year = iYear - 1900;

	string	newTime = strTime.substr(7, 2) + ":" + strTime.substr(9, 2) + ":" + strTime.substr(11, 2);
	sscanf(newTime.c_str(), "%d:%d:%d", &(tmNew.tm_hour), &(tmNew.tm_min), &(tmNew.tm_sec));
	return mktime(&tmNew);
}

/* =============================================================================
 =============================================================================== */
dtime getDateTime(int year, int month, int day, int hour, int min, int sec)
{
	struct tm	tmNew = { 0 };
	tmNew.tm_year = year;
	tmNew.tm_mon = month;
	tmNew.tm_mday = day;
	tmNew.tm_hour = hour;
	tmNew.tm_min = min;
	tmNew.tm_sec = sec;
	if (tmNew.tm_year > 1900)
		tmNew.tm_year -= 1900;
	else if (tmNew.tm_year < 50)
		tmNew.tm_year += 100;
	tmNew.tm_mon--;
	return mktime(&tmNew);
}

/* =============================================================================
 =============================================================================== */
string getNiceDateTimeString(dtime dClock, dtime dRange)
{
	if (dClock < 0.0)
		return "";

	time_t		tClock = (time_t) dClock;
	char		buf[64];
	struct tm	*pTime = gmtime(&tClock);
	if (g_Env.m_TimeFormat == TIME_FMT_DMY)
	{
		sprintf
		(
			buf,
			"%i/%i/%i %i:%.02i:%.02i",
			pTime->tm_mday,
			pTime->tm_mon + 1,
			pTime->tm_year + 1900,
			pTime->tm_hour,
			pTime->tm_min,
			pTime->tm_sec
		);
	}
	else
	{
		sprintf
		(
			buf,
			"%i/%i/%i %i:%.02i:%.02i",
			pTime->tm_mon + 1,
			pTime->tm_mday,
			pTime->tm_year + 1900,
			pTime->tm_hour,
			pTime->tm_min,
			pTime->tm_sec
		);
	}

	//  for prettying up time
	if (dRange > DUR_MONTH)
		*strchr(buf, ' ') = '\0';
	else if (dRange > DUR_DAY)
		strcpy(strchr(buf, ':'), ":00");
	else if (dRange > DUR_HOUR)
		*strrchr(buf, ':') = '\0';

	return buf;
}

/* =============================================================================
 =============================================================================== */
string getDateString(dtime dClock)
{
	return getNiceDateTimeString(dClock, DUR_MONTH + 1);
}

/* =============================================================================
 =============================================================================== */
string getGMTDateTimeString(dtime dClock)
{
	if (dClock < 0.0)
		return "";

	time_t		tClock = (time_t) dClock;
	char		buf[64];
	struct tm	*pTime = gmtime(&tClock);
	sprintf
	(
		buf,
		"%i-%i-%iT%i:%.02i:%.02iZ",
		pTime->tm_year + 1900,
		pTime->tm_mon + 1,
		pTime->tm_mday,
		pTime->tm_hour,
		pTime->tm_min,
		pTime->tm_sec
	);
	return buf;
}

/* =============================================================================
 =============================================================================== */
dtime getCurrentTime()
{
	time_t	t;
	//time(&t);  BDC - CRASHING ON 64-bit (0.0 value returned for now)
	//return t;
	return 0.0;
}

/* =============================================================================
 =============================================================================== */
string davesCommas(double f, bool bDecimal)
{
	char	buf[10];
	string	strOut;
	double	factor = 1;
	for (; (f / factor) > 1000.; factor *= 1000);
	for (int i = 0; factor >= 1.; i++)
	{
		double	val = floor(f / factor);
		sprintf(buf, i == 0 ? "%.0lf" : ",%03.0lf", val);
		f = fmodf(f, factor);
		factor /= 1000;
		strOut += buf;
	}

	if (bDecimal)
	{
		sprintf(buf, "%.2lf", f);
		strOut += (buf + 1);
	}

	return strOut;
}

/* =============================================================================
 =============================================================================== */
double getKMfromGPS(Vec3d pnt0, Vec3d pnt1)
{

	// pretty good approximate distance equation from geocoder.us site
	return sqrt(sqr((pnt0[1] - pnt1[1]) * cos(((pnt0[0] + pnt1[0]) / 2) * vl_pi / 180)) + sqr(pnt0[0] - pnt1[0])) * KmPerDegree;
}

/* =============================================================================
 =============================================================================== */
double getGPSTriangleArea(Vec3d a, Vec3d b, Vec3d c)
{
	double	da = getKMfromGPS(a, b);
	double	db = getKMfromGPS(a, c);
	double	dc = getKMfromGPS(b, c);
	double	s = 0.5 * (da + db + dc);

	//  herons forumla from mathworld
	return sqrt(s * (s - da) * (s - db) * (s - dc));
}

/* =============================================================================
 =============================================================================== */
bool getUTMfromLL(double lat, double lon, double &zx, double &zy, double &E, double &N)
{
	if (lat < -80 || lat > 84)
		return false;

	int x = (lon + 180) / 6;
	int y = min((lat + 80) / 8, 19.0);
	zx = x + 1;
	zy = y < 6 ? y + 'C' : y < 11 ? (y - 6) + 'J' : (y - 11) + 'P';

	double	a = 6378137, b = 6356752.3; //  WGS84 major & minor semi-axes
	double	k0 = 0.9996012717;			//  NatGrid scale factor on central meridian
	lat *= vl_pi / 180;
	lon *= vl_pi / 180;

	double	lat0 = 0, lon0 = (-180 + x * 6 + 3) * vl_pi / 180;	//  UTM true origin
	double	N0 = lat < 0 ? -10000000 : 0, E0 = 500000;			//  northing & easting of true origin, metres

	double	e2 = 1 - (b * b) / (a * a); //  eccentricity squared
	double	n = (a - b) / (a + b), n2 = n * n, n3 = n * n * n;

	double	cosLat = cos(lat), sinLat = sin(lat);
	double	nu = a * k0 / sqrt(1 - e2 * sinLat * sinLat);	//  transverse radius of curvature
	double	rho = a * k0 * (1 - e2) / pow(1 - e2 * sinLat * sinLat, 1.5);	//  meridional radius of curvature
	double	eta2 = nu / rho - 1;

	double	Ma = (1 + n + (5 / 4) * n2 + (5 / 4) * n3) * (lat - lat0);
	double	Mb = (3 * n + 3 * n * n + (21 / 8) * n3) * sin(lat - lat0) * cos(lat + lat0);
	double	Mc = ((15 / 8) * n2 + (15 / 8) * n3) * sin(2 * (lat - lat0)) * cos(2 * (lat + lat0));
	double	Md = (35 / 24) * n3 * sin(3 * (lat - lat0)) * cos(3 * (lat + lat0));
	double	M = b * k0 * (Ma - Mb + Mc - Md);	//  meridional arc

	double	cos3lat = cosLat * cosLat * cosLat;
	double	cos5lat = cos3lat * cosLat * cosLat;
	double	tan2lat = tan(lat) * tan(lat);
	double	tan4lat = tan2lat * tan2lat;

	double	I = M + N0;
	double	II = (nu / 2) * sinLat * cosLat;
	double	III = (nu / 24) * sinLat * cos3lat * (5 - tan2lat + 9 * eta2);
	double	IIIA = (nu / 720) * sinLat * cos5lat * (61 - 58 * tan2lat + tan4lat);
	double	IV = nu * cosLat;
	double	V = (nu / 6) * cos3lat * (nu / rho - tan2lat);
	double	VI = (nu / 120) * cos5lat * (5 - 18 * tan2lat + tan4lat + 14 * eta2 - 58 * tan2lat * eta2);

	double	dLon = lon - lon0;
	double	dLon2 = dLon * dLon, dLon3 = dLon2 * dLon, dLon4 = dLon3 * dLon, dLon5 = dLon4 * dLon, dLon6 = dLon5 * dLon;

	N = I + II * dLon2 + III * dLon4 + IIIA * dLon6;
	E = E0 + IV * dLon + V * dLon3 + VI * dLon5;
	return true;
}

/* =============================================================================
 =============================================================================== */
bool getLLfromUTM(double zx, double zy, double E, double N, double &lat, double &lon)
{
	int x = zx - 1;
	int y = zy - 'C';
	y = zy > 'H' ? y - 1 : zy > 'N' ? y - 2 : y;

	double	a = 6378137, b = 6356752.3; //  WGS84 major & minor semi-axes
	double	F0 = 0.9996012717;			//  NatGrid scale factor on central meridian
	double	lat0 = 0, lon0 = (-180 + x * 6 + 3) * vl_pi / 180;	//  UTM true origin
	double	N0 = y < 10 ? -10000000 : 0, E0 = 500000;			//  northing & easting of true origin, metres

	double	e2 = 1 - (b * b) / (a * a);		//  eccentricity squared
	double	n = (a - b) / (a + b), n2 = n * n, n3 = n * n * n;

	lat = lat0;

	double	M = 0;
	do
	{
		lat = (N - N0 - M) / (a * F0) + lat;

		double	Ma = (1 + n + (5 / 4) * n2 + (5 / 4) * n3) * (lat - lat0);
		double	Mb = (3 * n + 3 * n * n + (21 / 8) * n3) * sin(lat - lat0) * cos(lat + lat0);
		double	Mc = ((15 / 8) * n2 + (15 / 8) * n3) * sin(2 * (lat - lat0)) * cos(2 * (lat + lat0));
		double	Md = (35 / 24) * n3 * sin(3 * (lat - lat0)) * cos(3 * (lat + lat0));
		M = b * F0 * (Ma - Mb + Mc - Md);	//  meridional arc
	} while(N - N0 - M >= 0.00001);

	// ie until < 0.01mm

	double	cosLat = cos(lat), sinLat = sin(lat);
	double	nu = a * F0 / sqrt(1 - e2 * sinLat * sinLat);	//  transverse radius of curvature
	double	rho = a * F0 * (1 - e2) / pow(1 - e2 * sinLat * sinLat, 1.5);	//  meridional radius of curvature
	double	eta2 = nu / rho - 1;

	double	tanLat = tan(lat);
	double	tan2lat = tanLat * tanLat, tan4lat = tan2lat * tan2lat, tan6lat = tan4lat * tan2lat;
	double	secLat = 1 / cosLat;
	double	nu3 = nu * nu * nu, nu5 = nu3 * nu * nu, nu7 = nu5 * nu * nu;
	double	VII = tanLat / (2 * rho * nu);
	double	VIII = tanLat / (24 * rho * nu3) * (5 + 3 * tan2lat + eta2 - 9 * tan2lat * eta2);
	double	IX = tanLat / (720 * rho * nu5) * (61 + 90 * tan2lat + 45 * tan4lat);
	double	X = secLat / nu;
	double	XI = secLat / (6 * nu3) * (nu / rho + 2 * tan2lat);
	double	XII = secLat / (120 * nu5) * (5 + 28 * tan2lat + 24 * tan4lat);
	double	XIIA = secLat / (5040 * nu7) * (61 + 662 * tan2lat + 1320 * tan4lat + 720 * tan6lat);

	double	dE = (E - E0), dE2 = dE *
		dE, dE3 = dE2 *
		dE, dE4 = dE2 *
		dE2, dE5 = dE3 *
		dE2, dE6 = dE4 *
		dE2, dE7 = dE5 *
		dE2;
	lat = lat - VII * dE2 + VIII * dE4 - IX * dE6;
	lon = lon0 + X * dE - XI * dE3 + XII * dE5 - XIIA * dE7;

	lat *= 180 / vl_pi;
	lon *= 180 / vl_pi;
	return true;
}

/* =============================================================================
 =============================================================================== */
string getDegMinSecString(double pos, bool bSec, bool bHTML, bool bPretty)
{
	char	cDeg = bHTML ? ' ' : 0xba;
	double	fDeg = fabs(pos) + 1e-8;
	double	fMin = fmod(fDeg, 1.) * 60.;
	double	fSec = fmod(fMin, 1.) * 60.;
	char	pch[64];
	if (bPretty)
	{
		if (fabs(fSec) < 0.05)
		{
			if (fabs(fMin) < 0.05)
				sprintf(pch, "%.0lf%c", floor(fDeg), cDeg);
			else
				sprintf(pch, "%.0lf%c %2.0lf'", floor(fDeg), cDeg, floor(fMin));
		}
		else
			sprintf(pch, "%.0lf%c %2.2lf'", floor(fDeg), cDeg, fMin);
	}
	else if (!bSec)
		sprintf(pch, "%.0lf%c %2.5lf'", floor(fDeg), cDeg, fMin);
	else
		sprintf(pch, "%.0lf%c %2.0lf' %2.2lf''", floor(fDeg), cDeg, floor(fMin), fSec);
	return pch;
}

/* =============================================================================
    dist_Point_to_Segment(): get the distance of a point to a segment. ;
    Input: a Point P and a Segment S (in any dimension) ;
    Return: the shortest distance from P to S
 =============================================================================== */
Vec3f closest_Point_on_Line(Vec3f Pnt, Vec3f P0, Vec3f P1, bool bSeg)
{
	Vec3f	v = P1 - P0;
	Vec3f	w = Pnt - P0;
	double	c1 = dot(w, v);
	if (bSeg && c1 <= 0) 
		return P0;

	double	c2 = dot(v, v);
	if (bSeg && c2 <= c1) 
		return P1;

	double	b = c1 / c2;
	Vec3f	Pb = P0 + b * v;
	return Pb;
}

/* =============================================================================
 =============================================================================== */
float dist_Point_to_Line(Vec3f Pnt, Vec3f P0, Vec3f P1, bool bSeg)
{
	Vec3f	P2 = closest_Point_on_Line(Pnt, P0, P1, bSeg);
	return len(Pnt - P2);
}

/* =============================================================================
    smooth interpolating curve
 =============================================================================== */
void CatmullRomPnt(Vec3f &pnt, Vec3f &tgt, double t, Vec3f p[4])
{
	double	t2 = t * t;
	double	t3 = t2 * t;
	pnt = 0.5 * ((2.0 * p[1]) 
		+ t * (-p[0] + p[2]) 
		+ t2 * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3]) 
		+ t3 * (-p[0] + 3.0 * p[1] - 3.0 * p[2] + p[3]));

	// tangent is (p+1 - p-1) * 0.5
	tgt = norm(0.5 * norm(p[2] - p[0]) * (1.0 - t) + 0.5 * norm(p[3] - p[1]) * t);
}

/* =============================================================================
 =============================================================================== */
void CatmullRomValue(float &val, double t, float p[4])
{
	double	t2 = t * t;
	double	t3 = t2 * t;
	val = 0.5 * ((2.0 * p[1]) 
		+ t * (-p[0] + p[2]) 
		+ t2 * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3]) 
		+ t3 * (-p[0] + 3.0 * p[1] - 3.0 * p[2] + p[3]));
}

/* =============================================================================
 =============================================================================== */
bool getIntel()
{
	char	val[sizeof(int)];
	*((int *) val) = 1;
	return(val[0] == 1);
}

/* =============================================================================
 =============================================================================== */
bool swapEndian(bool bIntelData, void *pData, int iSize, int iCnt)
{

	// intel data is little endian, need to swap if on big endian machine (PPC)
	if (bIntelData == getIntel())
		return false;

	unsigned char	*ps = (unsigned char *) pData;
	for (int i = 0; i < iCnt; i++, ps += iSize)
		for (int j = 0; j < iSize / 2; j++)
		{
			unsigned char	tmp = *(ps + j);
			*(ps + j) = *(ps + iSize - j - 1);
			*(ps + iSize - j - 1) = tmp;
		}

	return true;
}

/* =============================================================================
 =============================================================================== */
string encode_base64(string src)
{
	if (src.size() > 512)
		return "";

	static char base_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char		src_buf[1024] = "";
	char		dst_buf[1024] = "";
	strcpy(src_buf, src.c_str());

	int		src_length = src.size();
	int		line_length = 0;
	char	*src_ptr = src_buf;
	char	*dst_ptr = dst_buf;
	while(src_length > 2)
	{
		*dst_ptr = base_64[(int) (*src_ptr) >> 2];
		*(dst_ptr + 1) = base_64[((((int) (*src_ptr) & 0x3)) << 4) | (((int) (*(src_ptr + 1)) & 0xF0) >> 4)];
		*(dst_ptr + 2) = base_64[((((int) (*(src_ptr + 1))) & 0xF) << 2) | ((((int) (*(src_ptr + 2))) & 0xC0) >> 6)];
		*(dst_ptr + 3) = base_64[((int) (*(src_ptr + 2))) & 0x3F];
		src_ptr += 3;
		src_length -= 3;
		line_length += 4;
		if (line_length > 71)
		{
			line_length = 0;
			*(dst_ptr + 4) = '\r';
			*(dst_ptr + 5) = '\n';
			dst_ptr += 6;
		}
		else
		{
			dst_ptr += 4;
		}
	}

	if (src_length == 2)
	{
		*dst_ptr = base_64[(int) (*src_ptr) >> 2];
		*(dst_ptr + 1) = base_64[((((int) (*src_ptr) & 0x3)) << 4) | (((int) (*(src_ptr + 1)) & 0xF0) >> 4)];
		*(dst_ptr + 2) = base_64[((((int) (*(src_ptr + 1))) & 0xF) << 2) | ((((int) (*(src_ptr + 2))) & 0xC0) >> 6)];
		*(dst_ptr + 3) = '=';
		dst_ptr += 4;
		line_length = 0;
	}
	else if (src_length == 1)
	{
		*dst_ptr = base_64[(int) (*src_ptr) >> 2];
		*(dst_ptr + 1) = base_64[((((int) (*src_ptr) & 0x3)) << 4) | (((int) (*(src_ptr + 1)) & 0xF0) >> 4)];
		*(dst_ptr + 2) = '=';
		*(dst_ptr + 3) = '=';
		dst_ptr += 4;
		line_length = 0;
	}

	*dst_ptr = '\0';

	return(string) dst_buf;
}

/* =============================================================================
 =============================================================================== */
string obscureString(string str)
{
	char	buf[MAX_PATH];
	string	strOut = "";
	for (int i = 0; i < str.size(); i++)
	{
		sprintf(buf, "%2x", (int) (str[i] ^ 129));
		strOut += buf;
	}

	return strOut;
}

/* =============================================================================
 =============================================================================== */
string unobscureString(string str)
{
	char	buf[] = "xx";
	string	strOut = "";
	if (str.size() % 2 == 1) 
		return strOut;

	int iVal;
	for (int i = 0; i < str.size() / 2; i++)
	{
		buf[0] = str[i * 2];
		buf[1] = str[i * 2 + 1];
		sscanf(buf, "%x", &iVal);
		strOut.push_back((char) (iVal ^ 129));
	}

	return strOut;
}


// ** THIS IS A CODE SNIPPET WHICH WILL EFFICIEINTLY TRIANGULATE ANY
// ** POLYGON/CONTOUR (without holes) AS A STATIC CLASS.
// ** SUBMITTED BY JOHN W. RATCLIFF (jratcliff@verant.com) July 22, 2000
static const float EPSILON=0.0000000001f;

float Triangulate::Area(const vector<Vec3d> &contour)
{
  int n = contour.size();

  float A=0.0f;

  for(int p=n-1,q=0; q<n; p=q++)
  {
    A+= contour[p][0]*contour[q][1] - contour[q][0]*contour[p][1];
  }
  return A*0.5f;
}

// InsideTriangle decides if a point P is Inside of the triangle defined by A, B, C.
bool Triangulate::InsideTriangle(float Ax, float Ay,
                      float Bx, float By,
                      float Cx, float Cy,
                      float Px, float Py)

{
  float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  float cCROSSap, bCROSScp, aCROSSbp;

  ax = Cx - Bx;  ay = Cy - By;
  bx = Ax - Cx;  by = Ay - Cy;
  cx = Bx - Ax;  cy = By - Ay;
  apx= Px - Ax;  apy= Py - Ay;
  bpx= Px - Bx;  bpy= Py - By;
  cpx= Px - Cx;  cpy= Py - Cy;

  aCROSSbp = ax*bpy - ay*bpx;
  cCROSSap = cx*apy - cy*apx;
  bCROSScp = bx*cpy - by*cpx;

  return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};

bool Triangulate::Snip(const vector<Vec3d> &contour,int u,int v,int w,int n,int *V)
{
  int p;
  float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

  Ax = contour[V[u]][0];
  Ay = contour[V[u]][1];

  Bx = contour[V[v]][0];
  By = contour[V[v]][1];

  Cx = contour[V[w]][0];
  Cy = contour[V[w]][1];

  if ( EPSILON > (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) )
	  return false;

  for (p=0;p<n;p++)
  {
    if( (p == u) || (p == v) || (p == w) ) 
		continue;
    Px = contour[V[p]][0];
    Py = contour[V[p]][1];
    if (InsideTriangle(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) 
		return false;
  }

  return true;
}

bool Triangulate::Process(const vector<Vec3d> &contour, vector<int> &result)
{
  // allocate and initialize list of Vertices in polygon

  int n = contour.size();
  if ( n < 3 ) 
	  return false;

  vector<int> V(n);

  // we want a counter-clockwise polygon in V

  if ( 0.0f < Area(contour) )
    for (int v=0; v<n; v++) 
		V[v] = v;
  else
    for(int v=0; v<n; v++) 
		V[v] = (n-1)-v;

  int nv = n;

  //  remove nv-2 Vertices, creating 1 triangle every time
  int count = 2*nv;   // error detection

  for(int m=0, v=nv-1; nv>2; )
  {
    // if we loop, it is probably a non-simple polygon
    if (0 >= (count--))
    {
      // Triangulate: ERROR - probable bad polygon!
      return false;
    }

    // three consecutive vertices in current polygon, <u,v,w>
    int u = v  ; if (nv <= u) u = 0;	// previous
    v = u+1; if (nv <= v) v = 0;		// new v
    int w = v+1; if (nv <= w) w = 0;	// next

    if ( Snip(contour,u,v,w,nv,&V[0]) )
    {
      int a,b,c,s,t;

      // true names of the vertices
      a = V[u]; b = V[v]; c = V[w];

      // output Triangle
      result.push_back( a );
      result.push_back( b );
      result.push_back( c );

	  cout << a << "," << b << "," << c << endl;

      m++;

      // remove v from remaining polygon
      for(s=v,t=v+1;t<nv;s++,t++) V[s] = V[t]; nv--;

      // resest error detection counter
      count = 2*nv;
    }
  }

  return true;
}


void main_test(int argc,char **argv)
{

  // Small test application demonstrating the usage of the triangulate
  // class.


  // Create a pretty complicated little contour by pushing them onto
  // an stl vector.

  vector<Vec3d> a;

  a.push_back( Vec3d(0,6,0));
  a.push_back( Vec3d(0,0,0));
  a.push_back( Vec3d(3,0,0));
  a.push_back( Vec3d(4,1,0));
  a.push_back( Vec3d(6,1,0));
  a.push_back( Vec3d(8,0,0));
  a.push_back( Vec3d(12,0,0));
  a.push_back( Vec3d(13,2,0));
  a.push_back( Vec3d(8,2,0));
  a.push_back( Vec3d(8,4,0));
  a.push_back( Vec3d(11,4,0));
  a.push_back( Vec3d(11,6,0));
  a.push_back( Vec3d(6,6,0));
  a.push_back( Vec3d(4,3,0));
  a.push_back( Vec3d(2,6,0));

  // allocate an STL vector to hold the answer.

  vector<int> result;

  //  Invoke the triangulator to triangulate this polygon.
  Triangulate::Process(a,result);

  // print out the results.
  int tcount = result.size()/3;

  for (int i=0; i<tcount; i++)
  {
    const Vec3d &p1 = a[result[i*3+0]];
    const Vec3d &p2 = a[result[i*3+1]];
    const Vec3d &p3 = a[result[i*3+2]];
    printf("Triangle %d => (%0.0f,%0.0f) (%0.0f,%0.0f) (%0.0f,%0.0f)\n",i+1,p1[0],p1[1],p2[0],p2[1],p3[0],p3[1]);
  }
}

