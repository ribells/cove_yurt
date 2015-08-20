#pragma once

#ifdef WIN32
#include <winsock2.h>
#else

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SOCKET int
#define DWORD long
#define SOCKADDR_IN struct sockaddr_in
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

#endif

class CWinTcpSocket
{
public:
	CWinTcpSocket( void );
	virtual ~CWinTcpSocket( void );

	virtual bool Create( int af = AF_INET );
	virtual bool Create_UDP( int af = AF_INET );
	virtual bool Connect( const char *pszHost, int nPort );
	virtual bool SetTimeout( int secs );
	virtual bool Bind( int nLocalPort );
	virtual bool Bind_UDP( int nLocalPort );
	virtual bool Accept( CWinTcpSocket *pSocket );
	virtual bool Listen( int nBacklog = SOMAXCONN );
	virtual int Send( const void *pData, int nDataLen, int nFlags = 0 );
	virtual int SendText( const char *pszText );
	virtual int Recv( void *pData, int nDataLen, int nFlags = 0 );
	virtual int RecvFrom( void *pData, int nDataLen );
	virtual int RecvLine( char *pszBuf, int nLen, bool bEcho = false );
	virtual bool Shutdown( int nHow );
	virtual bool Close( void );

	SOCKET m_hSocket;

private:
	static DWORD m_dwRefCount;
};
