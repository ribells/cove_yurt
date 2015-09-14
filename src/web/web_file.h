#pragma once

#include <string>
#include <sstream>
#include <vector>
using namespace std;

enum {THREAD_TRY, THREAD_WAIT, THREAD_FAIL, THREAD_SUCCEED, THREAD_DONE};

namespace web_func
{
	struct WebFileStruct
	{
		int *pThreadStatus;
		string WebPath;
		string LocalPath;
		string Auth;
		string PostData;
		bool bAsynch;
	};

	static bool m_RecacheWebData = false;

	string getLocalPath(string strFile);
	string cacheWebFile(string strFile);
	string cacheWebFiles(string strLink, string strFolder, string strSupported[]);
	bool getWebFile(string strWebPath, string strLocalPath);
	bool getWebFile(string strWebPath, string strLocalPath, string strAuth, int *pStatus);

	bool uploadCOVEServerFile(string strWebPath, string strUploadPath);
	bool removeCOVEServerFile(string strWebPath);

	bool checkNewDownload(bool bSet = false);

	void runWebServer(bool bStart);
	void runUDPServer(bool bStart);

	string addRequest(string strFileName, string * pFileData, string strDataOut, string strVisScript, string strDataPath, string strWFPath, bool bRerunWF);
	bool eraseRequest(string strID);
	bool delRequest(string strID);
	bool setRequestPerf(string strID);
	bool getNextRequest(string & strID);
	bool getRequestData(string strID, string &strCOVFile, string &strDataOut, string &strVisScript, string &strDataPath, string &strWFPath, bool &bRerunWF);
	bool finishRequest(string strID);
	bool setRequestDataFile(string strID, string strDataOut);

	bool handleVisualizationRequest();

	bool WMS_GetCapabilities(string strLink, string strPath);
	bool WMS_GetImageFormat(string strPath, string &strImageType);
	bool WMS_GetMapNames(string strPath, vector<string> & strTitles, vector<string> & strNames);
};

using namespace web_func;

