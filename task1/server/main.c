#define WIN32_LEAN_AND_MEAN

#include "server.h"
#include "service.h"

SERVICE_STATUS g_hSvcStatus;
SERVICE_STATUS_HANDLE g_hSvcStatusHandle;
HANDLE g_hSvcStopEvent = NULL;

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
SOCKET ClientSocket = INVALID_SOCKET;

HANDLE g_hExecuteService = NULL;
VOID WINAPI SvcMain(DWORD, LPTSTR *);

DWORD WINAPI ExecuteThread(LPDWORD);

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
      printf("Usage error: I know 3 arguments \n\t\"install\"\n\t\"delete\"\n\t\"app\"\n");
      return -1;
    }

  }

  SERVICE_TABLE_ENTRY DispatchTable[] = {
      {SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain}, {NULL, NULL}};

  if (!StartServiceCtrlDispatcher(DispatchTable)) {
    printf("Error while starting Dispatcher Table : %d\n",  GetLastError());
    ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
  }
}

VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv) {
  // Register the handler function for the service
  g_hSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);

  if (!g_hSvcStatusHandle) {
    SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
    return;
  }

  // These SERVICE_STATUS members remain as set here
  g_hSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  g_hSvcStatus.dwServiceSpecificExitCode = 0;

  // Report initial status to the SCM
  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
  // Perform service-specific initialization and work.
  SvcInit(dwArgc, lpszArgv);
}

DWORD WINAPI ExecuteThread(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);

  for (;;) WorkWithClient();

  return 0;
}

VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv) {
  g_hSvcStopEvent = CreateEvent(NULL,  // default security attributes
                                TRUE,  // manual reset event
                                FALSE, // not signaled
                                NULL); // no name

  if (g_hSvcStopEvent == NULL) {
    ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
    return;
  }

  // Report running status when initialization is complete.
  ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

  g_hExecuteService = CreateThread(NULL, 0, ExecuteThread, NULL, 0, NULL);

  while (1) {
    // Check whether to stop the service.
    WaitForSingleObject(g_hSvcStopEvent, INFINITE);
    TerminateThread(g_hExecuteService, 0);
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
    return;
  }
}
