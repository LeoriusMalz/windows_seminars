#include "client.h"

SOCKET ConnectSocket = INVALID_SOCKET;

DWORD main(DWORD argc, CHAR **argv) {
  if (argc != 2) {
    printf("usage: %s <server-address>\n", argv[0]);
    exit(1);
  }

  CreateSocket(argv[1]);

  HANDLE threads[2];
  threads[0] = CreateThread(NULL, 0, WriteToPipe, NULL, 0, NULL);
  threads[1] = CreateThread(NULL, 0, ReadFromPipe, NULL, 0, NULL);

  WaitForMultipleObjects(2, threads, TRUE, INFINITE);

  for (int i = 0; i < 2; ++i)
    CloseHandle(threads[i]);

  ShutdownConnection();
  printf("Disconnected from server\n");

  return EOK;
}
