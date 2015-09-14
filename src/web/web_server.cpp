 /* =============================================================================
	File: web_server.cpp

  =============================================================================== */

#include <errno.h>

#ifdef WIN32
#include <process.h>
#include <direct.h>
#include <io.h>
#else
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "web_http.h"
#include "web_file.h"
#include "utility.h"
#include "timer.h"

#include "web_server.h"

#include "xmlParser.h"

#ifdef UWWF_EXPORTS
void decompressFiles(string strFiles, string strOutputFolder){ }
#endif

#define BUFFER_SIZE 4096

static CServerQueue g_ServerQueue;

/* =============================================================================
 =============================================================================== */

string web_func::addRequest
(
	string strFileName,
	string *pFileData,
	string strData,
	string strVisScript,
	string strDataPath,
	string strWFPath,
	bool bRerunWF
)
{
	return g_ServerQueue.addRequest(strFileName, pFileData, strData, strVisScript, strDataPath, strWFPath, bRerunWF);
}

/* =============================================================================
 =============================================================================== */
bool web_func::eraseRequest(string strID)
{
	return g_ServerQueue.eraseRequest(strID);
}

/* =============================================================================
 =============================================================================== */
bool web_func::getNextRequest(string &strID)
{
	return g_ServerQueue.getNextRequest(strID);
}

/* =============================================================================
 =============================================================================== */
bool web_func::getRequestData
(
	string strID,
	string &strFile,
	string &strData,
	string &strVisScript,
	string &strDataPath,
	string &strWFPath,
	bool &bRerunWF
)
{
	return g_ServerQueue.getRequestData(strID, strFile, strData, strVisScript, strDataPath, strWFPath, bRerunWF);
}

/* =============================================================================
 =============================================================================== */
bool web_func::finishRequest(string strID)
{
	return g_ServerQueue.finishRequest(strID);
}

/* =============================================================================
 =============================================================================== */
bool web_func::setRequestDataFile(string strID, string strFile)
{
	return g_ServerQueue.setDataFile(strID, strFile);
}

/* =============================================================================
 =============================================================================== */
bool web_func::delRequest(string strID)
{
	return g_ServerQueue.delRequest(strID);
}

/* =============================================================================
 =============================================================================== */
bool web_func::setRequestPerf(string strID)
{
	return g_ServerQueue.setRequestPerf(strID);
}

/* =============================================================================
 =============================================================================== */
inline int findString(string str, vector<string> &strList)
{
	for (int i = 0; i < strList.size(); i++)
		if (str == strList[i]) 
			return i;
	return -1;
}

#ifdef WIN32
void web_server (void *pData)
#else
void	*
web_server (void *pData)
#endif
{
		//  set up request types
	enum RequestType { REQ_ADD, REQ_STATUS, REQ_PERF, REQ_OUTPUT, REQ_KILL, REQ_RESET, REQ_NONE };
	string tags[] = { "AddTask", "GetStatus", "GetPerfOutput", "GetTaskOutput", "KillTask", "ResetSystem", "" };
	vector<string> tagIDs;
	for (int i = 0; tags[i] != ""; i++) 
		tagIDs.push_back(tags[i]);

	//  set up http class
	CHttpDownload	*pDownloader = new CHttpDownload();
	if (!pDownloader->Create())
		goto webserver_end;

	//  bind to port 11223 and start listening
	if (!pDownloader->Bind(11223) || !pDownloader->Listen())
		goto webserver_end;

	//  loop waiting for people to send strings
	cout << ("Web server started on port 11223.");
	while(g_ServerQueue.getRunServer())
	{
		CWinTcpSocket	*pSocket = new CWinTcpSocket;

		//  block on accept
		if (pDownloader->Accept(pSocket))
		{
			pDownloader->setAcceptSocket(pSocket);
			vector<string> strTags, strVals;
			if (!pDownloader->ParseArgs(pSocket, strTags, strVals))
			{
				pDownloader->responseFail(pSocket, "COVE failed to handle request");
				continue;
			}
			bool bSuccess = false;

			int iSubmit = findString("Submit", strTags);
			if (iSubmit == -1)
			{
				cout << ("Invalid Multipart form received.");
				pDownloader->responseFail(pSocket, "COVE failed to handle request");
				continue;
			}
			int iTask = findString(strVals[iSubmit], tagIDs);
			RequestType iType = iTask == -1 ? REQ_STATUS : (RequestType) iTask;

			bSuccess = true;
			if (iType == REQ_NONE)
			{
				bSuccess = false;
			}
			else if (iType == REQ_RESET) //  reset the system
			{
				g_ServerQueue.resetRequestList();
				pDownloader->responseSuccess(pSocket, "Server Successfully reset");
			}
			else if (iType == REQ_ADD)	//  reading in new task file
			{
				int iTag = findString("InputName", strTags);
				if (iTag == -1) 
					pDownloader->responseSuccess(pSocket, "Could not find File in add request.");
				string strFileName;
				string *pFileData = NULL;
				if (getExt(strVals[iTag]) == ".cov")
					strFileName = strVals[iTag];
				else
					pFileData = &strVals[iTag];
				string strVisScript;
				string strDataPath;
				string strWFPath;
				bool bRerunWF = false;
				if ((iTag = findString("VisScript", strTags)) != -1) 
					strVisScript = strVals[iTag];
				if ((iTag = findString("DataPath", strTags)) != -1) 
					strDataPath = strVals[iTag];
				if ((iTag = findString("WFPath", strTags)) != -1) 
					strWFPath = strVals[iTag];
				if ((iTag = findString("RerunWF", strTags)) != -1) 
					bRerunWF = strVals[iTag] == "true";

				string strID = g_ServerQueue.addRequest
					(strFileName, pFileData, "", strVisScript, strDataPath, strWFPath, bRerunWF);
				if (strID == "")
					pDownloader->responseSuccess(pSocket, "Failed to add task to queue");
				else
					pDownloader->responseSuccess(pSocket, "Task added to COVE queue - ID: " + strID);
			}
			else	//  kill id or get output id
			{
				int iRec = -1;
				string strID = "";
				int iID = findString("TaskID", strTags);
				if (iID != -1)
				{
					strID = strVals[iID];
					iRec = g_ServerQueue.findRequest(strID);
				}
				if (iRec == -1)
				{
					pDownloader->responseSuccess(pSocket, "Task ID not found: " + strID);
				}
				else if (iType == REQ_STATUS)
				{
					pDownloader->responseSuccess(pSocket, g_ServerQueue.getStatus(strID));
				}
				else if (iType == REQ_PERF)
				{
					pDownloader->responseSuccess(pSocket, g_ServerQueue.getPerf(strID));
				}
				else if (iType == REQ_OUTPUT)
				{
					if (iRec > g_ServerQueue.getCurrentTask())
						pDownloader->responseSuccess(pSocket, "Task in queue: " + strID);
					else if (!g_ServerQueue.getCompleted(iRec))
						pDownloader->responseSuccess(pSocket, "Task currently being processed: " + strID);
					else
					{
						string strFile = g_ServerQueue.getDataFile(iRec);
						if (strFile == "")
							pDownloader->responseSuccess(pSocket, "No output files for Task: " + strID);
						else if (iType == REQ_OUTPUT)
						{
							string strMimeType =
							getExt(strFile)
							== ".jpg" ? "image/jpeg" :
							getExt(strFile)
							== ".mp4" ? "movie/mp4" :
							getExt(strFile)
							== ".zip" ? "application/zip" : "application/cove";
							pDownloader->responseFile(pSocket, strFile, strMimeType);
						}
						else
						{
							string strMimeType = "text/xml";
							strFile =
							remExt(strFile)
							+ "_perf.xml";
							pDownloader->responseFile(pSocket, strFile, strMimeType);
						}
					}
				}
				else if (iType == REQ_KILL)
				{
					if (!g_ServerQueue.delRequest(iRec))
						pDownloader->responseSuccess(pSocket, "COVE unable to kill task: " + strID);
					else if (g_Env.m_SystemInterrupt)
						pDownloader->responseSuccess(pSocket, "Task deletion initiated: " + strID);
					else
						pDownloader->responseSuccess(pSocket, "Task deletion successful: " + strID);
				}
				if (!bSuccess)
					pDownloader->responseFail(pSocket, "COVE failed to handle request");
			}
		}
		delete pSocket;
	}
	pDownloader->Close();
	cout << ("Web server closed on port 11223.");

webserver_end:
	delete pDownloader;
#ifdef WIN32
	_endthread();
#else
	pthread_exit((void *) 0);
	return (void *) 0;
#endif
}

/* =============================================================================
 =============================================================================== */
void web_func::runWebServer(bool bStart)
{
	if (!g_Env.m_Internet) 
		return;
	if (!bStart)
	{
		if (g_ServerQueue.getRunServer())
		{
			g_ServerQueue.setRunServer(false);

			string tmp = cacheWebFile("http://localhost:11223/temp.temp");
		}

		return;
	}

	//  check if already running
	if (g_ServerQueue.getRunServer()) 
		return;

	//  start if up
	g_ServerQueue.setRunServer(true);
#ifdef WIN32
	_beginthread(web_server, 0, NULL);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, web_server, (void *) 0);
#endif
}

#ifdef ALVIN

//  UDP Server

vector<string>	*pvRecords = NULL;

/* =============================================================================
 =============================================================================== */

void addUDPRecord(string strRec)
{
	pvRecords->push_back(strRec);
}

/* =============================================================================
 =============================================================================== */
vector<string> *getUDPRecords()
{
	vector<string>	*vTmp = pvRecords;
	pvRecords = new vector<string>;
	return vTmp;
}

/* =============================================================================
 =============================================================================== */
int getUDPRecordCnt()
{
	return pvRecords == NULL ? 0 : (int) pvRecords->size();
}

#ifdef WIN32
void udp_server (void *pData)
#else
void	*
udp_server (void *pData)
#endif
{
	double dTimeLim = 0;
	string strText;
	getUDPRecords();	//  start the records queueu

	//  set up http class
	CHttpDownload	*pDownloader = new CHttpDownload();
	if (!pDownloader->Create_UDP())
	{
		delete pDownloader;
		goto webserver_end;
	}

	//  bind to port 11223 and start listening
	if (!pDownloader->Bind_UDP(g_UDPPort))
	{
		delete pDownloader;
		goto webserver_end;
	}

	//  loop waiting for people to send strings
	cout << ("UDP Server Started.");
	while(g_Run_UDP_Server)
	{
		char buf[BUFFER_SIZE];

		int iLen = pDownloader->RecvFrom(buf, BUFFER_SIZE);
		if (iLen < 0 || iLen >= BUFFER_SIZE)
			continue;
		buf[iLen] = '\0';

		cout << buf;

		// if (strstr(buf, "PWHLBX") == NULL) //LBL navigation on ship
		if (strstr(buf, "PWHDOP") == NULL)	//  Doppler navigation on sub
			continue;

		double dTime = getCurrentTime();
		if (dTime < dTimeLim)
			continue;
		dTimeLim = dTime + 5;

		string strRec =
		getNiceDateTimeString(dTime)
		+ (string) "," + (string) buf;
		addUDPRecord(strRec);
	}
	pDownloader->Close();
	delete pDownloader;
	cout << ("UDP Server Closed.");

webserver_end:
#ifdef WIN32
	_endthread();
#else
	pthread_exit((void *) 0);
	return (void *) 0;
#endif
}

/* =============================================================================
 =============================================================================== */
void runUDPServer(bool bStart)
{
	if (!g_Env.m_Internet)
		return;
	if (!bStart)
	{
		g_Run_UDP_Server = false;
		return;
	}

	//  check if already running
	if (g_Run_UDP_Server)
		return;

	//  start if up
	g_Run_UDP_Server = true;
#ifdef WIN32
	_beginthread(udp_server, 0, NULL);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, udp_server, (void *) 0);
#endif
}
#endif
