#ifndef __service_h__
#define __service_h__

#include <windows.h>
#include <strsafe.h>
#include <tchar.h>

#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("ServerService")
#define SVC_ERROR ((DWORD)0xC0020001L)

VOID SvcInstall(void);
VOID SvcRemove(void);
VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcCtrlHandler(DWORD);
VOID SvcInit(DWORD, LPTSTR *);
VOID SvcReportEvent(LPTSTR);

#endif
