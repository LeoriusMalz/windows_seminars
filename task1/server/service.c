#include "service.h"

extern SERVICE_STATUS g_hSvcStatus;
extern SERVICE_STATUS_HANDLE g_hSvcStatusHandle;
extern HANDLE g_hSvcStopEvent;

VOID SvcInstall() {
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  TCHAR szUnquotedPath[MAX_PATH];

  // printf("Svc install begin\n");
  if (!GetModuleFileName(NULL, szUnquotedPath, MAX_PATH)) {
    printf("Cannot install service (%d)\n", GetLastError());
    return;
  }

  TCHAR szPath[MAX_PATH];
  StringCbPrintf(szPath, MAX_PATH, TEXT("\"%s\""), szUnquotedPath);

  // Get a handle to the SCM database.
  schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

  if (NULL == schSCManager) {
    printf("OpenSCManager failed (%d)\n", GetLastError());
    return;
  }

  schService = CreateService(schSCManager,       // SCM database
                             SVCNAME,            // name of service
                             SVCNAME,            // service name to display
                             SERVICE_ALL_ACCESS, // desired access
                             SERVICE_WIN32_OWN_PROCESS, // service type
                             SERVICE_DEMAND_START,      // start type
                             SERVICE_ERROR_NORMAL,      // error control type
                             szPath, // path to service's binary
                             NULL,   // no load ordering group
                             NULL,   // no tag identifier
                             NULL,   // no dependencies
                             NULL,   // LocalSystem account
                             NULL);  // no password

  if (schService == NULL) {
    printf("CreateService failed (%d)\n", GetLastError());
    CloseServiceHandle(schSCManager);
    return;
  } else
    printf("Service installed successfully\n");

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);
}

VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                     DWORD dwWaitHint) {
  static DWORD dwCheckPoint = 1;

  // Fill in the SERVICE_STATUS structure.
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

  // Report the status of the service to the SCM.
  SetServiceStatus(g_hSvcStatusHandle, &g_hSvcStatus);
}

VOID WINAPI SvcCtrlHandler(DWORD dwCtrl) {
  // Handle the requested control code.
  switch (dwCtrl) {
  case SERVICE_CONTROL_STOP:
    ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

    // Signal the service to stop.
    SetEvent(g_hSvcStopEvent);
    ReportSvcStatus(g_hSvcStatus.dwCurrentState, NO_ERROR, 0);

    return;

  case SERVICE_CONTROL_INTERROGATE:
    break;

  default:
    break;
  }
}

VOID SvcReportEvent(LPTSTR szFunction) {
  HANDLE hEventSource;
  LPCTSTR lpszStrings[2];
  TCHAR Buffer[80];

  hEventSource = RegisterEventSource(NULL, SVCNAME);

  if (NULL != hEventSource) {
    StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction,
                    GetLastError());

    lpszStrings[0] = SVCNAME;
    lpszStrings[1] = Buffer;

    ReportEvent(hEventSource,        // event log handle
                EVENTLOG_ERROR_TYPE, // event type
                0,                   // event category
                SVC_ERROR,           // event identifier
                NULL,                // no security identifier
                2,                   // size of lpszStrings array
                0,                   // no binary data
                lpszStrings,         // array of strings
                NULL);               // no binary data

    DeregisterEventSource(hEventSource);
  }
}

VOID SvcRemove(void) {
  SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if(!hSCManager) {
    printf("Error: Can't open Service Control Manager\n");
    //  addLogMessage("Error: Can't open Service Control Manager");
    return;
  }

  SC_HANDLE hService = OpenService(hSCManager, SVCNAME, SERVICE_STOP | DELETE);
  if(!hService) {
    printf("Error: Can't remove service\n");
    //  addLogMessage("Error: Can't remove service");
    CloseServiceHandle(hSCManager);
    return;
  }
  
  DeleteService(hService);
  CloseServiceHandle(hService);
  CloseServiceHandle(hSCManager);
  // addLogMessage("Success remove service!");
  printf("Service deleted successfully\n");
}
