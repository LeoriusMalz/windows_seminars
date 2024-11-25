#define WIN32_LEAN_AND_MEAN

#include "server.h"
#include "service.h"

SERVICE_STATUS g_hSvcStatus;
SERVICE_STATUS_HANDLE g_hSvcStatusHandle;
HANDLE g_hSvcStopEvent = NULL;
HANDLE g_hExecuteService = NULL;

SOCKET ClientSocket = INVALID_SOCKET;

VOID WINAPI SvcMain(DWORD, LPTSTR *);
DWORD WINAPI ExecuteThread(LPDWORD);
VOID ServiceCleanup(void);

int main(int argc, TCHAR *argv[]) {
  if (argc == 2) {
    if (lstrcmpi(argv[1], TEXT("install")) == 0) {
      SvcInstall();
      return 0;
    } else if (lstrcmpi(argv[1], TEXT("delete")) == 0) {
      SvcRemove();
      return 0;
    } else if (lstrcmpi(argv[1], TEXT("app")) == 0) {
      for (;;) 
        WorkWithClient();
    } else {
      printf("Usage: install | delete | app\n");
      return -1;
    }
  }

  SERVICE_TABLE_ENTRY DispatchTable[] = {
      {SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain},
      {NULL, NULL}
  };

  if (!StartServiceCtrlDispatcher(DispatchTable)) {
    printf("Error: %d\n", GetLastError());
    SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
    return -1;
  }
  return 0;
}

VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv) {
  g_hSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);
  if (!g_hSvcStatusHandle) {
    SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
    return;
  }

  g_hSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  g_hSvcStatus.dwServiceSpecificExitCode = 0;

  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
  SvcInit(dwArgc, lpszArgv);
}

DWORD WINAPI ExecuteThread(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);
  for (;;) 
    WorkWithClient();
  return 0;
}

VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv) {
  g_hSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (g_hSvcStopEvent == NULL) {
    ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
    return;
  }

  ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

  g_hExecuteService = CreateThread(NULL, 0, ExecuteThread, NULL, 0, NULL);

  WaitForSingleObject(g_hSvcStopEvent, INFINITE);

  ServiceCleanup();
}

VOID ServiceCleanup(void) {
  TerminateThread(g_hExecuteService, 0);
  ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}
