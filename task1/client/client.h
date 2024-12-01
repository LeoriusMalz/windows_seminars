#ifndef __client_h__
#define __client_h__

#define WIN32_LEAN_AND_MEAN
#undef UTF8

#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define BUFSIZE 512
#define DEFAULT_PORT "51488"
#define EOK 0

DWORD WINAPI WriteToPipe(LPDWORD);
DWORD WINAPI ReadFromPipe(LPDWORD);

void CreateSocket(const CHAR *argv);
void ShutdownConnection(void);

#endif
