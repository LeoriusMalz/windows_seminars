#include "service.h"

extern SERVICE_STATUS g_hSvcStatus;
extern SERVICE_STATUS_HANDLE g_hSvcStatusHandle;
extern HANDLE g_hSvcStopEvent;

VOID SvcRemove(void) {
  SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if(!hSCManager) {
    printf("Error: Can't open Service Control Manager\n");
    return;
  }

  SC_HANDLE hService = OpenService(hSCManager, SVCNAME, SERVICE_STOP | DELETE);
  if(!hService) {
    printf("Error: Can't remove service\n");
    CloseServiceHandle(hSCManager);
    return;
  }
  
  DeleteService(hService);
  CloseServiceHandle(hService);
  CloseServiceHandle(hSCManager);
  printf("Service deleted successfully\n");
}

VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                     DWORD dwWaitHint) {
  static DWORD dwCheckPoint = 1;

  g_hSvcStatus.dwCurrentState = dwCurrentState;
  g_hSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
  g_hSvcStatus.dwWaitHint = dwWaitHint;

  if (dwCurrentState == SERVICE_START_PENDING)
    g_hSvcStatus.dwControlsAccepted = 0;
  else
    g_hSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

  if ((dwCurrentState == SERVICE_RUNNING) ||
      (dwCurrentState == SERVICE_STOPPED))
    g_hSvcStatus.dwCheckPoint = 0;
  else
    g_hSvcStatus.dwCheckPoint = dwCheckPoint++;

  SetServiceStatus(g_hSvcStatusHandle, &g_hSvcStatus);
}

VOID SvcCtrlHandler(DWORD dwCtrl) {
  switch (dwCtrl) {
  case SERVICE_CONTROL_STOP:
    ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
    SetEvent(g_hSvcStopEvent);
    ReportSvcStatus(g_hSvcStatus.dwCurrentState, NO_ERROR, 0);
    return;
  case SERVICE_CONTROL_INTERROGATE:
    break;
  default:
    break;
  }
}

VOID SvcInstall() {
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  TCHAR szUnquotedPath[MAX_PATH];

  if (!GetModuleFileName(NULL, szUnquotedPath, MAX_PATH)) {
    printf("Cannot install service (%d)\n", GetLastError());
    return;
  }

  TCHAR szPath[MAX_PATH];
  StringCbPrintf(szPath, MAX_PATH, TEXT("\"%s\""), szUnquotedPath);

  schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

  if (NULL == schSCManager) {
    printf("OpenSCManager failed (%d)\n", GetLastError());
    return;
  }

  schService = CreateService(schSCManager, SVCNAME, SVCNAME, SERVICE_ALL_ACCESS,
                             SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
                             SERVICE_ERROR_NORMAL, szPath, NULL, NULL, NULL, NULL, NULL);

  if (schService == NULL) {
    printf("CreateService failed (%d)\n", GetLastError());
    CloseServiceHandle(schSCManager);
    return;
  } else
    printf("Service installed successfully\n");

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);
}

VOID SvcReportEvent(LPTSTR szFunction) {
  HANDLE hEventSource;
  LPCTSTR lpszStrings[2];
  TCHAR Buffer[80];

  hEventSource = RegisterEventSource(NULL, SVCNAME);

  if (NULL != hEventSource) {
    StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

    lpszStrings[0] = SVCNAME;
    lpszStrings[1] = Buffer;

    ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, 0, SVC_ERROR, NULL, 2, 0, lpszStrings, NULL);

    DeregisterEventSource(hEventSource);
  }
}
