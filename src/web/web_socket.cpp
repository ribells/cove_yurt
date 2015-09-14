 /* =============================================================================
	File: web_socket.cpp
  =============================================================================== */

#include "web_socket.h"

#include <stdio.h>
#include <stdlib.h>
#include "const.h"

DWORD CWinTcpSocket::	m_dwRefCount = 0;

/* =============================================================================
 =============================================================================== */
CWinTcpSocket::CWinTcpSocket(void)
{
#ifdef WIN32
	WSADATA wd;
	if (++m_dwRefCount == 1)
	{
		::WSAStartup(0x0101, &wd);
	}
#endif
}

/* =============================================================================
 =============================================================================== */
CWinTcpSocket::~CWinTcpSocket(void)
{
	Close();

#ifdef WIN32
	if (--m_dwRefCount == 0)
	{
		::WSACleanup();
	}
#endif
}

/* =============================================================================
 =============================================================================== */
bool CWinTcpSocket::Create(int af)
{
	m_hSocket = ::socket(af, SOCK_STREAM, IPPROTO_TCP);

	if (m_hSocket == INVALID_SOCKET)
	{
#ifdef _DEBUG
		printf("ERROR: Socket creation failed for some reason\n");
#endif
		return false;
	}
	else
	{
		return true;
	}
}

/* =============================================================================
 =============================================================================== */
bool CWinTcpSocket::Create_UDP(int af)
{
	m_hSocket = ::socket(af, SOCK_DGRAM, 0);

	if (m_hSocket == INVALID_SOCKET)
	{
#ifdef _DEBUG
		printf("ERROR: Socket creation failed for some reason\n");
#endif
		return false;
	}
	else
	{
		return true;
	}
}

/* =============================================================================
 =============================================================================== */
bool CWinTcpSocket::Bind_UDP(int nLocalPort)
{
	SOCKADDR_IN addr;

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(nLocalPort);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	if (::bind(m_hSocket, (const sockaddr *) &addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
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
int CWinTcpSocket::RecvFrom(void *pData, int nDataLen)
{
	struct sockaddr_in	server;
#ifdef WIN32
	int					server_length = (int) sizeof(struct sockaddr_in);
#else
	unsigned int		server_length = (int) sizeof(struct sockaddr_in);
#endif
	memset((void *) &server, '\0', sizeof(struct sockaddr_in));

	return ::recvfrom(m_hSocket, (char *) pData, nDataLen, 0, (struct sockaddr *) &server, &server_length);
}

/* =============================================================================
 =============================================================================== */
bool CWinTcpSocket::Connect(const char *pszHost, int nPort)
{
	unsigned long	ulAddr = 0;
#ifdef WIN32
	hostent			*pEnt = ::gethostbyname(pszHost);
#else
	struct hostent	*pEnt = ::gethostbyname(pszHost);
#endif
	SOCKADDR_IN		addr;

	if (pEnt == 0)
	{
		ulAddr = ::inet_addr(pszHost);

		if (ulAddr == INADDR_NONE)
		{
//ifdef _DEBUG
//printf("ERROR: Invalid internet address\n");
//#endif
			return false;
		}
		else
		{
			addr.sin_family = AF_INET;
		}
	}
	else
	{
		memcpy(&ulAddr, pEnt->h_addr_list[0], sizeof(long));

		addr.sin_family = pEnt->h_addrtype;
	}

	addr.sin_addr.s_addr = ulAddr;
	addr.sin_port = htons(nPort);

	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	if (::connect(m_hSocket, (const sockaddr *) &addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
#ifdef _DEBUG
#ifdef WIN32
		//printf("ERROR: Unable to connect: %X / %d", ::WSAGetLastError(), ::WSAGetLastError() );
#endif
#endif
		return false;
	}
	else
	{
		return true;
	}
}

/* =============================================================================
 =============================================================================== */
bool CWinTcpSocket::SetTimeout(int secs)
{
	struct timeval	tv;

	tv.tv_sec = secs;
	tv.tv_usec = 0;
	if (setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *) &tv, sizeof(tv)) == SOCKET_ERROR)
		return false;
	return true;
}

/* =============================================================================
 =============================================================================== */
bool CWinTcpSocket::Bind(int nLocalPort)
{
	SOCKADDR_IN addr;

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(nLocalPort);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	if (::bind(m_hSocket, (const sockaddr *) &addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
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
bool CWinTcpSocket::Accept(CWinTcpSocket *pSocket)
{
	if (pSocket == 0)
	{
		return false;
	}

	SOCKADDR_IN		addr;
#ifdef WIN32
	int				len = sizeof(SOCKADDR_IN);
#else
	unsigned int	len = sizeof(SOCKADDR_IN);
#endif
	memset(&addr, 0, sizeof(SOCKADDR_IN));

	pSocket->m_hSocket = ::accept(m_hSocket, (sockaddr *) &addr, &len);

	if (pSocket->m_hSocket == INVALID_SOCKET)
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
bool CWinTcpSocket::Listen(int nBackLog)
{
	if (::listen(m_hSocket, nBackLog) == SOCKET_ERROR)
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
int CWinTcpSocket::Send(const void *pData, int nDataLen, int nFlags)
{
	return ::send(m_hSocket, (const char *) pData, nDataLen, nFlags);
}

/* =============================================================================
 =============================================================================== */
int CWinTcpSocket::SendText(const char *pszText)
{
	return Send(pszText, strlen(pszText));
}

/* =============================================================================
 =============================================================================== */
int CWinTcpSocket::Recv(void *pData, int nDataLen, int nFlags)
{
	return ::recv(m_hSocket, (char *) pData, nDataLen, nFlags);
}

/* =============================================================================
 =============================================================================== */
int CWinTcpSocket::RecvLine(char *pszBuf, int nLen, bool bEcho)
{
	int		nCount = 0;
	int		nRdLen;
	char	ch = 0;

	while(ch != '\n' && nCount < nLen)
	{
		nRdLen = Recv(&ch, 1);

		if (nRdLen == 0 || nRdLen == SOCKET_ERROR)
		{
			nCount = 0;
			break;
		}

		if (ch != '\n' && ch != '\r')
		{
			pszBuf[nCount] = ch;
			nCount++;
		}

		if (bEcho)
		{
			Send(&ch, 1);
		}
	}

	if (nCount != 0)
	{
		pszBuf[nCount] = 0;
	}

	return nCount ? nCount : nRdLen;
}

/* =============================================================================
 =============================================================================== */
bool CWinTcpSocket::Shutdown(int nHow)
{
	return ::shutdown(m_hSocket, nHow) == SOCKET_ERROR ? false : true;
}

/* =============================================================================
 =============================================================================== */
bool CWinTcpSocket::Close(void)
{
#ifdef WIN32
	return ::closesocket(m_hSocket) == SOCKET_ERROR ? false : true;
#else
	return ::close(m_hSocket) == SOCKET_ERROR ? false : true;
#endif
}
