#ifndef __server_h__
#define __server_h__

#define WIN32_LEAN_AND_MEAN
#undef UTF8

#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <tchar.h>

#include <windows.h>
#include <ws2tcpip.h>
#include <winsock2.h>


#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "AdvApi32.lib")

#define BUFSIZE 512
#define EOK 0
#define DEFAULT_PORT "51488"

void WorkWithClient(void);
void CreateChildProcess(void);
void CreatePipes(void);
void CreateSocket(void);
void CloseConnection(void);
void ErrorExit(PTSTR);

DWORD WINAPI WriteToPipe(LPDWORD);
DWORD WINAPI ReadFromPipe(LPDWORD);

#endif
