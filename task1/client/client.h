#ifndef __client_h__
#define __client_h__

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_SIZE 512
#define PORT_NUMBER "51488"
#define SUCCESS_CODE 0
#define ERROR_CODE -1

DWORD WINAPI HandleWriteThread(LPDWORD);
DWORD WINAPI HandleReadThread(LPDWORD);

void InitializeSocket(const CHAR *serverAddress);
void TerminateConnection(void);

#endif
