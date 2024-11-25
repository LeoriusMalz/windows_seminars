#ifndef __server_h__
#define __server_h__

#define WIN32_LEAN_AND_MEAN
#undef UTF8

#include <strsafe.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <ws2tcpip.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "AdvApi32.lib")

#define EOK 0
#define BUFSIZE 512
#define DEFAULT_PORT "51488"

void setupPipes(void);
void configureSocket(void);
void spawnChildProcess(void);
void processClientConnection(void);
void closeClientSocket(void);
void exitWithError(PTSTR);

DWORD WINAPI handleWritePipe(LPDWORD);
DWORD WINAPI handleReadPipe(LPDWORD);

#endif
