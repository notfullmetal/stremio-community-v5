#include <windows.h>
#include <string>
#include "config.h"

#include <sstream>

#include "../core/globals.h"
#include "../utils/helpers.h"

// Return the path to "portable_config/stremio-settings.ini"
static std::wstring GetIniPath()
{
    std::wstring exeDir = GetExeDirectory();
    std::wstring pcDir  = exeDir + L"\\portable_config";
    CreateDirectoryW(pcDir.c_str(), nullptr);  // ensure it exists
    return pcDir + L"\\stremio-settings.ini";
}

void LoadSettings()
{
    std::wstring iniPath = GetIniPath();
    wchar_t buffer[16];

    GetPrivateProfileStringW(L"General", L"CloseOnExit", L"0", buffer, _countof(buffer), iniPath.c_str());
    g_closeOnExit = (wcscmp(buffer, L"1") == 0);
    GetPrivateProfileStringW(L"General", L"UseDarkTheme", L"1", buffer, _countof(buffer), iniPath.c_str());
    g_useDarkTheme = (wcscmp(buffer, L"1") == 0);
    g_thumbFastHeight = GetPrivateProfileIntW(L"General", L"ThumbFastHeight", 0, iniPath.c_str());
    g_allowZoom = GetPrivateProfileIntW(L"General", L"AllowZoom", 0, iniPath.c_str());
    g_pauseOnMinimize = (GetPrivateProfileIntW(L"General", L"PauseOnMinimize", 1, iniPath.c_str()) == 1);
    g_pauseOnLostFocus = (GetPrivateProfileIntW(L"General", L"PauseOnLostFocus", 0, iniPath.c_str()) == 1);
    g_isRpcOn = (GetPrivateProfileIntW(L"General", L"DiscordRPC", 1, iniPath.c_str()) == 1);
    //Mpv
    wchar_t voBuffer[32];
    GetPrivateProfileStringW(L"MPV", L"VideoOutput", L"gpu-next", voBuffer, 32, iniPath.c_str());
    char narrowVO[32];
    WideCharToMultiByte(CP_UTF8, 0, voBuffer, -1, narrowVO, 32, NULL, NULL);
    g_initialVO = narrowVO;
    g_currentVolume = GetPrivateProfileIntW(L"MPV", L"InitialVolume", 50, iniPath.c_str());
}

void SaveSettings()
{
    std::wstring iniPath = GetIniPath();

    const wchar_t* closeVal = g_closeOnExit    ? L"1" : L"0";
    const wchar_t* darkVal  = g_useDarkTheme   ? L"1" : L"0";
    const wchar_t* pauseMinVal  = g_pauseOnMinimize ? L"1" : L"0";
    const wchar_t* pauseFocVal  = g_pauseOnLostFocus ? L"1" : L"0";
    const wchar_t* allowZoomVal = g_allowZoom ? L"1" : L"0";
    const wchar_t* rpcVal = g_isRpcOn ? L"1" : L"0";

    WritePrivateProfileStringW(L"General", L"CloseOnExit", closeVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"UseDarkTheme", darkVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"PauseOnMinimize", pauseMinVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"PauseOnLostFocus", pauseFocVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"AllowZoom", allowZoomVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"DiscordRPC", rpcVal, iniPath.c_str());
    WriteIntToIni(L"MPV", L"InitialVolume", g_currentVolume, iniPath);
}

static void WriteIntToIni(const std::wstring &section, const std::wstring &key, int value, const std::wstring &iniPath)
{
    std::wstringstream ws;
    ws << value;
    WritePrivateProfileStringW(section.c_str(), key.c_str(), ws.str().c_str(), iniPath.c_str());
}

// This helper reads an integer from the .ini, returning `defaultVal` if not found
static int ReadIntFromIni(const std::wstring &section, const std::wstring &key, int defaultVal, const std::wstring &iniPath)
{
    return GetPrivateProfileIntW(section.c_str(), key.c_str(), defaultVal, iniPath.c_str());
}

/**
 * Optional helper:
 * Check if the given rectangle is on a valid monitor.
 * If completely off-screen, we re-center it on the primary monitor so the user sees it.
 */
static void EnsureRectOnScreen(RECT &rc)
{
    HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
    if(hMon)
    {
        return;
    }

    MONITORINFO mi = {0};
    mi.cbSize = sizeof(mi);
    HMONITOR hPrimary = MonitorFromPoint({0,0}, MONITOR_DEFAULTTOPRIMARY);
    if(!hPrimary || !GetMonitorInfoW(hPrimary, &mi)) {
        return;
    }

    int width  = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    int workWidth  = mi.rcWork.right  - mi.rcWork.left;
    int workHeight = mi.rcWork.bottom - mi.rcWork.top;

    // Center the rect in the primary monitor's WORK area
    int newLeft = mi.rcWork.left + (workWidth - width)/2;
    int newTop  = mi.rcWork.top  + (workHeight - height)/2;

    rc.left   = newLeft;
    rc.top    = newTop;
    rc.right  = newLeft + width;
    rc.bottom = newTop  + height;
}

/**
 * SaveWindowPlacement:
 * Writes showCmd (normal vs. maximized) and the normal window rectangle
 * into the [Window] section of our .ini.
 */
void SaveWindowPlacement(const WINDOWPLACEMENT &wp)
{
    std::wstring iniPath = GetIniPath();

    WriteIntToIni(L"Window", L"ShowCmd", (int)wp.showCmd, iniPath);
    WriteIntToIni(L"Window", L"Left",    wp.rcNormalPosition.left,   iniPath);
    WriteIntToIni(L"Window", L"Top",     wp.rcNormalPosition.top,    iniPath);
    WriteIntToIni(L"Window", L"Right",   wp.rcNormalPosition.right,  iniPath);
    WriteIntToIni(L"Window", L"Bottom",  wp.rcNormalPosition.bottom, iniPath);
}

/**
 * LoadWindowPlacement:
 * Reads ShowCmd + window rectangle from [Window].
 * If not found or incomplete, returns false => use default behavior.
 * If found, also ensures the rect is at least partially visible on a monitor.
 */
bool LoadWindowPlacement(WINDOWPLACEMENT &wp)
{
    wp.length = sizeof(wp);
    wp.flags  = 0;

    std::wstring iniPath = GetIniPath();

    // We default to SW_SHOWNORMAL if not found
    int showCmd = ReadIntFromIni(L"Window", L"ShowCmd", SW_SHOWNORMAL, iniPath);
    wp.showCmd  = (UINT)showCmd;

    // If any of these are -1 => means not found
    int left   = ReadIntFromIni(L"Window", L"Left",   -1, iniPath);
    int top    = ReadIntFromIni(L"Window", L"Top",    -1, iniPath);
    int right  = ReadIntFromIni(L"Window", L"Right",  -1, iniPath);
    int bottom = ReadIntFromIni(L"Window", L"Bottom", -1, iniPath);

    if(left == -1 || top == -1 || right == -1 || bottom == -1)
    {
        // No saved geometry
        return false;
    }

    wp.rcNormalPosition.left   = left;
    wp.rcNormalPosition.top    = top;
    wp.rcNormalPosition.right  = right;
    wp.rcNormalPosition.bottom = bottom;

    // Edge-case: if user had it on a disconnected monitor => re-center
    EnsureRectOnScreen(wp.rcNormalPosition);

    return true;
}