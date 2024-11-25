#include "client.h"

extern SOCKET ClientSocket;

void InitializeSocket(const CHAR *serverAddress) {
    WSADATA wsaData;
    struct addrinfo *addressInfo = NULL, *current = NULL, settings;
    DWORD initResult;

    // Инициализация Winsock
    initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (initResult != 0) {
        fprintf(stderr, "WSAStartup error: %d\n", initResult);
        exit(EXIT_FAILURE);
    }

    ZeroMemory(&settings, sizeof(settings));
    settings.ai_family = AF_UNSPEC;
    settings.ai_socktype = SOCK_STREAM;
    settings.ai_protocol = IPPROTO_TCP;

    // Получение адреса сервера
    initResult = getaddrinfo(serverAddress, PORT_NUMBER, &settings, &addressInfo);
    if (initResult != 0) {
        fprintf(stderr, "getaddrinfo error: %d\n", initResult);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Попытка подключиться
    for (current = addressInfo; current != NULL; current = current->ai_next) {
        ClientSocket = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (ClientSocket == INVALID_SOCKET) {
            fprintf(stderr, "Socket creation error: %ld\n", WSAGetLastError());
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        // Подключение к серверу
        initResult = connect(ClientSocket, current->ai_addr, (int)current->ai_addrlen);
        if (initResult == SOCKET_ERROR) {
            closesocket(ClientSocket);
            ClientSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(addressInfo);

    if (ClientSocket == INVALID_SOCKET) {
        fprintf(stderr, "Connection failed!\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }
}

void TerminateConnection(void) {
    DWORD shutdownResult = shutdown(ClientSocket, SD_SEND);
    if (shutdownResult == SOCKET_ERROR) {
        fprintf(stderr, "Shutdown error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    closesocket(ClientSocket);
    WSACleanup();
}

DWORD WINAPI HandleWriteThread(LPDWORD unused) {
    UNREFERENCED_PARAMETER(unused);
    DWORD sendResult, bytesRead;
    CHAR buffer[BUFFER_SIZE] = {0};

    while (TRUE) {
        BOOL readSuccess = ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, BUFFER_SIZE, &bytesRead, NULL);
        if (!readSuccess) return SUCCESS_CODE;

        sendResult = send(ClientSocket, buffer, bytesRead, 0);
        if (sendResult == SOCKET_ERROR) return ERROR_CODE;
    }
}

DWORD WINAPI HandleReadThread(LPDWORD unused) {
    UNREFERENCED_PARAMETER(unused);
    DWORD recvResult, bytesWritten;
    CHAR buffer[BUFFER_SIZE + 1] = {0};

    while (TRUE) {
        recvResult = recv(ClientSocket, buffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            buffer[recvResult] = '\0';
            BOOL writeSuccess = WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, strlen(buffer), &bytesWritten, NULL);
            if (!writeSuccess) return SUCCESS_CODE;
        } else {
            return ERROR_CODE;
        }
    }
}
