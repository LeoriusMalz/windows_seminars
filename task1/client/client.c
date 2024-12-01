#include "client.h"

extern SOCKET ConnectSocket;

void CreateSocket(const CHAR *argv) {
  WSADATA wsaData;

  struct addrinfo *result = NULL, *ptr = NULL, hints;
  DWORD iResult;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed with error: %d\n", iResult);
    exit(1);
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  // Resolve the server address and port
  iResult = getaddrinfo(argv, DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    exit(1);
  }

  // Attempt to connect to an address until one succeeds
  for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

    // Create a SOCKET for connecting to server
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
      printf("socket failed with error: %ld\n", WSAGetLastError());
      WSACleanup();
      exit(1);
    }

    // Connect to server.
    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      closesocket(ConnectSocket);
      ConnectSocket = INVALID_SOCKET;
      continue;
    }
    break;
  }

  freeaddrinfo(result);

  if (ConnectSocket == INVALID_SOCKET) {
    printf("Unable to connect to server!\n");
    WSACleanup();
    exit(1);
  }
}

void ShutdownConnection(void) {
  DWORD iResult = shutdown(ConnectSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed with error: %d\n", WSAGetLastError());
    closesocket(ConnectSocket);
    WSACleanup();
    exit(-1);
  }
  // cleanup
  closesocket(ConnectSocket);
  WSACleanup();
}

DWORD WINAPI WriteToPipe(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);
  DWORD iResult, dwRead, bSuccess;

  for (;;) {
    CHAR chBuf[BUFSIZE] = "";

    bSuccess =
        ReadFile(GetStdHandle(STD_INPUT_HANDLE), chBuf, BUFSIZE, &dwRead, NULL);
    if (!bSuccess) {
      return EOK;
    }

    iResult = send(ConnectSocket, chBuf, dwRead, 0);
    if (iResult == SOCKET_ERROR) {
      return -1;
    }
  }
}

DWORD WINAPI ReadFromPipe(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);
  DWORD iResult, dwRead, bSuccess;

  for (;;) {
    CHAR chBuf[BUFSIZE + 1] = "";

    iResult = recv(ConnectSocket, chBuf, BUFSIZE, 0);
    if (iResult > 0) {
      chBuf[strlen(chBuf)] = 0;
      bSuccess = WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), chBuf,
                           strlen(chBuf), &dwRead, NULL);
      if (!bSuccess) {
        return EOK;
      }
    } else {
      return -1;
    }
  }
}
