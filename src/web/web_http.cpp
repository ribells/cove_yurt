 /* =============================================================================
	File: web_http.cpp

  =============================================================================== */

#include "web_http.h"
#include "utility.h"

string HTTP_GET_REQUEST = "GET %s HTTP/1.1\r\n\
Host: %s\r\n\r\n";

string HTTP_GET_REQUEST_AUTH = "GET %s HTTP/1.1\r\n\
Host: %s\r\n\
Authorization: Basic %s\r\n\r\n";

string HTTP_POST_REQUEST = "POST %s HTTP/1.1\r\n\
Host: %s\r\n\
Accept: */*\r\n\
Referer: COVE\r\n\
User-Agent: Mozilla/4.0\r\n\
Content-type: multipart/form-data; boundary=-----7d772c20582\r\n\
Content-length: %i\r\n\r\n";

string HTTP_POST_PARAMETER = "-------7d772c20582\r\n\
Content-Disposition: form-data; name=\"%s\"\r\n\r\n\
%s\r\n";

string HTTP_POST_FILE = "-------7d772c20582\r\n\
Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n\
Content-Type: application/octet-stream\r\n\
Content-Transfer-Encoding: binary\r\n\r\n";

string HTTP_POST_FOOTER = "\r\n-------7d772c20582--\r\n\r\n";

string HTTP_RESPONSE_SUCCESS = "HTTP/1.1 200 OK\r\n\
Date: Thu, 07 Feb 2008 23:38:51 GMT\r\n\
Server: COVE/1.0 \r\n\
Content-Length: %i\r\n\
Connection: close\r\n\
Content-Type: text/html;�charset=UTF-8\r\n\r\n\
%s";

string HTTP_RESPONSE_SUCCESS_FILE = "HTTP/1.1 200 OK\r\n\
Date: Thu, 07 Feb 2008 23:38:51 GMT\r\n\
Server: COVE/1.0 \r\n\
Content-Length: %i\r\n\
Connection: close\r\n\
Content-Type: %s\r\n\r\n";

string HTTP_RESPONSE_FAIL = "HTTP/1.1 400 Bad Request\r\n\
Date: Thu, 07 Feb 2008 23:38:51 GMT\r\n\
Server: COVE/1.0 \r\n\
Content-Length: %i\r\n\
Connection: close\r\n\
Content-Type: text/html;�charset=UTF-8\r\n\r\n\
%s";

const int	RECV_BUFFER_SIZE = 16384;

/* =============================================================================
 =============================================================================== */

bool parseURL(string WebPath, string &strServer, string &strURI, int *iPort)
{
	if (WebPath == "") 
		return false;

	//  parse out server, port, and filename from webpath
	int iPos = WebPath.find("://") + 3;
	int iEnd;
	for (iEnd = iPos; iEnd < (int) WebPath.size(); iEnd++)
		if (WebPath[iEnd] == ':' || WebPath[iEnd] == '/')
			break;
	strServer = WebPath.substr(iPos, iEnd - iPos);

	*iPort = (WebPath[iEnd] == ':') ? atoi(WebPath.substr(iEnd + 1).c_str()) : 80;

	if (WebPath[iEnd] == ':')
		iEnd = WebPath.find('/', iEnd);

	strURI = WebPath.substr(iEnd);
	return true;
}

/* =============================================================================
 =============================================================================== */
string getPostFileHeader(string parm, string filename)
{
	char	*pchPost = new char[HTTP_POST_FILE.size() + parm.size() + filename.size() + 128];
	sprintf(pchPost, HTTP_POST_FILE.c_str(), parm.c_str(), filename.c_str());

	string	strPost = pchPost;
	delete pchPost;
	return strPost;
}

/* =============================================================================
 =============================================================================== */
string getPostParmHeader(string parm, string val)
{
	char	*pchPost = new char[HTTP_POST_PARAMETER.size() + parm.size() + val.size() + 128];
	sprintf(pchPost, HTTP_POST_PARAMETER.c_str(), parm.c_str(), val.c_str());

	string	strPost = pchPost;
	delete pchPost;
	return strPost;
}

/* =============================================================================
 =============================================================================== */
CHttpDownload::CHttpDownload(void) :
	CWinTcpSocket()
{
	m_DownloadSize = 0;
	m_DownloadCur = 0;
	m_AcceptSocket = NULL;
}

/* =============================================================================
 =============================================================================== */
void CHttpDownload::responseSuccess(CWinTcpSocket *pSocket, string str)
{
	char	buf[1024];
	sprintf(buf, HTTP_RESPONSE_SUCCESS.c_str(), str.size(), str.c_str());
	pSocket->Send(buf, strlen(buf), 0);
}

/* =============================================================================
 =============================================================================== */
void CHttpDownload::responseFail(CWinTcpSocket *pSocket, string str)
{
	char	buf[1024];
	sprintf(buf, HTTP_RESPONSE_FAIL.c_str(), str.size(), str.c_str());
	pSocket->Send(buf, strlen(buf), 0);
}

/* =============================================================================
 =============================================================================== */
void CHttpDownload::responseFile(CWinTcpSocket *pSocket, string strFile, string strType)
{
	char	*pData = NULL;
	int		iDataLen = 0;
	if (strFile == "" || !readfile(strFile, pData, iDataLen))
	{
		cout << ("Unable to return file " + getFileName(strFile));
		return responseSuccess(pSocket, "Unable to return file " + getFileName(strFile));
	}

	char	buf[1024];
	sprintf(buf, HTTP_RESPONSE_SUCCESS_FILE.c_str(), iDataLen, strType.c_str());

	int		iHdrLen = strlen(buf);
	int		iTotLen = iHdrLen + iDataLen;
	char	*pSendData = new char[iTotLen];

	memcpy(pSendData, buf, iHdrLen);
	memcpy(pSendData + iHdrLen, pData, iDataLen);

	pSocket->Send(pSendData, iTotLen, 0);
	delete pSendData;
}

/* =============================================================================
 =============================================================================== */
bool CHttpDownload::ParseArgs(CWinTcpSocket *pSocket, vector<string> &strTags, vector<string> &strVals)
{
	if (ParseHeader() != 0)
		return false;

	string	strMessage;
	if (WriteToString(strMessage) != 0)
		return false;

	if (m_Boundary.size() == 0)
	{
		size_t	iStart = 0;
		while(strMessage.find('=', iStart) != string::npos)
		{
			size_t	iPos = strMessage.find('=', iStart);
			size_t	iEnd = strMessage.find('&', iPos);
			if (iEnd == string::npos)
				iEnd = strMessage.size();
			strTags.push_back(trimstr(strMessage.substr(iStart, iPos - iStart)));
			strVals.push_back(cleanstr(trimstr(strMessage.substr(iPos + 1, iEnd - iPos - 1))));
			iStart = iEnd + 1;
		}
	}
	else
	{
		size_t	iStart;

		if ((iStart = strMessage.find(m_Boundary)) == string::npos)
			return true;
		iStart += m_Boundary.size();

		while(strMessage.find(m_Boundary, iStart) != string::npos)
		{
			size_t	iPos0, iPos1;
			if ((iPos0 = strMessage.find("name=", iStart)) == string::npos)
				return false;
			iPos0 += 6;
			if ((iPos1 = strMessage.find('\"', iPos0)) == string::npos)
				return false;
			strTags.push_back(strMessage.substr(iPos0, iPos1 - iPos0));

			if ((iPos0 = strMessage.find("\r\n\r\n", iPos1)) == string::npos)
				return false;
			iPos0 += 4;
			iPos1 = strMessage.find(m_Boundary, iPos0);
			strVals.push_back(strMessage.substr(iPos0, iPos1 - iPos0 - 4));
			iStart = iPos1 + m_Boundary.size();
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
int CHttpDownload::DownloadFile(string strServer, string strURI, string strLocalFile, string strAuth, int iCnt)
{
	char	*pMemBuf;

	if (iCnt == 5)
	{
		cout << "Exceeded limit on redirects.  Download failed." << endl;
		return HTTPERR_REQUEST;
	}

	if (!GetRequest(strServer.c_str(), strURI.c_str(), strAuth.c_str()))
		return HTTPERR_REQUEST;

	pMemBuf = new char[RECV_BUFFER_SIZE];

	if (pMemBuf == 0)
		return HTTPERR_MEMORY;
	if (GetResponse(pMemBuf) != 0)
		return HTTPERR_RESPONSE;

	if (strstr(pMemBuf, "Location:"))
	{
		string	strServer;
		string	strURI;
		int		iPort;
		string	WebPath = strstr(pMemBuf, "Location:") + 10;
		if (!parseURL(WebPath, strServer, strURI, &iPort))
			return HTTPERR_NOTOK;

		Close();
		Create();
		if (!Connect(strServer.c_str(), iPort))
			return HTTPERR_NOTOK;

		int nErr = DownloadFile(strServer, strURI, strLocalFile, strAuth, iCnt + 1);
		if (nErr != HTTPERR_NONE)
			return nErr;
	}

	// if ( !IsResponseOK( pMemBuf ) )
	//		return HTTPERR_NOTOK;

	delete[] pMemBuf;

	if (ParseHeader() != 0)
		return HTTPERR_HEADERSKIP;

	if (strLocalFile.size()) 
		return WriteToFile(strLocalFile.c_str());

	return HTTPERR_NONE;
}

/* =============================================================================
 =============================================================================== */
int CHttpDownload::PostData(string strServer, string strURI, string strLocalFile, string strData)
{
	char	*pMemBuf;

	char	*pSendData;
	int		iDataSize = strData.size();
	int		iHeaderSize = iDataSize;
	int		iFooterSize = HTTP_POST_FOOTER.size();
	int		iPos = (int) strData.find("filename=\"");
	if (iPos != string::npos)
	{
		iPos += 10;

		int		iEnd = (int) strData.find('\"', iPos);
		string	fileName = iEnd == string::npos ? strData : strData.substr(iPos, iEnd - iPos);

		FILE	*pFile = NULL;
		string	strFiller = "No File";
		long	iFileLen = strFiller.size();
		if (fileName != "")
		{
			if ((pFile = fopen(fileName.c_str(), "rb")) == NULL)
				return HTTPERR_REQUEST;
			fseek(pFile, 0, SEEK_END);
			iFileLen = ftell(pFile);
			fseek(pFile, 0, SEEK_SET);
		}

		iDataSize = iHeaderSize + iFileLen + iFooterSize;
		pSendData = new char[iDataSize];

		memcpy(pSendData, strData.c_str(), iHeaderSize);
		if (pFile)
			fread(pSendData + iHeaderSize, iFileLen, 1, pFile);
		else
			memcpy(pSendData + iHeaderSize, strFiller.c_str(), iFileLen);

		fclose(pFile);
	}
	else
	{
		iDataSize = iHeaderSize + iFooterSize;
		pSendData = new char[iDataSize];
		memcpy(pSendData, strData.c_str(), iDataSize);
	}

	memcpy(pSendData + iDataSize - iFooterSize, HTTP_POST_FOOTER.c_str(), iFooterSize);

	if (!PostRequest(strServer.c_str(), strURI.c_str(), pSendData, iDataSize))
	{
		return HTTPERR_REQUEST;
	}

	delete pSendData;

	pMemBuf = new char[RECV_BUFFER_SIZE];

	if (pMemBuf == 0)
	{
		return HTTPERR_MEMORY;
	}

	if (GetResponse(pMemBuf) != 0)
	{
		return HTTPERR_RESPONSE;
	}

	// if ( !IsResponseOK( pMemBuf ) )
	//		return HTTPERR_NOTOK;
	delete[] pMemBuf;

	if (ParseHeader() != 0)
	{
		return HTTPERR_HEADERSKIP;
	}

	if (strLocalFile.size())
		return WriteToFile(strLocalFile.c_str());

	return HTTPERR_NONE;
}

/* =============================================================================
 =============================================================================== */
bool CHttpDownload::GetRequest(const char *pszHost, const char *pszRequest, const char *pszAuth)
{
	char	pszEncoded[1024] = "";

	if (pszAuth && *pszAuth)
	{
		string	strEncoded = encode_base64(pszAuth);
		strcpy(pszEncoded, strEncoded.c_str());
	}

	char	*pReqStr = new char[HTTP_GET_REQUEST.size() + strlen(pszRequest) + strlen(pszHost) + strlen(pszEncoded) + 128];

	if (strlen(pszEncoded) > 0)
		sprintf(pReqStr, HTTP_GET_REQUEST_AUTH.c_str(), pszRequest, pszHost, pszEncoded);
	else
		sprintf(pReqStr, HTTP_GET_REQUEST.c_str(), pszRequest, pszHost);

	int nErr = SendText(pReqStr);

	delete[] pReqStr;

	if (nErr == 0 || nErr == SOCKET_ERROR)
	{
		return false;
	}
	else
	{
		return true;
	}
}

/* =============================================================================
 =============================================================================== */
bool CHttpDownload::PostRequest(const char *pszHost, const char *pszRequest, const char *pData, int iDataSize)
{
	char	*pchReq = new char[HTTP_POST_REQUEST.size() + strlen(pszRequest) + strlen(pszHost) + iDataSize + MAX_PATH];
	sprintf(pchReq, HTTP_POST_REQUEST.c_str(), pszRequest, pszHost, iDataSize);

	int iHeaderSize = strlen(pchReq);
	int iLen = iHeaderSize + iDataSize;
	memcpy(pchReq + iHeaderSize, pData, iDataSize);
	pchReq[iLen] = '\0';

	int nErr = Send(pchReq, iLen);

	delete[] pchReq;

	if (nErr == 0 || nErr == SOCKET_ERROR)
	{
		return false;
	}
	else
	{
		return true;
	}
}

/* =============================================================================
 =============================================================================== */
int CHttpDownload::GetResponse(char *pszBuf)
{
	int nErr = RecvLine(pszBuf, RECV_BUFFER_SIZE);

	if (strstr(pszBuf, "302"))
	{
		while(!strstr(pszBuf, "Location:") && !strstr(pszBuf, "Connection:"))
			nErr = RecvLine(pszBuf, RECV_BUFFER_SIZE);
	}

	if (nErr == 0 || nErr == SOCKET_ERROR)
	{
		return HTTPERR_DISCONNECTED;
	}
	else
	{
		return HTTPERR_NONE;
	}
}

/* =============================================================================
 =============================================================================== */
bool CHttpDownload::IsResponseOK(const char *pszResponse)
{
	if (strstr(pszResponse, HTTP_OK_STRING) == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

/* =============================================================================
 =============================================================================== */
int CHttpDownload::ParseHeader(void)
{
	char			buf[2048];
	char			*pch = buf;
	CWinTcpSocket	*pSocket = m_AcceptSocket ? m_AcceptSocket : this;
	m_Boundary = "";
	m_DownloadSize = 0;

	int iSize = pSocket->Recv(buf, 2048, MSG_PEEK);
	pch = strstr(buf, "\r\n\r\n");
	if (!pch || iSize == 0)
		return SOCKET_ERROR;

	iSize = pch - buf + 4;
	if (pSocket->Recv(buf, iSize) != iSize)
		return SOCKET_ERROR;
	buf[iSize] = '\0';

	pch = strstr(buf, "Content-Length");
	if (pch)
		sscanf(pch, "Content-Length: %i", &m_DownloadSize);

	m_Chunked = false;
	pch = strstr(buf, "Transfer-Encoding");
	if (pch)
	{
		sscanf(pch, "Transfer-Encoding: %s", buf);
		if (strcmp(buf, "chunked") == 0)
			m_Chunked = true;
	}

	pch = strstr(buf, "boundary");
	if (pch)
	{
		char	buf[256];
		sscanf(pch, "boundary=%s\r", buf);
		m_Boundary = buf;
	}

	return HTTPERR_NONE;
}

/* =============================================================================
 =============================================================================== */
int CHttpDownload::WriteToString(string &strMsg)
{
	CWinTcpSocket	*pSocket = m_AcceptSocket ? m_AcceptSocket : this;
	try
	{
		strMsg.resize(m_DownloadSize);
	}
	catch(std::bad_alloc const &)
	{
		cout << "Memory allocation failure!" << endl;
		return HTTPERR_FILEIO;
	}

	// have to download in a loop - it brings down as much as it wants to bring down
	int nErr = 0;
	for (int iTotSize = 0; iTotSize < m_DownloadSize; iTotSize += nErr)
	{
		nErr = pSocket->Recv(&strMsg[iTotSize], m_DownloadSize);
		if (nErr == 0)
			break;
		else if (nErr == SOCKET_ERROR)
			return HTTPERR_DISCONNECTED;
	}

	return HTTPERR_NONE;
}

#define HTTP_LOAD_START	1000000
#define HTTP_LOAD_STEP	500000

void	progress_dlg(string caption, string text);
void	progress_update(string pText, double val);
void	progress_end();
void	download_progress_dlg(string caption, string text);
void	download_progress_update(string caption, double val);
void	download_progress_end();

#ifdef UWWF_EXPORTS

/* =============================================================================
 =============================================================================== */
void progress_dlg(string caption)
{ }

/* =============================================================================
 =============================================================================== */
void progress_update(string pText, double val)
{ }

/* =============================================================================
 =============================================================================== */
void progress_end()
{ }

/* =============================================================================
 =============================================================================== */
void download_progress_dlg(string caption, string text)
{ }

/* =============================================================================
 =============================================================================== */
void download_progress_update(string pText, double val)
{ }

/* =============================================================================
 =============================================================================== */
void download_progress_end()
{ }
#endif

/* =============================================================================
 =============================================================================== */
int CHttpDownload::WriteToFile(const char *pszFile)
{
	char	*pBuf = new char[RECV_BUFFER_SIZE];
	char	pchHeader[1024] = "";
	FILE	*pFile = fopen(pszFile, "wb");

	if (pFile == 0)
		return HTTPERR_FILEIO;

	int		fShow = HTTP_LOAD_STEP;

	bool	bProgress = m_DownloadSize > HTTP_LOAD_START;
	if (bProgress)
	{
		string	strName = g_Env.m_DownloadName;
		if (strName == "")
			strName = remExt(getFileName(pszFile));
		//download_progress_dlg("Web Download", "Downloading " + strName);
	}

	for (int i = 0;; i++)
	{
		if (m_Chunked)
		{
			char	buf[1024];
			char	*pch = buf;
			if (i > 0)	//  pull off trailing /r/n on chunked data
			{
				if (Recv(buf, 2) != 2)
					break;
			}

			int iSize = Recv(buf, 1024, MSG_PEEK);
			pch = strstr(buf, "\r\n");
			if (!pch || iSize == 0)
				break;
			iSize = pch - buf + 2;
			if (Recv(buf, iSize) != iSize)
				break;
			buf[iSize] = '\0';
			sscanf(buf, "%x", &m_DownloadSize);
			if (m_DownloadSize == 0)
				break;
			m_DownloadCur = 0;
		}

		int nCnt = 0;
		for (int j = 0; m_DownloadCur < m_DownloadSize; m_DownloadCur += nCnt, j++)
		{
			nCnt = Recv(pBuf, min(RECV_BUFFER_SIZE, m_DownloadSize - m_DownloadCur));

			if (nCnt == 0)
				break;

			if (nCnt == SOCKET_ERROR)
			{
				fclose(pFile);
				delete[] pBuf;
				return HTTPERR_DISCONNECTED;
			}

			fwrite(pBuf, sizeof(char), nCnt, pFile);

			if (i == 0 && j == 0)
			{
				int iCnt = min(1024, nCnt);
				memcpy(pchHeader, pBuf, iCnt - 1);
				pchHeader[iCnt - 1] = '\0';
			}

			if (bProgress && fShow < m_DownloadCur)
			{
				double	dPercent = (double) m_DownloadCur / (double) m_DownloadSize;
				//download_progress_update("", dPercent);
				fShow = m_DownloadCur + HTTP_LOAD_STEP;
			}
		}

		if (!m_Chunked)
			break;
	}

	fclose(pFile);

	if (bProgress)
	{
		//download_progress_update("", 1.0);
		//if (((string) pszFile).find(".zip.tmp") == string::npos && getExt(remExt((string) pszFile)) != "")
		//	download_progress_end();
	}

	int iRet = HTTPERR_NONE;
	if	(
		strstr(pchHeader, "Error report")
	||	strstr(pchHeader, "Internal Server Error")
	||	strstr(pchHeader, "Service Unavailable")
	||	strstr(pchHeader, "Service Temporarily Unavailable")
	||	strstr(pchHeader, "ServiceExceptionReport")
	||	strstr(pchHeader, "404 Not Found")
	||	strstr(pchHeader, "403 Forbidden")
	||	strstr(pchHeader, "Detailed Error")
	)
	{
		//cout << "Error downloading: " + (string)pszFile << endl;
		iRet = HTTPERR_FILEIO;
	}

	delete[] pBuf;
	return iRet;
}
