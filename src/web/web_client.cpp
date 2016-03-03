 /* =============================================================================
	File: web_client.cpp

  =============================================================================== */

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "web_http.h"
#include "web_file.h"
#include "timer.h"
#include "utility.h"
#include "xmlParser.h"
#include "zip/compress.h"

#ifdef UWWF_EXPORTS
void decompressFiles(string strFiles, string strOutputFolder){ }
#endif

//  Functions 
void		download_progress_end();

/* =============================================================================
//  get current value and then set new value  
//   (no arg defaults to false and checks and clears the var)
 =============================================================================== */
bool web_func::checkNewDownload(bool bSet)
{
	static bool	bNewDownloadAvailable = false;

	bool	bRet = bNewDownloadAvailable;
	bNewDownloadAvailable = bSet;
	return bRet;
}

#ifdef WIN32
void webclient (void *pData)
#else
void	*
webclient (void *pData)
#endif
{
	string			WebPath = ((WebFileStruct *) pData)->WebPath;
	string			LocalPath = ((WebFileStruct *) pData)->LocalPath;
	string			strAuth = ((WebFileStruct *) pData)->Auth;
	string			strPost = ((WebFileStruct *) pData)->PostData;
	int				*pStatus = ((WebFileStruct *) pData)->pThreadStatus;

	int				iStatus = THREAD_FAIL;

	CHttpDownload	*pDownloader = NULL;
	string			strServer, strURI;
	int				iPort;
	if (parseURL(WebPath, strServer, strURI, &iPort))
	{
		if (g_Env.m_Internet)
		{
			if ((pDownloader = new CHttpDownload()) != NULL)
			{
				if (pDownloader->Create())
				{
					if (pDownloader->Connect(strServer.c_str(), iPort))
					{
						if (strPost.size())
						{
							if (pDownloader->PostData(strServer, strURI, LocalPath, strPost) != 0)
								cout << ("Unable to upload data to " + strURI);
							else
								iStatus = THREAD_SUCCEED;
						}
						else
						{

							// save to tmp file and then rename to avoid load of half downloaded file
							string	strDest = LocalPath;
							strDest += ".tmp";

							//  download file
							int iRet = pDownloader->DownloadFile(strServer, strURI, strDest, strAuth);
							if (iRet != HTTPERR_NONE) 
								setExt(LocalPath, ".html");	//  save html error message

							//  rename tmp file to final filename
							if (fileexists(LocalPath)) 
								delfile(LocalPath);
							if (renfile(strDest, LocalPath) != 0)
							{
								cout << ("Unable to rename downloaded file to " + LocalPath);
								delfile(strDest);
							}
							else if (iRet == HTTPERR_NONE)
								iStatus = THREAD_SUCCEED;
						}
					}
				}
				pDownloader->Close();
			}
			delete pDownloader;
		}
	}

	checkNewDownload(true);

	if (iStatus != THREAD_SUCCEED && !((WebFileStruct *) pData)->bAsynch)
	{
		if (((WebFileStruct *) pData)->PostData.size())
			cout << ("POST request failed. Report in " + LocalPath);
		else if (iPort != 11223)
			cout << ("GET request failed.  Report in " + LocalPath);
	}
	bool	bAsynch = ((WebFileStruct *) pData)->bAsynch;
	delete (WebFileStruct *) pData;
	*pStatus = iStatus;
	if (bAsynch)
	{
#ifdef WIN32
		_endthread();
	}
#else
	pthread_exit((void *) 0);
}
	return (void *) 0;
#endif
}

/* =============================================================================
 =============================================================================== */
bool execWebFileRequest(string WebPath, string LocalPath, string strPost, string strAuth, int *pStatus)
{
	int				iStatus;

	//  package up arguments
	WebFileStruct	*pData = new WebFileStruct;
	pData->pThreadStatus = pStatus ? pStatus : &iStatus;
	pData->bAsynch = pStatus ? true : false;
	*(pData->pThreadStatus) = THREAD_WAIT;
	pData->WebPath = WebPath;
	pData->LocalPath = LocalPath;
	pData->PostData = strPost;
	pData->Auth = strAuth;

	//  send off to thread
	if (!pData->bAsynch)
	{
		webclient(pData);
		return(iStatus == THREAD_SUCCEED);
	}
	else
	{
#ifdef WIN32
		_beginthread(webclient, 0, pData);
#else
		pthread_t	thread;
		pthread_create(&thread, NULL, webclient, (void *) pData);
#endif
	}

	//  if we have a status flag then return and do this asynchronously
	if (pStatus) 
		return true;

	assert(false);

	//  if no status flag then need to wait to return
	while(iStatus == THREAD_WAIT)
#ifdef WIN32
		Sleep(1000L);
#else
	usleep(1000);
#endif
	return(iStatus == THREAD_SUCCEED);

}

/* =============================================================================
 =============================================================================== */
bool web_func::uploadCOVEServerFile(string strWebPath, string strUploadPath)
{
	if (strWebPath.substr(0, 7) != "datasvr")
		return false;
	strWebPath = strWebPath.substr(8);

	string	strPost;
	strPost += getPostParmHeader("webloc", strWebPath);
	strPost += getPostFileHeader("file", strUploadPath);

	string	strURI = g_Env.m_COVEServerPath + "/cgi-bin/upload_file.py";
	bool	bRet = execWebFileRequest(strURI, "", strPost, "", NULL);
	cout << ("Upload of " + strWebPath + (bRet ? " successful." : " failed."));
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool web_func::removeCOVEServerFile(string strWebPath)
{
	if (strWebPath.substr(0, 7) != "datasvr")
		return false;
	strWebPath = strWebPath.substr(8);

	string	strPost;
	strPost += getPostParmHeader("webloc", strWebPath);

	string	strURI = g_Env.m_COVEServerPath + "/cgi-bin/remove_file.py";
	bool	bRet = execWebFileRequest(strURI, "", strPost, "", NULL);
	cout << ("Delete of " + strWebPath + (bRet ? " successful." : " failed."));
	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool web_func::getWebFile(string strWebPath, string strPath, string strAuth, int *pStatus)
{
	string	strLocalPath = strPath;
	string	strURI = strWebPath;

	if (findfile(strLocalPath, NULL))
		return true;

	if (!makedir(getFilePath(strLocalPath)))
	{
		if (pStatus)
			*pStatus = THREAD_FAIL;
		//cout << "From web_client.cpp: Unable to create local file " + strLocalPath;
		return false;
	}

	//  fix up calls to the data server
	strURI = getDataServerPath(strWebPath);

	return execWebFileRequest(strURI, strLocalPath, "", strAuth, pStatus);
}

bool web_func::getWebFile(string strWebPath, string strLocalPath) { return getWebFile(strWebPath, strLocalPath, "", NULL); }

/* =============================================================================
 =============================================================================== */
string web_func::getLocalPath(string strFile)
{
	if (getIsLocalFile(strFile))
		return strFile;

	string	strLocal;
	if (strFile.substr(0, 7) == "datasvr")
		strLocal = g_Env.m_LocalCachePath + "/" + strFile.substr(8);
	else if (strFile.substr(0, 5) == "wfsvr")
		strLocal = g_Env.m_LocalCachePath + "/workflows/" + strFile.substr(6);
	else
		strLocal = getTempFile(getExt(strFile));	//  get local filename
	makedir(getFilePath(strLocal));
	return strLocal;
}

/* =============================================================================
 =============================================================================== */
string web_func::cacheWebFile(string strFile)
{
	string	strLocal = getLocalPath(strFile);
	if (strLocal != strFile)
		if (!getWebFile(strFile, strLocal))
			strLocal = "";
	return strLocal;
}

/* =============================================================================
 =============================================================================== */
string web_func::cacheWebFiles(string strLink, string strCacheFolder, string strSupported[])
{
	string	strNewPath;
	if (strLink == "")
		return "";

	string	fname = getFileName(strLink);
	if (getExt(strLink) != "" && getExt(strLink) != ".zip")
	{
		strNewPath = strLink;
		if (fileexists(strNewPath))	//  see if the file exists based on the path
			return strNewPath;
		strNewPath = g_Env.m_AppPath + "/" + strLink;
		if (fileexists(strNewPath))	//  see if it's relative to the run location
			return strNewPath;
		strNewPath = g_Env.m_CurFilePath + getFileName(strLink);
		if (fileexists(strNewPath))	//  see if it's with the cov file
			return strNewPath;
		strNewPath = g_Env.m_LocalCachePath + "/" + strCacheFolder + "/" + fname;
		if (fileexists(strNewPath))	//  see if it's already in the cache
			return strNewPath;
		cout << "  - downloading File: " + strLink + "\n";
		if (!getWebFile(strLink, strNewPath))
			return "";
	}
	else
	{
		strNewPath = g_Env.m_LocalCachePath + "/" + strCacheFolder + "/" + remExt(fname);

		//  if no extension download package from server and decompress
		if (!findfile(strNewPath, strSupported))
		{
			strNewPath += ".zip";
			if (getExt(strLink) != ".zip") 
				strLink += ".zip";
			cout << "  - downloading File: " + strLink + "\n";
			if (!getWebFile(strLink, strNewPath)) 
				return "";
			decompressFiles(strNewPath, strCacheFolder);
			//delfile(strNewPath);

			//download_progress_end();

			strNewPath = remExt(strNewPath);
			if (!findfile(strNewPath, strSupported)) 
				return "";
		}
	}

	return strNewPath;
}

/* =============================================================================
 =============================================================================== */
bool web_func::WMS_GetCapabilities(string strLink, string strPath)
{

	//  getCapabilities from server
	string	strRequest = strLink + "?VERSION=1.1.1&SERVICE=WMS&REQUEST=GetCapabilities";
	return getWebFile(strRequest, strPath);
}

/* =============================================================================
 =============================================================================== */
bool web_func::WMS_GetImageFormat(string strPath, string &strImageType)
{

	//  find format to use
	XMLNode		xn0, xn1, xn2, xn3, xn4;
	const char	*pchName = NULL;

	//  get format
	if (!fileexists(strPath)) 
		return false;
	strImageType = "";

	XMLResults	pResults;
	xn0 = XMLNode::parseFile(strPath.c_str(), "WMT_MS_Capabilities", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		cout << ("Unable to read the WMS capabilities file.");
		return false;
	}

	xn1 = xn0.getChildNode("Capability");
	xn2 = xn1.getChildNode("Request");
	xn3 = xn2.getChildNode("GetMap");

	int fCnt = xn3.nChildNode("Format");
	for (int i = 0; i < fCnt; i++)
	{
		xn4 = xn3.getChildNode("Format", i);
		if ((pchName = xn4.getText()) == NULL)
			continue;

		string	strFormat = pchName;
		if ((strFormat.find("jpeg") != string::npos) || (strFormat.find("png") != string::npos))
		{
			strImageType = pchName;
			return true;
		}
	}

	return false;
}

/* =============================================================================
 =============================================================================== */
bool web_func::WMS_GetMapNames(string strPath, vector<string> &strTitles, vector<string> &strNames)
{

	//  find format to use
	XMLNode		xn0, xn1, xn2, xn3, xn4, xn5, xn6;
	const char	*pchFolder;
	const char	*pchName;
	const char	*pchTitle;
	const char	*pchStyle;

	//  get format
	if (!fileexists(strPath))
		return false;
	strTitles.clear();
	strNames.clear();

	XMLResults	pResults;
	xn0 = XMLNode::parseFile(strPath.c_str(), "WMT_MS_Capabilities", &pResults);
	if (pResults.error != eXMLErrorNone)
	{
		cout << ("Unable to read the WMS capabilities file.");
		return false;
	}

	xn1 = xn0.getChildNode("Capability");
	xn2 = xn1.getChildNode("Layer");

	int nCnt = xn2.nChildNode("Layer");
	for (int i = 0; i < nCnt; i++)
	{
		xn3 = xn2.getChildNode("Layer", i);

		int		fCnt = xn3.nChildNode("Layer");
		int		ifCnt = (fCnt == 0) ? 1 : fCnt;

		//  get folder name if it exists
		string	strFolder;
		if (fCnt)
		{
			xn4 = xn3.getChildNode("Title");
			if (!xn4.isEmpty() && (pchFolder = xn4.getText()) != NULL)
				strFolder = (string) pchFolder + ".";
		}

		//  get all the maps
		for (int j = 0; j < ifCnt; j++)
		{
			xn4 = fCnt == 0 ? xn3 : xn3.getChildNode("Layer", j);
			xn5 = xn4.getChildNode("Name");
			if (xn5.isEmpty() || (pchName = xn5.getText()) == NULL)
				continue;

			//  get the title if it exists
			xn5 = xn4.getChildNode("Title");
			if (!xn5.isEmpty()) 
				pchTitle = xn5.getText();

			//  go through each style
			int sCnt = xn4.nChildNode("Style");
			if (sCnt == 0)
			{
				strNames.push_back(pchName);
				strTitles.push_back(pchTitle);
			}
			else
			{
				for (int k = 0; k < sCnt; k++)
				{
					string	strName = pchName;
					string	strTitle = strFolder + (pchTitle ? pchTitle : pchName);
					if (sCnt > 1)
					{
						xn5 = xn4.getChildNode("Style", k);
						xn6 = xn5.getChildNode("Name");
						if (!xn6.isEmpty() && (pchStyle = xn6.getText()) != NULL)
						{
							strName += "&STYLES=" + (string) pchStyle;
							strTitle += "(" + (string) pchStyle + ")";
						}
					}

					strNames.push_back(strName);
					strTitles.push_back(strTitle);
				}
			}
		}
	}

	return true;
}
