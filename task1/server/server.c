#include "server.h"

extern HANDLE g_hChildStd_IN_Rd;
extern HANDLE g_hChildStd_IN_Wr;
extern HANDLE g_hChildStd_OUT_Rd;
extern HANDLE g_hChildStd_OUT_Wr;
extern SOCKET ClientSocket;

void WorkWithClient(void) {
  CreatePipes();
  CreateSocket();
  CreateChildProcess();

  HANDLE threads[2];
  threads[0] = CreateThread(NULL, 0, WriteToPipe, NULL, 0, NULL);
  threads[1] = CreateThread(NULL, 0, ReadFromPipe, NULL, 0, NULL);

  WaitForMultipleObjects(2, threads, TRUE, INFINITE);

  for (int i = 0; i < 2; ++i)
    CloseHandle(threads[i]);

  CloseConnection();
}

void CreateSocket(void) {
  WSADATA wsaData;
  DWORD iResult, iSendResult;
  SOCKET ListenSocket = INVALID_SOCKET;

  struct addrinfo *result = NULL;
  struct addrinfo hints;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed with error: %d\n", iResult);
    ErrorExit(TEXT("WSAStartup failed"));
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // Resolve the server address and port
  iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    ErrorExit(TEXT("getaddrinfo failed"));
  }

  // Create a SOCKET for the server to listen for client connections.
  ListenSocket =
      socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (ListenSocket == INVALID_SOCKET) {
    printf("socket failed with error: %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    ErrorExit(TEXT("socket failed"));
  }

  // Setup the TCP listening socket
  iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    ErrorExit(TEXT("bind failed"));
  }

  freeaddrinfo(result);

  iResult = listen(ListenSocket, SOMAXCONN);
  if (iResult == SOCKET_ERROR) {
    printf("listen failed with error: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    ErrorExit(TEXT("listen failed"));
  }

  // Accept a client socket
  ClientSocket = accept(ListenSocket, NULL, NULL);
  if (ClientSocket == INVALID_SOCKET) {
    printf("accept failed with error: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    ErrorExit(TEXT("accept failed"));
  }

  // No longer need server socket
  closesocket(ListenSocket);
}

void CloseConnection(void) {
  DWORD iResult = shutdown(ClientSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed with error: %d\n", WSAGetLastError());
    closesocket(ClientSocket);
    WSACleanup();
    ErrorExit(TEXT("Shutdown Failed"));
  }
  // cleanup
  closesocket(ClientSocket);
  WSACleanup();
}

void CreatePipes(void) {
  SECURITY_ATTRIBUTES saAttr;

  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
    ErrorExit(TEXT("StdoutRd CreatePipe"));

  if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    ErrorExit(TEXT("Stdout SetHandleInformation"));

  if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
    ErrorExit(TEXT("Stdin CreatePipe"));

  if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
    ErrorExit(TEXT("Stdin SetHandleInformation"));
}

void CreateChildProcess(void) {
  TCHAR szCmdline[] = TEXT("C:\\Windows\\System32\\cmd.exe");
  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  BOOL bSuccess = FALSE;

  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO);
  siStartInfo.hStdError = g_hChildStd_OUT_Wr;
  siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
  siStartInfo.hStdInput = g_hChildStd_IN_Rd;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  bSuccess = CreateProcess(NULL, szCmdline, NULL, NULL, TRUE, 0, NULL, NULL,
                           &siStartInfo, &piProcInfo);

  if (!bSuccess)
    ErrorExit(TEXT("CreateProcess"));
  else {
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    CloseHandle(g_hChildStd_OUT_Wr);
    CloseHandle(g_hChildStd_IN_Rd);
  }
}

DWORD WINAPI WriteToPipe(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);

  DWORD dwWritten, iResult;
  CHAR chBuf[BUFSIZE] = "";

  for (;;) {
    BOOL bSuccess = FALSE;
    for (DWORD i = 0; i < BUFSIZE; ++i)
      chBuf[i] = 0;
    while (!(iResult = recv(ClientSocket, chBuf, BUFSIZE, 0)))
      ;
    if (iResult < 0) {
      printf("recv failed with error: %d\n", WSAGetLastError());
      exit(-1);
    }

    bSuccess =
        WriteFile(g_hChildStd_IN_Wr, chBuf, strlen(chBuf), &dwWritten, NULL);
    if (!bSuccess || !dwWritten) {
      return EOK;
    }
  }
  return EOK;
}

DWORD WINAPI ReadFromPipe(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);

  DWORD dwRead, dwWritten, iSendResult;
  CHAR chBuf[BUFSIZE + 1];

  for (;;) {
    BOOL bSuccess = FALSE;

    for (;;) {
      bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
      if (!bSuccess) {
        return EOK;
      }

      chBuf[dwRead] = 0;

      iSendResult = send(ClientSocket, chBuf, dwRead, 0);
      if (iSendResult == SOCKET_ERROR) {
        exit(-1);
      }
    }
  }
  return EOK;
}

void ErrorExit(PTSTR lpszFunction) {
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf, 0, NULL);

  lpDisplayBuf =
      (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) +
                                         lstrlen((LPCTSTR)lpszFunction) + 40) *
                                            sizeof(TCHAR));
  StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                  TEXT("%s failed with error %d: %s"), lpszFunction, dw,
                  lpMsgBuf);
  MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
  ExitProcess(1);
}
