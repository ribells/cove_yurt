#pragma once

#include "web_socket.h"

#include <vector>
#include <string>
using namespace std;

#define HTTP_OK_STRING			"200 OK"
#define HTTP_OK					200
#define HTTPERR_NONE			0

#define HTTPERR_REQUEST			0x0001
#define HTTPERR_RESPONSE		0x0002
#define HTTPERR_NOTOK			0x0004
#define HTTPERR_HEADERSKIP		0x0008
#define HTTPERR_FILEIO			0x0010
#define HTTPERR_MEMORY			0x0020
#define HTTPERR_DISCONNECTED	0x0040

#define HttpErrorString( x, y ) itoa( y, x, sizeof( x ) )

typedef enum _tagHttpVersion
{
	HTTPVER_10 = 0x0100,
	HTTPVER_11 = 0x0101

}HttpVersion;

bool parseURL(string WebPath, string &strServer, string &strURI, int *iPort);
string getPostFileHeader(string weblocation, string filename);
string getPostParmHeader(string parm, string val);

class CHttpDownload : public CWinTcpSocket
{
private:
	int		m_DownloadSize;
	int		m_DownloadCur;
	string	m_Boundary;
	bool	m_Chunked;
	CWinTcpSocket * m_AcceptSocket;

public:
	CHttpDownload( void );
	~CHttpDownload() { if (m_AcceptSocket) delete m_AcceptSocket; }

	virtual int DownloadFile( string strServer, string strURI, string strLocalFile, string strAuth, int iCnt=0 );
	virtual int PostData( string strServer, string strURI, string strLocalFile, string strData );

	void responseSuccess(CWinTcpSocket *pSocket, string str);
	void responseFail(CWinTcpSocket *pSocket, string str);
	void responseFile(CWinTcpSocket *pSocket, string strFile, string strType);
	bool ParseArgs(CWinTcpSocket *pSocket, vector<string> &strTags, vector<string> &strVals);
	void setAcceptSocket(CWinTcpSocket *pSocket) { m_AcceptSocket = pSocket; }

protected:
	bool GetRequest( const char *pszRequest, const char *pszHost, const char *pszAuth );
	bool PostRequest( const char *pszRequest, const char *pszHost, const char *pData, int iDataSize );
	int GetResponse( char *pszBuf );
	bool IsResponseOK( const char *pszResponse );
	int ParseHeader( void );
	int WriteToFile( const char *pszFile );
	int WriteToString( string & strMsg );
};
