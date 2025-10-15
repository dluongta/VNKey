

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <vector>

#include <shellapi.h>
#include <Commctrl.h>
#include <psapi.h>

#include "resource.h"

#include "../../../engine/Engine.h"

#include "VNKeyManager.h"
#include "VNKeyHelper.h"
#include "SystemTrayHelper.h"

using namespace std;

extern wchar_t _logBuffer[1024];
#define LOG(...)  wsprintfW(_logBuffer, __VA_ARGS__); \
					OutputDebugString(_logBuffer);

#define APP_SET_DATA(KEY, VAL) KEY = VAL; VNKeyHelper::setRegInt(_T(#KEY), KEY)
#define APP_GET_DATA(KEY, DEFAULT_VAL) KEY = VNKeyHelper::getRegInt(_T(#KEY), DEFAULT_VAL)

#define APP_CLASS _T("VNKeyVietnameseInputMethod")

extern void saveSmartSwitchKeyData();

extern int vLanguage;
extern int vInputType;
extern int vFreeMark;
extern int vCodeTable;
extern int vCheckSpelling;
extern int vUseModernOrthography;
extern int vQuickTelex;
extern int vSwitchKeyStatus;
extern int vRestoreIfWrongSpelling;
extern int vFixRecommendBrowser;
extern int vUseMacro;
extern int vUseMacroInEnglishMode;
extern int vAutoCapsMacro;
extern int vSendKeyStepByStep;
extern int vUseSmartSwitchKey;
extern int vUpperCaseFirstChar;
extern int vUseGrayIcon;
extern int vShowOnStartUp;
extern int vRunWithWindows;
extern int vSupportMetroApp;
extern int vCreateDesktopShortcut;
extern int vRunAsAdmin;
extern int vCheckNewVersion;
extern int vRememberCode;
extern int vOtherLanguage;
extern int vTempOffVNKey;
extern int vFixChromiumBrowser;