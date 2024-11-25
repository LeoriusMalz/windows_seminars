#include "client.h"

SOCKET ClientSocket = INVALID_SOCKET;

int main(int argc, CHAR **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server-address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    InitializeSocket(argv[1]);

    HANDLE workerThreads[2];
    workerThreads[0] = CreateThread(NULL, 0, HandleWriteThread, NULL, 0, NULL);
    workerThreads[1] = CreateThread(NULL, 0, HandleReadThread, NULL, 0, NULL);

    WaitForMultipleObjects(2, workerThreads, TRUE, INFINITE);

    for (int i = 0; i < 2; i++) {
        CloseHandle(workerThreads[i]);
    }

    TerminateConnection();
    printf("Disconnected successfully.\n");

    return SUCCESS_CODE;
}
