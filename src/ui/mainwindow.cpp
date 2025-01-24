#include "mainwindow.h"

#include <fstream>
#include <iostream>
#include <windowsx.h>
#include <ShlObj.h>

#include "../core/globals.h"
#include "../resource.h"
#include "../utils/crashlog.h"
#include "../utils/helpers.h"
#include "../utils/config.h"
#include "../mpv/player.h"
#include "../tray/tray.h"
#include "../ui/splash.h"
#include "../webview/webview.h"
#include "../updater/updater.h"

// Single-instance
bool FocusExistingInstance(const std::wstring &protocolArg)
{
    HWND hExistingWnd = FindWindowW(APP_CLASS, nullptr);
    if(hExistingWnd) {
        if(IsIconic(hExistingWnd)) {
            ShowWindow(hExistingWnd, SW_RESTORE);
        } else if(!IsWindowVisible(hExistingWnd)) {
            ShowWindow(hExistingWnd, SW_SHOW);
        }
        SetForegroundWindow(hExistingWnd);
        SetFocus(hExistingWnd);
        if(!protocolArg.empty()) {
            COPYDATASTRUCT cds;
            cds.dwData = 1;
            cds.cbData = static_cast<DWORD>(protocolArg.size() * sizeof(wchar_t));
            cds.lpData = (PVOID)protocolArg.c_str();
            SendMessage(hExistingWnd, WM_COPYDATA, 0, (LPARAM)&cds);
        }
        return true;
    }
    return false;
}

bool CheckSingleInstance(int argc, char* argv[])
{
    g_hMutex = CreateMutexW(nullptr, FALSE, L"SingleInstanceMtx_StremioWebShell");
    if(!g_hMutex){
        std::wcerr << L"CreateMutex failed => fallback to multi.\n";
        AppendToCrashLog("CreateMutex failed => fallback to multi.");
        return true;
    }
    std::wstring protocolArg;
    for(int i=1; i<argc; ++i){
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, NULL, 0);
        if(size_needed > 0){
            std::wstring argW(size_needed - 1, 0);
            MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, &argW[0], size_needed);
            if(argW.rfind(L"stremio://",0)==0 || argW.rfind(L"magnet:",0)==0 || FileExists(argW)) {
                protocolArg = argW;
                break;
            }
        }
    }

    if(GetLastError()==ERROR_ALREADY_EXISTS){
        FocusExistingInstance(protocolArg);
        return false;
    }
    return true;
}

void ToggleFullScreen(HWND hWnd, bool enable)
{
    static WINDOWPLACEMENT prevPlc={sizeof(prevPlc)};
    if(enable==g_isFullscreen) return;
    g_isFullscreen = enable;

    if(enable){
        GetWindowPlacement(hWnd, &prevPlc);
        MONITORINFO mi={sizeof(mi)};
        if(GetMonitorInfoW(MonitorFromWindow(hWnd,MONITOR_DEFAULTTOPRIMARY), &mi)){
            SetWindowLongW(hWnd,GWL_STYLE,WS_POPUP|WS_VISIBLE);
            SetWindowPos(hWnd,HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        }
    } else {
        SetWindowLongW(hWnd,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);
        SetWindowPlacement(hWnd,&prevPlc);
        SetWindowPos(hWnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
    }
}

// Dark/Light theme
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

static void UpdateTheme(HWND hWnd)
{
    if(g_useDarkTheme){
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    } else {
        BOOL dark = FALSE;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    }
}
// Handling inbound/outbound Messages
void SendToJS(const std::string &eventName, const nlohmann::json &eventData)
{
    static int nextId = 1;
    nlohmann::json msg;
    msg["type"]   = 1;
    msg["object"] = "transport";
    msg["id"]     = nextId++;

    msg["args"]   = { eventName, eventData };

    // Serialize to wstring + Post
    std::string payload = msg.dump();
    std::wstring wpayload(payload.begin(), payload.end());
    g_webview->PostWebMessageAsJson(wpayload.c_str());

#ifdef DEBUG_BUILD
    std::cout << "[Native->JS] " << payload << "\n";
#endif
}

void HandleEvent(const std::string &ev, std::vector<std::string> &args)
{
    if(ev=="mpv-command"){
        if(!args.empty() && args[0] == "loadfile" && args.size() > 1) {
            args[1] = decodeURIComponent(args[1]);
        }
        HandleMpvCommand(args);
    } else if(ev=="mpv-set-prop"){
        HandleMpvSetProp(args);
    } else if(ev=="mpv-observe-prop"){
        HandleMpvObserveProp(args);
    } else if(ev=="app-ready"){
        std::cout<<"[Native->JS] APP READY"<<"\n" << std::endl;
        g_isAppReady=true;
        HideSplash();
        PostMessage(g_hWnd, WM_NOTIFY_FLUSH, 0, 0);
    } else if(ev=="update-requested"){
        RunInstallerAndExit();
    } else if(ev=="start-drag"){
        ReleaseCapture();
        SendMessageW(g_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    } else if(ev=="refresh"){
        refreshWeb(args.size()>0 && args[0]=="all");
    } else if(ev=="app-error"){
        if(!args.empty() && args.size()>0 && args[0] == "shellComm"){
            if(g_hSplash && !g_waitStarted.exchange(true)){
                WaitAndRefreshIfNeeded();
            }
        }
    }
    else {
        std::cout<<"Unknown event="<<ev<<"\n";
    }
}


void HandleInboundJSON(const std::string &msg)
{
    try {
        std::cout << "[JS -> NATIVE]: " << msg << std::endl;

        auto j = nlohmann::json::parse(msg);
        int type = 0;
        if (j.contains("type") && j["type"].is_number()) {
            type = j["type"].get<int>();
        }

        if (type == 3) {
            // 3 = Init event
            nlohmann::json root;
            root["id"] = 0;
            nlohmann::json transportObj;
            transportObj["properties"] = {
                1,
                nlohmann::json::array({0, "shellVersion", 0, APP_VERSION}),
            };
            transportObj["signals"] = {
                nlohmann::json::array({0, "handleInboundJSONSignal"}),
            };
            nlohmann::json methods = nlohmann::json::array();
            methods.push_back(nlohmann::json::array({"onEvent", "handleInboundJSON"}));
            transportObj["methods"] = methods;
            root["data"]["transport"] = transportObj;

            std::string payload = root.dump();
            std::wstring wpayload(payload.begin(), payload.end());
            g_webview->PostWebMessageAsJson(wpayload.c_str());
            return;
        }

        if (type == 6 && j.contains("method"))
        {
            std::string methodName = j["method"].get<std::string>();
            if (methodName == "handleInboundJSON")
            {
                if (j["args"].is_array() && !j["args"].empty())
                {
                    std::string ev;
                    if (j["args"][0].is_string()) {
                        ev = j["args"][0].get<std::string>();
                    } else {
                        ev = "Unknown";
                    }

                    std::vector<std::string> argVec;
                    if (j["args"].size() > 1)
                    {
                        auto &second = j["args"][1];
                        if (second.is_array())
                        {
                            for (auto &x: second)
                            {
                                if (x.is_string()) argVec.push_back(x.get<std::string>());
                                else argVec.push_back(x.dump());
                            }
                        }
                        else if (second.is_string()) {
                            argVec.push_back(second.get<std::string>());
                        }
                        else {
                            argVec.push_back(second.dump());
                        }
                    }

                    HandleEvent(ev, argVec);
                }
                else {
                    std::cout << "[WARN] invokeMethod=handleInboundJSON => no args array?\n";
                }
            }
            return;
        }
        std::cout<<"Unknown Inbound event="<<msg<<"\n";
    } catch(std::exception &ex) {
        std::cerr<<"JSON parse error:"<<ex.what()<<"\n";
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
    {
        HICON hIconBig   = LoadIcon(g_hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
        HICON hIconSmall = LoadIcon(g_hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
        SendMessage(hWnd, WM_SETICON, ICON_BIG,   (LPARAM)hIconBig);
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

        CreateTrayIcon(hWnd);
        UpdateTheme(hWnd);
        break;
    }
    case WM_DPICHANGED:
    {
        RECT* const newRect = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hWnd, NULL, newRect->left, newRect->top,
            newRect->right - newRect->left,
            newRect->bottom - newRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }
    case WM_NOTIFY_FLUSH: {
        if (g_isAppReady) {
            for(const auto& pendingMsg : g_outboundMessages) {
                SendToJS(pendingMsg["type"], pendingMsg);
            }
            g_outboundMessages.clear();
        }
        break;
    }
    case WM_SETTINGCHANGE:
    {
        UpdateTheme(hWnd);
        break;
    }
    case WM_TRAYICON:
    {
        if(LOWORD(lParam)==WM_RBUTTONUP) {
            ShowTrayMenu(hWnd);
        }
        if(lParam==WM_LBUTTONDBLCLK){
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        }
        break;
    }
    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
        case ID_TRAY_SHOWWINDOW:
            g_showWindow = !g_showWindow;
            ShowWindow(hWnd, g_showWindow?SW_SHOW:SW_HIDE);
            break;
        case ID_TRAY_ALWAYSONTOP:
            g_alwaysOnTop=!g_alwaysOnTop;
            SetWindowPos(hWnd,
                g_alwaysOnTop?HWND_TOPMOST:HWND_NOTOPMOST,
                0,0,0,0,
                SWP_NOMOVE|SWP_NOSIZE);
            break;
        case ID_TRAY_CLOSE_ON_EXIT:
            g_closeOnExit=!g_closeOnExit;
            SaveSettings();
            break;
        case ID_TRAY_USE_DARK_THEME:
            g_useDarkTheme=!g_useDarkTheme;
            SaveSettings();
            UpdateTheme(hWnd);
            break;
        case ID_TRAY_PICTURE_IN_PICTURE:
            TogglePictureInPicture(hWnd, !g_isPipMode);
            break;
        case ID_TRAY_PAUSE_FOCUS_LOST:
            g_pauseOnLostFocus=!g_pauseOnLostFocus;
            SaveSettings();
            break;
        case ID_TRAY_PAUSE_MINIMIZED:
            g_pauseOnMinimize=!g_pauseOnMinimize;
            SaveSettings();
            break;
        case ID_TRAY_QUIT:
            if(g_mpv) mpv_command_string(g_mpv,"quit");
            DestroyWindow(hWnd);
            break;
        }
        break;
    }
    case WM_COPYDATA:
    {
        PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
        if (pcds && pcds->dwData == 1 && pcds->lpData) {
            // Assuming data is a wide string containing the URL or file path
            std::wstring receivedUrl((wchar_t*)pcds->lpData, pcds->cbData / sizeof(wchar_t));
            std::wcout << L"Received URL in main instance: " << receivedUrl << std::endl;

            // Check if received URL is a file and exists
            if (FileExists(receivedUrl)) {
                // Extract file extension
                size_t dotPos = receivedUrl.find_last_of(L".");
                std::wstring extension = (dotPos != std::wstring::npos) ? receivedUrl.substr(dotPos) : L"";

                if (extension == L".torrent") {
                    // Handle .torrent files
                    std::string utf8FilePath = WStringToUtf8(receivedUrl);

                    std::ifstream ifs(utf8FilePath, std::ios::binary);
                    if (!ifs) {
                        std::cerr << "Error: Could not open torrent file.\n";
                        break;
                    }
                    std::vector<unsigned char> fileBuffer(
                        (std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>())
                    );

                    json j;
                    j["type"] = "OpenTorrent";
                    j["data"] = fileBuffer;
                    SendToJS("OpenTorrent", j);
                } else {
                    // Handle other media files
                    std::string utf8FilePath = WStringToUtf8(receivedUrl);
                    json j;
                    j["type"] = "OpenFile";
                    j["path"] = utf8FilePath;
                    SendToJS("OpenFile", j);
                }
            } else if (receivedUrl.rfind(L"stremio://", 0) == 0) {
                // Handle stremio:// protocol
                std::string utf8Url = WStringToUtf8(receivedUrl);
                json j;
                j["type"] = "AddonInstall";
                j["path"] = utf8Url;
                SendToJS("AddonInstall", j);
            } else if (receivedUrl.rfind(L"magnet:", 0) == 0) {
                std::string utf8Url = WStringToUtf8(receivedUrl);
                json j;
                j["type"] = "OpenTorrent";
                j["magnet"] = utf8Url;
                SendToJS("OpenTorrent", j);
            } else {
                std::wcout << L"Received URL is neither a valid file nor a stremio:// protocol." << std::endl;
            }
        }
        return 0;
    }
    case WM_CLOSE:
    {
        WINDOWPLACEMENT wp;
        wp.length = sizeof(wp);
        if (GetWindowPlacement(hWnd, &wp)) {
            // Save to ini
            SaveWindowPlacement(wp);
        }

        if(g_closeOnExit) {
            DestroyWindow(hWnd);
        } else {
            ShowWindow(hWnd, SW_HIDE);
            pauseMPV(g_pauseOnMinimize);
            g_showWindow=false;
        }
        return 0;
    }
    case WM_ACTIVATE:
    {
        if(LOWORD(wParam)==WA_INACTIVE){
            pauseMPV(g_pauseOnLostFocus);
        }
        break;
    }
    case WM_SIZE:
    {
        if(wParam==SIZE_MINIMIZED){
            pauseMPV(g_pauseOnMinimize);
        }
        if(g_webviewController){
            RECT rc; GetClientRect(hWnd,&rc);
            g_webviewController->put_Bounds(rc);
        }
        if(g_hSplash){
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            SetWindowPos(g_hSplash,nullptr,0,0,w,h,SWP_NOZORDER);
        }
        break;
    }
    case WM_MPV_WAKEUP:
        HandleMpvEvents();
        break;
    case WM_DESTROY:
    {
        // release mutex
        if(g_hMutex) { CloseHandle(g_hMutex); g_hMutex=nullptr; }
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
