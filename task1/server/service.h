#ifndef __service_h__
#define __service_h__

#include <strsafe.h>
#include <tchar.h>
#include <windows.h>

#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("ServerService")
#define SVC_ERROR ((DWORD)0xC0020001L)

VOID SvcInstall(void);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR *);
VOID SvcReportEvent(LPTSTR);
VOID SvcRemove(void);

#endif
