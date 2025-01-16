#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:mainCRTStartup")

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <tchar.h>
#include <string>
#include <thread>
#include <atomic>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <map>
#include <wrl.h>
#include <wil/com.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <filesystem>
#include <vector>

#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>


#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "Msimg32.lib")

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")


// WebView2
#include "WebView2.h"

// mpv
#include "mpv/client.h"

// png
#include "resource.h"

// nlohmann/json
#include <WebView2EnvironmentOptions.h>
#include <WebView2EnvironmentOptions.h>
#include <WebView2EnvironmentOptions.h>
#include <WebView2EnvironmentOptions.h>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

using namespace Microsoft::WRL;

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

#define APP_TITLE "Stremio - Freedom to Stream"
#define APP_NAME "Stremio"
#define APP_CLASS L"Stremio"
#define APP_VERSION "5.0.8"

static TCHAR  szWindowClass[]   = APP_NAME;
static TCHAR  szTitle[]         = APP_TITLE;

static HINSTANCE g_hInst   = nullptr;
static HWND      g_hWnd    = nullptr;
static HBRUSH    g_darkBrush = nullptr;
static HANDLE    g_hMutex  = nullptr;  // for single-instance
static HHOOK g_hMouseHook = nullptr;

// URLS
std::wstring g_webuiUrl = L"https://zaarrg.github.io/stremio-web-shell-fixes/";
std::string g_updateUrl = "https://raw.githubusercontent.com/Zaarrg/stremio-desktop-v5/refs/heads/webview-windows/version/version.json";

//Updater thread message
#define WM_NOTIFY_UPDATE (WM_USER + 101)
static json g_pendingUpdateMsg;

//Args
bool g_streamingServer = true;
bool g_autoupdaterForceFull = false;

// mpv
static mpv_handle* g_mpv = nullptr;
static std::set<std::string> g_observedProps;

#define IDR_MAINFRAME 101
#define WM_MPV_WAKEUP (WM_APP + 2)

// Node server
static std::atomic_bool g_nodeRunning = false;
static std::thread      g_nodeThread;
static HANDLE           g_nodeProcess = nullptr;
static HANDLE           g_nodeOutPipe = nullptr;
static HANDLE           g_nodeInPipe  = nullptr;

// WebView2
static wil::com_ptr<ICoreWebView2Controller4> g_webviewController;
static wil::com_ptr<ICoreWebView2Profile8>    g_webviewProfile;
static wil::com_ptr<ICoreWebView2_21>         g_webview;

// Tray
#define WM_TRAYICON         (WM_APP + 1)
#define ID_TRAY_SHOWWINDOW  1001
#define ID_TRAY_ALWAYSONTOP 1002
#define ID_TRAY_CLOSE_ON_EXIT  1003
#define ID_TRAY_USE_DARK_THEME 1004
#define ID_TRAY_PAUSE_MINIMIZED 1005
#define ID_TRAY_PAUSE_FOCUS_LOST 1006
#define ID_TRAY_PICTURE_IN_PICTURE 1007
#define ID_TRAY_QUIT        1008

struct MenuItem
{
    UINT id;
    bool checked;
    bool separator;
    std::wstring text;
};
static std::vector<MenuItem> g_menuItems;
static LRESULT CALLBACK DarkTrayMenuProc(HWND, UINT, WPARAM, LPARAM);

// State Globals
static NOTIFYICONDATA  g_nid        = {0};
static bool            g_showWindow = true;
static bool            g_alwaysOnTop= false;
static bool            g_isFullscreen = false;
static bool            g_closeOnExit = false;
static bool            g_useDarkTheme = false;
static bool            g_isPipMode = false;
static int             g_thumbFastHeight = 0;
static int             g_hoverIndex = -1;
static HFONT g_hMenuFont = nullptr;
static HANDLE g_serverJob = nullptr;
static HWND      g_trayHwnd    = nullptr;

//Ini Settings
static bool            g_pauseOnMinimize = true;
static bool            g_pauseOnLostFocus = false;
static bool            g_allowZoom = false;
// Tray Sizes
static int g_tray_itemH = 31;
static int g_tray_sepH = 8;
static int g_tray_w = 200;

// Splash Screen
static HWND g_hSplash = nullptr;
static HBITMAP g_hSplashImage = nullptr;
static float g_splashOpacity = 1.0f;
static int g_pulseDirection = -1; // -1 decrease, +1 increase
static ULONG_PTR g_gdiplusToken = 0;

// App Ready and Event Queue
static std::vector<json> g_pendingMessages;
static bool             g_isAppReady = false;

// Updater
static std::atomic_bool g_updaterRunning = false;
static std::filesystem::path g_installerPath;
static std::thread g_updaterThread;
static const char* public_key_pem = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoXoJRQ81xOT3Gx6+hsWM
ZiD4PwtLdxxNhEdL/iK0yp6AdO/L0kcSHk9YCPPx0XPK9sssjSV5vCbNE/2IJxnh
/mV+3GAMmXgMvTL+DZgrHafnxe1K50M+8Z2z+uM5YC9XDLppgnC6OrUjwRqNHrKI
T1vcgKf16e/TdKj8xlgadoHBECjv6dr87nbHW115bw8PVn2tSk/zC+QdUud+p6KV
zA6+FT9ZpHJvdS3R0V0l7snr2cwapXF6J36aLGjJ7UviRFVWEEsQaKtAAtTTBzdD
4B9FJ2IJb/ifdnVzeuNTDYApCSE1F89XFWN9FoDyw7Jkk+7u4rsKjpcnCDTd9ziG
kwIDAQAB
-----END PUBLIC KEY-----)";

// Thumb Fast

static std::atomic<bool> g_ignoreHover(false);
static std::chrono::steady_clock::time_point g_ignoreUntil;
constexpr std::chrono::milliseconds IGNORE_DURATION(200);

// Forward Declares

LRESULT CALLBACK SplashWndProc(HWND, UINT, WPARAM, LPARAM);
void CreateSplashScreen(HWND parent);
void HideSplash();


static json mpvNodeToJson(const mpv_node* node);

static HWND  GetMainHwnd()    { return g_hWnd; }
static void  CreateTrayIcon(HWND hWnd);
static void  RemoveTrayIcon();
static void  RunInstallerAndExit();
static void  StopNodeServer();
static void  ShowTrayMenu(HWND hWnd);
static void  AppendToCrashLog(const std::wstring& message);
static void  AppendToCrashLog(const std::string& message);
bool FileExists(const std::wstring& path);
bool DirectoryExists(const std::wstring& dirPath);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// JS injection Init Shell
static const wchar_t* INIT_SHELL_SCRIPT = LR"JS_CODE(
(function(){
    if (!window.initShellComm) {
        window.initShellComm = function() {
            console.log("[main.cpp injection] initShellComm() default called");
        };
    }
})();
)JS_CODE";

// -----------------------------------------------------------------------------
// Single-instance
// -----------------------------------------------------------------------------
static bool FocusExistingInstance(const std::wstring &protocolArg)
{
    // Use the actual window class name variable instead of reinterpret_cast(APP_NAME)
    HWND hExistingWnd = FindWindowW(APP_CLASS, nullptr);
    if (hExistingWnd) {
        if (IsIconic(hExistingWnd)) {
            ShowWindow(hExistingWnd, SW_RESTORE);
        } else if (!IsWindowVisible(hExistingWnd)) {
            ShowWindow(hExistingWnd, SW_SHOW);
        }
        SetForegroundWindow(hExistingWnd);
        SetFocus(hExistingWnd);
        // Send protocolArg if available
        if (!protocolArg.empty()) {
            COPYDATASTRUCT cds;
            cds.dwData = 1; // Custom identifier
            cds.cbData = static_cast<DWORD>(protocolArg.size() * sizeof(wchar_t));
            cds.lpData = (PVOID)protocolArg.c_str();
            SendMessage(hExistingWnd, WM_COPYDATA, 0, (LPARAM)&cds);
        }
        return true;
    }
    return false;
}


static bool CheckSingleInstance(int argc, char* argv[])
{
    g_hMutex = CreateMutexW(nullptr, FALSE, L"SingleInstanceMtx_StremioWebShell");
    if(!g_hMutex){
        std::wcerr << L"CreateMutex failed => fallback to multi.\n";
        AppendToCrashLog("CreateMutex failed => fallback to multi.");
        return true;
    }

    std::wstring protocolArg;
    for (int i = 1; i < argc; ++i) {
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, NULL, 0);
        if (size_needed > 0) {
            std::wstring argW(size_needed - 1, 0);
            MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, &argW[0], size_needed);

            // Check for protocol or file argument
            if (argW.rfind(L"stremio://", 0) == 0 || argW.rfind(L"magnet:", 0) == 0 || FileExists(argW) ) {
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


// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

inline std::string WStringToUtf8(const std::wstring &wstr)
{
    if (wstr.empty()) {
        return {};
    }
    // Determine required size (in bytes, including null terminator).
    int neededSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.data(),
        (int)wstr.size(),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (neededSize <= 0) {
        return {};
    }

    // Allocate and perform the conversion.
    std::string result(neededSize, '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.data(),
        (int)wstr.size(),
        &result[0],
        neededSize,
        nullptr,
        nullptr
    );

    // Remove any trailing null if present.
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

std::wstring Utf8ToWstring(const std::string& utf8Str) {
    if (utf8Str.empty()) {
        return std::wstring();
    }

    // Get the required buffer size for the conversion.
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), NULL, 0);
    if (size_needed == 0) {
        // Handle error if needed.
        return std::wstring();
    }

    // Convert and store into a wstring.
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), &wstr[0], size_needed);

    return wstr;
}

// Helper for nicer error formatting mpv
std::string capitalizeFirstLetter(const std::string& input) {
    if (input.empty()) return input;
    std::string result = input;
    result[0] = std::toupper(result[0]);
    return result;
}

std::wstring HResultToErrorMessage(HRESULT hr)
{
    LPWSTR buffer = nullptr;

    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS;

    DWORD size = FormatMessageW(
        flags,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr
    );

    if (size == 0)
    {
        return L"Unknown error (HRESULT=0x" + std::to_wstring(hr) + L")";
    }

    std::wstring message(buffer);

    LocalFree(buffer);

    while (!message.empty() &&
           (message.back() == L'\r' || message.back() == L'\n' || message.back() == L' '))
    {
        message.pop_back();
    }

    return message;
}

bool FileExists(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES &&
            !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool DirectoryExists(const std::wstring& dirPath) {
    DWORD attributes = GetFileAttributesW(dirPath.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

// -----------------------------------------------------------------------------
// Dark/Light theme
// -----------------------------------------------------------------------------
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

static void UpdateTheme(HWND hWnd) {
    if (g_useDarkTheme) {
        DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
        DwmSetWindowAttribute(hWnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    } else {
        BOOL dark = FALSE;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    }
}

static void LoadCustomMenuFont()
{
    if (g_hMenuFont) {
        DeleteObject(g_hMenuFont);
        g_hMenuFont = nullptr;
    }

    LOGFONTW lf = { 0 };
    lf.lfHeight = -12;
    lf.lfWeight = FW_MEDIUM;
    wcscpy_s(lf.lfFaceName, L"Arial Rounded MT");
    lf.lfQuality = CLEARTYPE_QUALITY;

    g_hMenuFont = CreateFontIndirectW(&lf);

    // Fallback to system menu font if the custom font is not available
    if (!g_hMenuFont) {
        NONCLIENTMETRICSW ncm = { sizeof(ncm) };
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        {
            ncm.lfMenuFont.lfQuality = CLEARTYPE_QUALITY;
            g_hMenuFont = CreateFontIndirectW(&(ncm.lfMenuFont));
        }
    }

    if (!g_hMenuFont) {
        std::cerr << "Failed to load custom menu font.\n";
        AppendToCrashLog("[FONT]: Failed to load custom menu font");
    }
}

// -----------------------------------------------------------------------------
// Send JSON to front-end
// -----------------------------------------------------------------------------
static void SendToJS(const json& j)
{
    if(!g_isAppReady) {
        // If WebView is not ready, queue the message
        g_pendingMessages.push_back(j);
        return;
    }
    std::string payload=j.dump();
    std::wstring wpayload(payload.begin(),payload.end());
    g_webview->PostWebMessageAsJson(wpayload.c_str());
    #ifdef DEBUG_BUILD
    std::cout<<"[Native->JS] "<<payload<<"\n";
    #endif
}


// -----------------------------------------------------------------------------
// mpv_node => JSON
// -----------------------------------------------------------------------------
static json mpvNodeArrayToJson(const mpv_node_list* list)
{
    json j=json::array();
    if(!list)return j;
    for(int i=0;i<list->num;i++){
        j.push_back(mpvNodeToJson(&list->values[i]));
    }
    return j;
}
static json mpvNodeMapToJson(const mpv_node_list* list)
{
    json j=json::object();
    if(!list)return j;
    for(int i=0;i<list->num;i++){
        const char* key = (list->keys&&list->keys[i])? list->keys[i] : "";
        mpv_node &val   = list->values[i];
        j[key]=mpvNodeToJson(&val);
    }
    return j;
}
static json mpvNodeToJson(const mpv_node* node)
{
    if(!node)return nullptr;

    switch(node->format)
    {
    case MPV_FORMAT_STRING:
        if(node->u.string) return json(node->u.string);
        else return json("");
    case MPV_FORMAT_INT64:
        return json((long long)node->u.int64);
    case MPV_FORMAT_DOUBLE:
        return json(node->u.double_);
    case MPV_FORMAT_FLAG:
        return json((bool)node->u.flag);
    case MPV_FORMAT_NODE_ARRAY:
        return mpvNodeArrayToJson(node->u.list);
    case MPV_FORMAT_NODE_MAP:
        return mpvNodeMapToJson(node->u.list);
    default:
        return "<unhandled mpv_node format>";
    }
}


// -----------------------------------------------------------------------------
// mpv wakeup callback
// -----------------------------------------------------------------------------
static void MpvWakeup(void *ctx)
{
    PostMessage((HWND)ctx, WM_MPV_WAKEUP, 0, 0);
}

// handle mpv events
static void HandleMpvEvents()
{
    if(!g_mpv)return;
    while(true){
        mpv_event* ev=mpv_wait_event(g_mpv,0);
        if(!ev||ev->event_id==MPV_EVENT_NONE)break;

        if(ev->error<0)
            std::cout<<"mpv event error="<<mpv_error_string(ev->error)<<"\n";

        switch(ev->event_id)
        {
        case MPV_EVENT_PROPERTY_CHANGE:
        {
            mpv_event_property* prop=(mpv_event_property*)ev->data;
            if(!prop||!prop->name)break;

            json j;
            j["type"] ="mpv-prop-change";
            j["id"]   =(int64_t)ev->reply_userdata;
            j["name"] = prop->name;
            if(ev->error<0)
                j["error"]=mpv_error_string(ev->error);

            switch(prop->format)
            {
            case MPV_FORMAT_INT64:
                if(prop->data)
                    j["data"]=(long long)(*(int64_t*)prop->data);
                else
                    j["data"]=nullptr;
                break;
            case MPV_FORMAT_DOUBLE:
                if(prop->data)
                    j["data"]=*(double*)prop->data;
                else
                    j["data"]=nullptr;
                break;
            case MPV_FORMAT_FLAG:
                if(prop->data)
                    j["data"]=(*(int*)prop->data!=0);
                else
                    j["data"]=false;
                break;
            case MPV_FORMAT_STRING:
                if(prop->data){
                    const char*s=*(char**)prop->data;
                    j["data"]=(s? s:"");
                } else {
                    j["data"]="";
                }
                break;
            case MPV_FORMAT_NODE:
                j["data"]=mpvNodeToJson((mpv_node*)prop->data);
                break;
            default:
                j["data"]=nullptr;
                break;
            }
            SendToJS(j);
            break;
        }
        case MPV_EVENT_END_FILE:
        {
            mpv_event_end_file* ef=(mpv_event_end_file*)ev->data;
            json j;
            j["type"]="mpv-event-ended";
            switch(ef->reason){
            case MPV_END_FILE_REASON_EOF:
                j["reason"]="quit";
                SendToJS(j);
                break;
            case MPV_END_FILE_REASON_ERROR: {
                std::string errorString = mpv_error_string(ef->error);
                std::string capitalizedErrorString = capitalizeFirstLetter(errorString);
                j["reason"]="error";
                if(ef->error<0)
                    j["error"]= capitalizedErrorString;
                SendToJS(j);
                AppendToCrashLog("[MPV]: " + capitalizedErrorString);
                break;
            }
            default:
                j["reason"]="other";
                break;
            }
            break;
        }
        case MPV_EVENT_SHUTDOWN:
        {
            std::cout<<"mpv EVENT_SHUTDOWN => terminate\n";
            mpv_terminate_destroy(g_mpv);
            g_mpv=nullptr;
            break;
        }
        default:
            // ignore
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// Inbound Mpv commands
// -----------------------------------------------------------------------------
static void HandleMpvCommand(const std::vector<std::string>& args)
{
    // e.g. ["loadfile","video.mp4"]
    if(!g_mpv||args.empty())return;
    // optional fix booleans => "true"/"false" => "yes"/"no"
    std::vector<const char*> cargs;
    for(auto &s: args)cargs.push_back(s.c_str());
    cargs.push_back(nullptr);
    mpv_command(g_mpv,cargs.data());
}
static void HandleMpvSetProp(const std::vector<std::string>& args)
{
    if(!g_mpv||args.size()<2)return;
    std::string val=args[1];
    if(val=="true") val="yes";
    if(val=="false")val="no";
    mpv_set_property_string(g_mpv, args[0].c_str(), val.c_str());
}
static void HandleMpvObserveProp(const std::vector<std::string>& args)
{
    if(!g_mpv||args.empty())return;
    std::string pname=args[0];
    g_observedProps.insert(pname);
    mpv_observe_property(g_mpv,0,pname.c_str(),MPV_FORMAT_NODE);
    std::cout<<"Observing prop="<<pname<<"\n";
}

// -----------------------------------------------------------------------------
// Mpv Init + Portable config, demux, caching
// -----------------------------------------------------------------------------
static std::wstring GetExeDirectory()
{
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr,buf,MAX_PATH);
    std::wstring path(buf);
    size_t pos=path.find_last_of(L"\\/");
    if(pos!=std::wstring::npos)
        path.erase(pos);
    return path;
}

static std::wstring GetIniPath()
{
    // e.g. place it in "portable_config/settings.ini"
    std::wstring exeDir = GetExeDirectory();
    std::wstring pcDir  = exeDir + L"\\portable_config";
    CreateDirectoryW(pcDir.c_str(), nullptr);  // ensure it exists
    return pcDir + L"\\stremio-settings.ini";
}

void pauseMPV(const bool allowed) {
    if (!allowed) return;
    std::vector<std::string> pauseArgs = {
        "pause",
        "true",
    };
    HandleMpvSetProp(pauseArgs);
}

// mpv init
bool InitMPV(HWND hwnd)
{
    g_mpv=mpv_create();
    if(!g_mpv){
        std::cerr<<"mpv_create failed\n";
        AppendToCrashLog("[MPV]: Create failed");
        return false;
    }

    // portable config
    std::wstring exeDir=GetExeDirectory();
    std::wstring cfg=exeDir+L"\\portable_config";
    CreateDirectoryW(cfg.c_str(),nullptr);

    // convert wide => utf8
    int needed=WideCharToMultiByte(CP_UTF8,0,cfg.c_str(),-1,nullptr,0,nullptr,nullptr);
    std::string utf8(needed,0);
    WideCharToMultiByte(CP_UTF8,0,cfg.c_str(),-1,&utf8[0],needed,nullptr,nullptr);

    mpv_set_option_string(g_mpv,"config-dir",utf8.c_str());
    mpv_set_option_string(g_mpv,"load-scripts","yes");
    mpv_set_option_string(g_mpv,"config","yes");
    mpv_set_option_string(g_mpv,"terminal","yes");
    mpv_set_option_string(g_mpv,"msg-level","all=v");

    // wid embedding
    int64_t wid=(int64_t)hwnd;
    mpv_set_option(g_mpv,"wid",MPV_FORMAT_INT64,&wid);

    mpv_set_wakeup_callback(g_mpv, MpvWakeup, hwnd);

    if(mpv_initialize(g_mpv)<0){
        std::cerr<<"mpv_initialize failed\n";
        AppendToCrashLog("[MPV]: Initialize failed");
        return false;
    }

    // Set VO
    mpv_set_option_string(g_mpv,"vo","gpu-next");

    //Some sub settings
    mpv_set_property_string(g_mpv, "sub-blur", "20");

    // demux/caching
    mpv_set_property_string(g_mpv,"demuxer-lavf-probesize",     "524288");
    mpv_set_property_string(g_mpv,"demuxer-lavf-analyzeduration","0.5");
    mpv_set_property_string(g_mpv,"demuxer-max-bytes","300000000");
    mpv_set_property_string(g_mpv,"demuxer-max-packets","150000000");
    mpv_set_property_string(g_mpv,"cache","yes");
    mpv_set_property_string(g_mpv,"cache-pause","no");
    mpv_set_property_string(g_mpv,"cache-secs","60");
    mpv_set_property_string(g_mpv,"vd-lavc-threads","0");
    mpv_set_property_string(g_mpv,"ad-lavc-threads","0");
    mpv_set_property_string(g_mpv,"audio-fallback-to-null","yes");
    mpv_set_property_string(g_mpv,"audio-client-name",APP_NAME);
    mpv_set_property_string(g_mpv,"title",APP_NAME);

    return true;
}

void CleanupMPV()
{
    if(g_mpv){
        mpv_terminate_destroy(g_mpv);
        g_mpv=nullptr;
    }
}

// -----------------------------------------------------------------------------
// Inbound event handling
// -----------------------------------------------------------------------------
static void ToggleFullScreen(HWND hWnd, bool enable)
{
    static WINDOWPLACEMENT prevPlc={sizeof(prevPlc)};
    if(enable==g_isFullscreen)return;
    g_isFullscreen=enable;

    if(enable){
        // store old
        GetWindowPlacement(hWnd,&prevPlc);
        MONITORINFO mi={sizeof(mi)};
        if(GetMonitorInfoW(MonitorFromWindow(hWnd,MONITOR_DEFAULTTOPRIMARY),&mi)){
            SetWindowLongW(hWnd,GWL_STYLE,WS_POPUP|WS_VISIBLE);
            SetWindowPos(hWnd,HWND_TOP,
                         mi.rcMonitor.left,mi.rcMonitor.top,
                         mi.rcMonitor.right-mi.rcMonitor.left,
                         mi.rcMonitor.bottom-mi.rcMonitor.top,
                         SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        }
    } else {
        // restore
        SetWindowLongW(hWnd,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);
        SetWindowPlacement(hWnd,&prevPlc);
        SetWindowPos(hWnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
    }
}

// app-ready inbound event
static void AppStart()
{
    std::cout<<"App initialize.\n";
    json j;
    j["type"] ="shellVersion";
    j["value"]   =APP_VERSION;
    SendToJS(j);
    HideSplash();

    for(const auto& pendingMsg : g_pendingMessages) {
        SendToJS(pendingMsg);
    }
    g_pendingMessages.clear();
}

// For local files
std::string decodeURIComponent(const std::string& encoded) {
    std::string result;
    result.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i) {
        char c = encoded[i];
        if (c == '%' && i + 2 < encoded.size() &&
            std::isxdigit(static_cast<unsigned char>(encoded[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(encoded[i + 2]))) {
            // Convert the two hex digits to a character
            std::string hex = encoded.substr(i + 1, 2);
            char decodedChar = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result.push_back(decodedChar);
            i += 2;
            } else {
                result.push_back(c);
            }
    }
    return result;
}

// parse inbound Events JSON
static void HandleInboundJSON(const std::string &msg)
{
    try
    {

        #ifdef DEBUG_BUILD
        std::cout<<"[JS->NATIVE] "<<msg<<"\n";
        #endif

        auto j=json::parse(msg);
        if(!j.contains("event")){
            std::cout<<"No 'event' field.\n";
            return;
        }
        std::string ev=j["event"].get<std::string>();

        // else mpv commands
        std::vector<std::string> argVec;
        if(j.contains("args")){
            if(j["args"].is_string()){
                argVec.push_back(j["args"].get<std::string>());
            } else if(j["args"].is_array()){
                for(auto& x: j["args"]){
                    if(x.is_string()) argVec.push_back(x.get<std::string>());
                    else argVec.push_back(x.dump());
                }
            }
        }

        if(ev=="mpv-command"){
            if(!argVec.empty() && argVec[0] == "loadfile" && argVec.size() > 1) {
                argVec[1] = decodeURIComponent(argVec[1]);
            }
            HandleMpvCommand(argVec);
        } else if(ev=="mpv-set-prop"){
            HandleMpvSetProp(argVec);
        } else if(ev=="mpv-observe-prop"){
            HandleMpvObserveProp(argVec);
        } else if (ev=="app-ready") {
            g_isAppReady = true;
            AppStart();
        } else if (ev=="update-requested") {
            RunInstallerAndExit();
        } else if (ev=="start-drag") {
            ReleaseCapture();
            SendMessageW(GetMainHwnd(), WM_NCLBUTTONDOWN, HTCAPTION, 0);
        } else if(ev == "seek-hover") {
            if (g_thumbFastHeight == 0) return;
            if(g_ignoreHover) {
                auto now = std::chrono::steady_clock::now();
                if(now < g_ignoreUntil) {
                    return;
                }
                g_ignoreHover = false;
            }

            // Expecting arguments: hovered_seconds, x, y
            if(argVec.size() < 3) {
                std::cerr << "seek-hover requires at least 3 arguments.\n";
                return;
            }

            // Convert the y-coordinate from string to an integer
            int yCoord = 0;
            try {
                yCoord = std::stoi(argVec[2]);
            } catch(const std::exception &e) {
                std::cerr << "Error converting y coordinate: " << e.what() << "\n";
                return;
            }

            // Subtract the thumb fast height from y
            int adjustedY = yCoord - g_thumbFastHeight;

            // Prepare command for thumbfast with adjusted y-coordinate
            std::vector<std::string> cmdArgs = {
                "script-message-to",
                "thumbfast",
                "thumb",
                argVec[0],              // hovered_seconds
                argVec[1],              // x
                std::to_string(adjustedY)  // y with offset
            };

            HandleMpvCommand(cmdArgs);
        }
        else if(ev == "seek-leave") {
            if (g_thumbFastHeight == 0) return;
            // Set ignore flag and calculate ignore-until timestamp
            g_ignoreHover = true;
            g_ignoreUntil = std::chrono::steady_clock::now() + IGNORE_DURATION;

            std::vector<std::string> cmdArgs = {
                "script-message-to",
                "thumbfast",
                "clear"
            };
            HandleMpvCommand(cmdArgs);
        } else {
            std::cout<<"Unknown event="<<ev<<"\n";
        }
    }
    catch(std::exception &ex){
        std::cerr<<"JSON parse error:"<<ex.what()<<"\n";
    }
}


// -----------------------------------------------------------------------------
// Stremio Settings Ini
// -----------------------------------------------------------------------------
static void LoadSettings() {
    std::wstring iniPath = GetIniPath();
    wchar_t buffer[8];

    // Load CloseOnExit
    GetPrivateProfileStringW(
        L"General",
        L"CloseOnExit",
        L"0",
        buffer,
        _countof(buffer),
        iniPath.c_str()
    );
    g_closeOnExit = (wcscmp(buffer, L"1") == 0);

    // Load UseDarkTheme
    GetPrivateProfileStringW(
        L"General",
        L"UseDarkTheme",
        L"1",
        buffer,
        _countof(buffer),
        iniPath.c_str()
    );
    g_useDarkTheme = (wcscmp(buffer, L"1") == 0);

    // Load ThumbFastHeight
    g_thumbFastHeight = GetPrivateProfileIntW(
        L"General",
        L"ThumbFastHeight",
        0,
        iniPath.c_str()
    );

    g_allowZoom = GetPrivateProfileIntW(
    L"General",
    L"AllowZoom",
    0,
    iniPath.c_str()
    );

    g_pauseOnMinimize = GetPrivateProfileIntW(
        L"General",
        L"PauseOnMinimize",
        1,
        iniPath.c_str()
    );

    g_pauseOnLostFocus = GetPrivateProfileIntW(
        L"General",
        L"PauseOnLostFocus",
        0,
        iniPath.c_str()
    );
}

static void SaveSettings() {
    std::wstring iniPath = GetIniPath();
    const wchar_t* closeVal = g_closeOnExit ? L"1" : L"0";
    const wchar_t* darkVal  = g_useDarkTheme ? L"1" : L"0";
    const wchar_t* pauseMinimizeVal  = g_pauseOnMinimize ? L"1" : L"0";
    const wchar_t* pauseFocusVal  = g_pauseOnLostFocus ? L"1" : L"0";
    const wchar_t* allowZoomVal  = g_allowZoom ? L"1" : L"0";

    WritePrivateProfileStringW(L"General", L"CloseOnExit", closeVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"UseDarkTheme", darkVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"PauseOnMinimize", pauseMinimizeVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"PauseOnLostFocus", pauseFocusVal, iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"AllowZoom", allowZoomVal, iniPath.c_str());
}


// -----------------------------------------------------------------------------
// Tray
// -----------------------------------------------------------------------------
// Create a borderless popup window for the tray menu
static HWND CreateDarkTrayMenuWindow()
{
    static bool s_classRegistered = false;
    if (!s_classRegistered)
    {
        WNDCLASSEXW wcex = { sizeof(wcex) };
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = DarkTrayMenuProc;
        wcex.hInstance     = GetModuleHandle(nullptr);
        wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = nullptr;
        wcex.lpszClassName = L"DarkTrayMenuWnd";
        RegisterClassExW(&wcex);
        s_classRegistered = true;
    }

    HWND hMenuWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"DarkTrayMenuWnd",
        L"",
        WS_POPUP,
        0, 0, 200, 200,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    g_trayHwnd = hMenuWnd;

    return hMenuWnd;
}

// create round corners
static void CreateRoundedRegion(HWND hWnd, int w, int h, int radius)
{
    HRGN hrgn = CreateRoundRectRgn(0, 0, w, h, radius, radius);
    SetWindowRgn(hWnd, hrgn, TRUE);
}

static void ShowDarkTrayMenu()
{
    // Fill vector with your items
    g_menuItems.clear();
    g_menuItems.push_back({ ID_TRAY_SHOWWINDOW,   g_showWindow,   false, L"Show Window" });
    g_menuItems.push_back({ ID_TRAY_ALWAYSONTOP,  g_alwaysOnTop,  false, L"Always on Top" });
    g_menuItems.push_back({ ID_TRAY_PICTURE_IN_PICTURE, g_isPipMode, false, L"Picture in Picture" });
    g_menuItems.push_back({ ID_TRAY_PAUSE_MINIMIZED, g_pauseOnMinimize, false, L"Pause Minimized" });
    g_menuItems.push_back({ ID_TRAY_PAUSE_FOCUS_LOST, g_pauseOnLostFocus, false, L"Pause Unfocused" });
    g_menuItems.push_back({ ID_TRAY_CLOSE_ON_EXIT,g_closeOnExit,  false, L"Close on Exit" });
    g_menuItems.push_back({ ID_TRAY_USE_DARK_THEME,g_useDarkTheme, false, L"Use Dark Theme" });
    g_menuItems.push_back({ 0, false, true,       L"" }); // separator
    g_menuItems.push_back({ ID_TRAY_QUIT, false, false,  L"Quit" });

    HWND hMenuWnd = CreateDarkTrayMenuWindow();

    // Measure total height
    int itemH = g_tray_itemH;
    int sepH  = g_tray_sepH;
    int w     = g_tray_w;
    int totalH = 0;
    for (auto &it: g_menuItems) {
        totalH += it.separator ? sepH : itemH;
    }

    // Get cursor position
    POINT cursor;
    GetCursorPos(&cursor);

    // Default position: bottom-left corner of menu at cursor position
    int posX = cursor.x;
    int posY = cursor.y - totalH;

    // Retrieve screen dimensions
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Adjust horizontal position:
    // If menu would go off right edge, reposition to the left of the cursor
    if (posX + w > screenWidth) {
        posX = cursor.x - w;
    }

    // If repositioning off left edge, clamp to screen left
    if (posX < 0) {
        posX = 0;
    }

    // Ensure vertical position is within screen bounds
    if(posY < 0) posY = 0;
    if(posY + totalH > screenHeight) posY = screenHeight - totalH;

    SetCapture(hMenuWnd);
    // Set window position and size
    SetWindowPos(hMenuWnd, HWND_TOPMOST, posX, posY, w, totalH, SWP_SHOWWINDOW);

    CreateRoundedRegion(hMenuWnd, w, totalH, 10);
    ShowWindow(hMenuWnd, SW_SHOW);
    UpdateWindow(hMenuWnd);
    SetForegroundWindow(hMenuWnd);
    SetFocus(hMenuWnd);
}

static LRESULT CALLBACK DarkTrayMenuProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    // Force close if user clicks away
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            DestroyWindow(hWnd);
        }
        break;

    case WM_KILLFOCUS:
    case WM_CAPTURECHANGED:
        DestroyWindow(hWnd);
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        // Decide background colors depending on g_useDarkTheme
        COLORREF bgBase, bgHover, txtNormal, txtCheck, lineColor;
        if (g_useDarkTheme)
        {
            bgBase   = RGB(30,30,30);
            bgHover  = RGB(50,50,50);
            txtNormal= RGB(200,200,200);
            txtCheck = RGB(200,200,200);
            lineColor= RGB(80,80,80);
        }
        else
        {
            bgBase   = RGB(240,240,240);
            bgHover  = RGB(200,200,200);
            txtNormal= RGB(0,0,0);
            txtCheck = RGB(0,0,0);
            lineColor= RGB(160,160,160);
        }

        // Fill entire background
        HBRUSH bgBrush = CreateSolidBrush(bgBase);
        FillRect(hdc, &rcClient, bgBrush);
        DeleteObject(bgBrush);

        // item metrics
        int y = 0;
        int itemH = g_tray_itemH;
        int sepH  = g_tray_sepH;

        // For each menu item
        for (int i=0; i<(int)g_menuItems.size(); i++)
        {
            auto &it = g_menuItems[i];
            if (it.separator)
            {
                // draw a line
                int midY = y + sepH/2;
                HPEN oldPen = (HPEN)SelectObject(hdc, CreatePen(PS_SOLID,1, lineColor));
                MoveToEx(hdc, 5, midY, nullptr);
                LineTo(hdc, rcClient.right-5, midY);
                DeleteObject(SelectObject(hdc, oldPen));
                y += sepH;
            }
            else
            {
                bool hovered = (i == g_hoverIndex);
                // Fill item background
                RECT itemRc = {0, y, rcClient.right, y+itemH};
                COLORREF itemColor = hovered ? bgHover : bgBase;
                HBRUSH itemBg = CreateSolidBrush(itemColor);
                FillRect(hdc, &itemRc, itemBg);
                DeleteObject(itemBg);

                // Check area (16 px)
                bool checked = it.checked;
                RECT cbox = { 4, y, 4+16, y+itemH };
                if (checked)
                {
                    // draw check glyph => “\u2714” or “\u2713”
                    SetTextColor(hdc, txtCheck);
                    SetBkMode(hdc, TRANSPARENT);
                    DrawTextW(hdc, L"\u2713", -1, &cbox, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
                }

                // draw label
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, txtNormal);

                // select custom font
                HFONT oldFnt = (HFONT)SelectObject(hdc, g_hMenuFont);

                RECT textRc = { 24, y, rcClient.right-5, y+itemH };
                DrawTextW(hdc, it.text.c_str(), -1, &textRc, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

                SelectObject(hdc, oldFnt);

                y += itemH;
            }
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int itemH = g_tray_itemH, sepH = g_tray_sepH;
        int curY = 0, hover = -1;

        for(int i = 0; i < (int)g_menuItems.size(); i++)
        {
            auto &it = g_menuItems[i];
            int h = it.separator ? sepH : itemH;
            if(!it.separator && yPos >= curY && yPos < (curY + h))
            {
                hover = i;
                break;
            }
            curY += h;
        }
        if(hover != g_hoverIndex)
        {
            g_hoverIndex = hover;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        // Change cursor to pointer if hovering over a clickable item, otherwise default arrow
        if(g_hoverIndex != -1) {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
        } else {
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        // Get current mouse position in screen coordinates
        POINT pt;
        GetCursorPos(&pt);

        // Get the window's rectangle in screen coordinates
        RECT rc;
        GetWindowRect(hWnd, &rc);

        // Determine if the mouse is inside the window rectangle
        bool inside = PtInRect(&rc, pt);

        // Unhook the low-level mouse hook to prevent interference
        if (g_hMouseHook) {
            UnhookWindowsHookEx(g_hMouseHook);
            g_hMouseHook = nullptr;
        }

        // Only process a selection if the click was inside the menu
        if (inside && g_hoverIndex >= 0 && g_hoverIndex < (int)g_menuItems.size())
        {
            auto &it = g_menuItems[g_hoverIndex];
            if (!it.separator)
            {
                PostMessage(GetMainHwnd(), WM_COMMAND, it.id, 0);
            }
        }
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY:
        g_hoverIndex = -1;
        if (g_hMouseHook) {
            UnhookWindowsHookEx(g_hMouseHook);
            g_hMouseHook = nullptr;
        }
        ReleaseCapture();
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void ShowTrayMenu(HWND /*hWnd*/)
{
    ShowDarkTrayMenu();
}

static void CreateTrayIcon(HWND hWnd)
{
    g_nid.cbSize=sizeof(NOTIFYICONDATA);
    g_nid.hWnd=hWnd;
    g_nid.uID=1;
    g_nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
    g_nid.uCallbackMessage=WM_TRAYICON;

    HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
    g_nid.hIcon = hIcon;

    _tcscpy_s(g_nid.szTip,_T("Stremio SingleInstance"));

    Shell_NotifyIcon(NIM_ADD,&g_nid);
}

static void RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE,&g_nid);
    if(g_nid.hIcon){
        DestroyIcon(g_nid.hIcon);
        g_nid.hIcon=nullptr;
    }
}


// -----------------------------------------------------------------------------
// Splash Screen
// -----------------------------------------------------------------------------
void CreateSplashScreen(HWND parent)
{
    WNDCLASSEXW splashWcex = {0};
    splashWcex.cbSize = sizeof(WNDCLASSEXW);
    splashWcex.lpfnWndProc = SplashWndProc;
    splashWcex.hInstance = g_hInst;
    splashWcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    splashWcex.lpszClassName = L"SplashScreenClass";
    RegisterClassExW(&splashWcex);

    // Get client area of the main window
    RECT rcClient;
    GetClientRect(parent, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    // Create child window of the main window
    g_hSplash = CreateWindowExW(
        0,
        L"SplashScreenClass",
        nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0,
        width, height,
        parent,
        nullptr,
        g_hInst,
        nullptr
    );

    if(!g_hSplash) {
        DWORD errorCode = GetLastError();
        std::string errorMessage = std::string("[SPLASH]: Failed to load custom menu font. Error=")
                           + std::to_string(errorCode);
        std::cerr << errorMessage << "\n";
        AppendToCrashLog(errorMessage);
        return;
    }

    // Load the PNG image resource.
    {
        HRSRC   hRes   = FindResource(g_hInst, MAKEINTRESOURCE(IDR_SPLASH_PNG), RT_RCDATA);
        if(!hRes) {
            std::cerr << "Could not find PNG resource.\n";
        } else {
            HGLOBAL hData  = LoadResource(g_hInst, hRes);
            DWORD   size   = SizeofResource(g_hInst, hRes);
            void*   pData  = LockResource(hData);
            if(!pData) {
                std::cerr << "LockResource returned null.\n";
            } else {
                // Create an IStream on this resource memory
                IStream* pStream = nullptr;
                if(CreateStreamOnHGlobal(nullptr, TRUE, &pStream) == S_OK)
                {
                    ULONG written = 0;
                    pStream->Write(pData, size, &written);
                    LARGE_INTEGER liZero = {};
                    pStream->Seek(liZero, STREAM_SEEK_SET, nullptr);

                    // Create GDI+ bitmap from the IStream
                    Gdiplus::Bitmap bitmap(pStream);
                    if(bitmap.GetLastStatus() == Gdiplus::Ok)
                    {
                        HBITMAP hBmp = NULL;
                        if(bitmap.GetHBITMAP(Gdiplus::Color(0,0,0,0), &hBmp) == Gdiplus::Ok) {
                            g_hSplashImage = hBmp;
                        } else {
                            std::cerr << "Failed to create HBITMAP from embedded PNG.\n";
                        }
                    } else {
                        std::cerr << "Failed to decode embedded PNG data.\n";
                    }
                    pStream->Release();
                }
            }
        }
    }

    // Animation update speed
    SetTimer(g_hSplash, 1, 4, nullptr);

    // Bring splash window to top
    SetWindowPos(
        g_hSplash,
        HWND_TOP,
        0, 0, width, height,
        SWP_SHOWWINDOW
    );

    // Force an initial repaint
    InvalidateRect(g_hSplash, nullptr, TRUE);
}

LRESULT CALLBACK SplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_TIMER:
        {
            // Use "g_splashSpeed" to scale the animation movement
            const float baseStep = 0.01f;
            const float splashSpeed = 1.1f;
            float actualStep = baseStep * splashSpeed;

            g_splashOpacity += actualStep * g_pulseDirection;

            if (g_splashOpacity <= 0.3f)
            {
                g_splashOpacity = 0.3f;
                g_pulseDirection = 1;
            }
            else if (g_splashOpacity >= 1.0f)
            {
                g_splashOpacity = 1.0f;
                g_pulseDirection = -1;
            }

            InvalidateRect(hWnd, nullptr, FALSE);
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);
            int winW = rc.right - rc.left;
            int winH = rc.bottom - rc.top;

            // Create an offscreen DC and bitmap the size of the window
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, winW, winH);
            HGDIOBJ oldMemBmp = SelectObject(memDC, memBmp);

            // Fill background in offscreen DC
            HBRUSH bgBrush = CreateSolidBrush(RGB(12, 11, 17)); // "#0c0b11"
            FillRect(memDC, &rc, bgBrush);
            DeleteObject(bgBrush);

            // Draw the PNG at current opacity
            if(g_hSplashImage)
            {
                BITMAP bm;
                GetObject(g_hSplashImage, sizeof(bm), &bm);

                int imgWidth = bm.bmWidth;
                int imgHeight = bm.bmHeight;
                int destX = (winW - imgWidth) / 2;
                int destY = (winH - imgHeight) / 2;

                // Create a DC for the image
                HDC imgDC = CreateCompatibleDC(memDC);
                HGDIOBJ oldImgBmp = SelectObject(imgDC, g_hSplashImage);

                BLENDFUNCTION blend = {};
                blend.BlendOp = AC_SRC_OVER;
                blend.SourceConstantAlpha = (BYTE)(g_splashOpacity * 255);
                blend.AlphaFormat = 0;
                blend.AlphaFormat = AC_SRC_ALPHA;

                // Create a temp DC/bitmap for alpha compositing
                HBITMAP tempBmp = CreateCompatibleBitmap(memDC, imgWidth, imgHeight);
                HDC tempDC = CreateCompatibleDC(memDC);
                HGDIOBJ oldTempBmp = SelectObject(tempDC, tempBmp);

                // Copy the image into tempDC
                BitBlt(tempDC, 0, 0, imgWidth, imgHeight, imgDC, 0, 0, SRCCOPY);

                // Blend from tempDC onto memDC
                AlphaBlend(memDC, destX, destY, imgWidth, imgHeight,
                           tempDC, 0, 0, imgWidth, imgHeight, blend);

                // Cleanup
                SelectObject(tempDC, oldTempBmp);
                DeleteObject(tempBmp);
                DeleteDC(tempDC);

                SelectObject(imgDC, oldImgBmp);
                DeleteDC(imgDC);
            }

            // Finally, blit the offscreen to the real device
            BitBlt(hdc, 0, 0, winW, winH, memDC, 0, 0, SRCCOPY);

            // Cleanup
            SelectObject(memDC, oldMemBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            break;
        }

        case WM_DESTROY:
            KillTimer(hWnd, 1);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void HideSplash()
{
    if (g_hSplash)
    {
        KillTimer(g_hSplash, 1);
        DestroyWindow(g_hSplash);
        g_hSplash = nullptr;
    }
    if (g_hSplashImage) {
        DeleteObject(g_hSplashImage);
        g_hSplashImage = nullptr;
    }
}


// -----------------------------------------------------------------------------
// PictureInPicture / PiP
// -----------------------------------------------------------------------------
void TogglePictureInPicture(HWND hWnd, bool enable) {
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    if (enable) {
        g_alwaysOnTop = true;
        style &= ~WS_CAPTION;
        SetWindowLong(hWnd, GWL_STYLE, style);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    } else {
        g_alwaysOnTop = false;
        style |= WS_CAPTION;
        SetWindowLong(hWnd, GWL_STYLE, style);
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    g_isPipMode = enable;

    if(g_webview) {
        if(enable) {
            json j;
            j["type"] ="showPictureInPicture";
            SendToJS(j);
        } else {
            json j;
            j["type"] ="hidePictureInPicture";
            SendToJS(j);
        }
    }
}


// -----------------------------------------------------------------------------
// Exception Handler
// -----------------------------------------------------------------------------
void Cleanup() {
    RemoveTrayIcon();
    CleanupMPV();
    StopNodeServer();

    if(g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
    }
}

static std::wstring GetDailyCrashLogPath()
{
    std::time_t t = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &t);

    std::wstringstream filename;
    filename << L"\\errors-"
             << localTime.tm_mday << L"."
             << (localTime.tm_mon + 1) << L"."
             << (localTime.tm_year + 1900) << L".txt";

    std::wstring exeDir = GetExeDirectory();
    std::wstring pcDir  = exeDir + L"\\portable_config";
    return pcDir + filename.str();
}

static void AppendToCrashLog(const std::wstring& message)
{
    std::wofstream logFile;
    logFile.open(GetDailyCrashLogPath(), std::ios::app);
    if(!logFile.is_open()) {
        return;
    }

    std::time_t t = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &t);

    // Write timestamp and message
    logFile << L"[" << std::put_time(&localTime, L"%H:%M:%S") << L"] "
            << message << std::endl;
}

static void AppendToCrashLog(const std::string& message)
{
    // Convert std::string to std::wstring and call the existing function
    std::wstring wideMessage = Utf8ToWstring(message);
    AppendToCrashLog(wideMessage);
}

LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo) {
    std::cout<<"WM_DESTROY_CRASH => stopping mpv + node + tray.\n";
    std::wstringstream ws;
    ws << L"Unhandled exception occurred! Exception code: 0x"
       << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode;
    AppendToCrashLog(ws.str());
    Cleanup();

    return EXCEPTION_EXECUTE_HANDLER;
}


// -----------------------------------------------------------------------------
// WebView2
// -----------------------------------------------------------------------------

static void SetupWebMessageHandler()
{
    if(!g_webview)return;
    EventRegistrationToken navToken;
    g_webview->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [](ICoreWebView2* snd, ICoreWebView2NavigationCompletedEventArgs* args)->HRESULT
            {
                snd->ExecuteScript(L"initShellComm();",nullptr);
                return S_OK;
            }
        ).Get(),
        &navToken
    );

    EventRegistrationToken contextMenuToken;
    g_webview->add_ContextMenuRequested(
        Callback<ICoreWebView2ContextMenuRequestedEventHandler>(
            [](ICoreWebView2* sender, ICoreWebView2ContextMenuRequestedEventArgs* args) -> HRESULT {
                wil::com_ptr<ICoreWebView2ContextMenuItemCollection> items;
                HRESULT hr = args->get_MenuItems(&items);
                if (FAILED(hr) || !items) {
                    return hr;
                }

                #ifdef DEBUG_BUILD
                return S_OK; //DEV TOOLS DEBUG ONLY
                #endif
                wil::com_ptr<ICoreWebView2ContextMenuTarget> target;
                hr = args->get_ContextMenuTarget(&target);
                BOOL isEditable = FALSE;
                if (SUCCEEDED(hr) && target) {
                    hr = target->get_IsEditable(&isEditable);
                }
                if (FAILED(hr)) {
                    return hr;
                }

                UINT count = 0;
                items->get_Count(&count);

                if (!isEditable) {
                    while(count > 0) {
                        wil::com_ptr<ICoreWebView2ContextMenuItem> item;
                        items->GetValueAtIndex(0, &item);
                        if(item) {
                            items->RemoveValueAtIndex(0);
                        }
                        items->get_Count(&count);
                    }
                    return S_OK;
                }

                // Define allowed command IDs for filtering
                std::set<INT32> allowedCommandIds = {
                    50151, // Cut
                    50150, // Copy
                    50152, // Paste
                    50157, // Paste as plain text
                    50156  // Select all
                };

                for (UINT i = 0; i < count; )
                {
                    wil::com_ptr<ICoreWebView2ContextMenuItem> item;
                    hr = items->GetValueAtIndex(i, &item);
                    if (FAILED(hr) || !item) {
                        ++i;
                        continue;
                    }

                    INT32 commandId = 0;
                    hr = item->get_CommandId(&commandId);
                    if (FAILED(hr)) {
                        ++i;
                        continue;
                    }

                    // If the commandId is not in the allowed list, remove the item
                    if (allowedCommandIds.find(commandId) == allowedCommandIds.end()) {
                        hr = items->RemoveValueAtIndex(i);
                        if (FAILED(hr)) {
                            std::wcerr << L"Failed to remove item at index " << i << std::endl;
                            return hr;
                        }
                        // After removal, the collection size reduces, so update count and don't increment i
                        items->get_Count(&count);
                        continue;
                    }
                    ++i;
                }
                return S_OK;
            }
        ).Get(),
        &contextMenuToken
    );


    EventRegistrationToken messageToken;
    g_webview->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [](ICoreWebView2* /*sender*/, ICoreWebView2WebMessageReceivedEventArgs* args)->HRESULT
            {
                wil::unique_cotaskmem_string msgRaw;
                args->TryGetWebMessageAsString(&msgRaw);
                if(!msgRaw)return S_OK;
                std::wstring wstr(msgRaw.get());
                std::string strUtf8 = WStringToUtf8(wstr);
                HandleInboundJSON(strUtf8);
                return S_OK;
            }
        ).Get(),&messageToken
    );

    EventRegistrationToken newWindowToken;
    g_webview->add_NewWindowRequested(
        Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [](ICoreWebView2* /*sender*/, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT
            {
                // Mark the event as handled to prevent default behavior
                args->put_Handled(TRUE);

                wil::unique_cotaskmem_string uri;
                if (SUCCEEDED(args->get_Uri(&uri)) && uri)
                {
                    std::wstring wuri(uri.get());
                    // Check if the URI is a local file (starts with "file://")
                    if (wuri.rfind(L"file://", 0) == 0)
                    {
                        std::wstring filePath = wuri.substr(8);
                        std::string utf8FilePath = WStringToUtf8(filePath);
                        json j;
                        j["type"] = "FileDropped";
                        j["path"] = utf8FilePath;
                        SendToJS(j);
                        return S_OK;
                    }

                    // For non-file URIs, open externally
                    ShellExecuteW(nullptr, L"open", uri.get(), nullptr, nullptr, SW_SHOWNORMAL);
                }
                return S_OK;
            }
        ).Get(),
        &newWindowToken
    );


    EventRegistrationToken cfeToken;
    g_webview->add_ContainsFullScreenElementChanged(
        Microsoft::WRL::Callback<ICoreWebView2ContainsFullScreenElementChangedEventHandler>(
            [](ICoreWebView2* sender, IUnknown* /*args*/) -> HRESULT
            {
                // FullScreen Toggle Handle
                BOOL inFull = FALSE;
                sender->get_ContainsFullScreenElement(&inFull);

                ToggleFullScreen(g_hWnd, inFull != FALSE);

                return S_OK;
            }
        ).Get(),
        &cfeToken
    );
}

std::vector<std::wstring> GetExtensionPaths(const std::wstring& extensionsRoot) {
    namespace fs = std::filesystem;
    std::vector<std::wstring> paths;
    try {
        for (const auto& entry : fs::directory_iterator(extensionsRoot)) {
            if (entry.is_directory()) {
                paths.push_back(entry.path().wstring());
                std::cout<<"PATH EXTENSION"<<entry.path()<<std::endl;
            }
        }
    } catch (const fs::filesystem_error&) {
        std::cout<<"[EXTENSIONS]: No Extensions Folder found in portable_config"<<std::endl;
    }
    return paths;
}

static void SetupExtensions() {
    if(!g_webview || !g_webviewProfile)return;
    // Construct extension root path
    std::wstring exeDir = GetExeDirectory();
    std::wstring extensionsRoot = exeDir + L"\\portable_config\\extensions";
    auto extensionPaths = GetExtensionPaths(extensionsRoot);

    // Add each extension
    for (const auto& extPath : extensionPaths) {
        HRESULT hr = g_webviewProfile->AddBrowserExtension(
            extPath.c_str(),
            Microsoft::WRL::Callback<ICoreWebView2ProfileAddBrowserExtensionCompletedHandler>(
                [extPath](HRESULT result, ICoreWebView2BrowserExtension* extension) -> HRESULT
                {
                    if (SUCCEEDED(result))
                    {
                        std::cout << "[EXTENSIONS]: Added Browser Extension: " << WStringToUtf8(extPath) << std::endl;
                    }
                    else
                    {
                        std::wstring error = L"[EXTENSIONS]: Failed to add Browser Extension: " + HResultToErrorMessage(result) + L" | " + extPath;
                        std::cout << WStringToUtf8(error) << std::endl;
                        AppendToCrashLog(error);
                    }
                    return S_OK;
                }
            ).Get()
        );

        if (FAILED(hr))
        {
            std::wstring error = L"[EXTENSIONS]: Failed Add Browser Extension Callback: " + HResultToErrorMessage(hr);
            std::cout << WStringToUtf8(error) << std::endl;
            AppendToCrashLog(error);
        }
    }
}

static ComPtr<ICoreWebView2EnvironmentOptions> setupEnvironment() {
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    ComPtr<ICoreWebView2EnvironmentOptions6> options6;
    if (options.As(&options6) == S_OK)
    {
        options6->put_AreBrowserExtensionsEnabled(TRUE);
    }
    ComPtr<ICoreWebView2EnvironmentOptions5> options5;
    if (options.As(&options5) == S_OK)
    {
        options5->put_EnableTrackingPrevention(TRUE);
    }
    return options;
}



static void InitWebView2(HWND hWnd)
{
    ComPtr<ICoreWebView2EnvironmentOptions> options = setupEnvironment();
    std::wstring exeDir = GetExeDirectory();
    std::wstring browserDir  = exeDir + L"\\portable_config" + L"\\EdgeWebView";

    const wchar_t* browserExecutableFolder = nullptr;
    if (DirectoryExists(browserDir)) {
        browserExecutableFolder = browserDir.c_str();
        std::wcout << L"[WEBVIEW]: Using Found Browser directory: " << browserDir << std::endl;
    } else {
        std::wcout << L"[WEBVIEW]: Browser directory does not exist, using default." << std::endl;
    }

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
            browserExecutableFolder,nullptr, options.Get(),
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [hWnd](HRESULT res, ICoreWebView2Environment* env)->HRESULT
                {
                    if(!env)return E_FAIL;
                    env->CreateCoreWebView2Controller(
                        hWnd,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [hWnd](HRESULT result, ICoreWebView2Controller* rawController)->HRESULT
                            {
                                if (FAILED(result) || !rawController) return E_FAIL;

                                wil::com_ptr<ICoreWebView2Controller> m_webviewController = rawController;
                                if (!m_webviewController) return E_FAIL;

                                g_webviewController = m_webviewController.try_query<ICoreWebView2Controller4>();
                                if (!g_webviewController) return E_FAIL;

                                wil::com_ptr<ICoreWebView2> coreWebView;
                                g_webviewController->get_CoreWebView2(&coreWebView);
                                g_webview = coreWebView.try_query<ICoreWebView2_21>();
                                if (!g_webview) return E_FAIL;

                                wil::com_ptr<ICoreWebView2Profile> webView2Profile;
                                g_webview->get_Profile(&webView2Profile);
                                g_webviewProfile = webView2Profile.try_query<ICoreWebView2Profile8>();
                                if (!g_webviewProfile) return E_FAIL;

                                wil::com_ptr<ICoreWebView2Settings> webView2Settings;
                                g_webview->get_Settings(&webView2Settings);
                                auto settings = webView2Settings.try_query<ICoreWebView2Settings8>();
                                if (!settings) return E_FAIL;


                                // Setup General Settings
                                #ifndef DEBUG_BUILD
                                settings->put_AreDevToolsEnabled(FALSE);
                                #endif
                                settings->put_IsStatusBarEnabled(FALSE);
                                std::wstring customUserAgent =  L"StremioShell/" + Utf8ToWstring(APP_VERSION);
                                settings->put_UserAgent(customUserAgent.c_str());
                                if (!g_allowZoom) {
                                    settings->put_IsZoomControlEnabled(FALSE);
                                    settings->put_IsPinchZoomEnabled(FALSE);
                                }

                                COREWEBVIEW2_COLOR col={0,0,0,0};
                                g_webviewController->put_DefaultBackgroundColor(col);

                                BOOL allowExternalDrop;
                                g_webviewController->get_AllowExternalDrop(&allowExternalDrop);

                                g_webviewController->put_AllowExternalDrop(TRUE);

                                RECT rc;GetClientRect(hWnd,&rc);
                                g_webviewController->put_Bounds(rc);

                                g_webview->AddScriptToExecuteOnDocumentCreated(INIT_SHELL_SCRIPT,nullptr);

                                SetupExtensions();
                                SetupWebMessageHandler();

                                g_webview->Navigate(g_webuiUrl.c_str());

                                return S_OK;
                            }
                        ).Get()
                    );

                    return S_OK;
                }
            ).Get()
        );

    if (FAILED(hr)) {
        std::wstring error = L"[WEBVIEW]: Failed to create Web View, make sure WebView2 runtime is installed or provide a portable WebView2 runtime exe in portable_config/EdgeWebView - Error: " + HResultToErrorMessage(hr);
        std::cout << WStringToUtf8(error) << std::endl;
        AppendToCrashLog(error);
        // Show error in a message box
        MessageBoxW(
            nullptr,
            error.c_str(),
            L"WebView2 Initialization Error",
            MB_ICONERROR | MB_OK
        );
        PostQuitMessage(1);
        exit(1);
    }
}


// -----------------------------------------------------------------------------
// Node server
// -----------------------------------------------------------------------------
void StopNodeServer()
{
    if(g_nodeRunning){
        g_nodeRunning=false;
        if(g_nodeProcess){
            TerminateProcess(g_nodeProcess,0);
            WaitForSingleObject(g_nodeProcess,INFINITE);
            CloseHandle(g_nodeProcess);g_nodeProcess=nullptr;
        }
        if(g_nodeThread.joinable())g_nodeThread.join();
        if(g_nodeOutPipe){CloseHandle(g_nodeOutPipe);g_nodeOutPipe=nullptr;}
        if(g_nodeInPipe){CloseHandle(g_nodeInPipe);g_nodeInPipe=nullptr;}
        std::cout<<"Node server stopped.\n";
    }
}

static void NodeOutputThreadProc()
{
    char buf[1024];
    DWORD readSz=0;
    while(g_nodeRunning){
        BOOL ok=ReadFile(g_nodeOutPipe,buf,sizeof(buf)-1,&readSz,nullptr);
        if(!ok||readSz==0) break;
        buf[readSz]='\0';
        std::cout<<"[node] "<<buf;
    }
    std::cout<<"NodeOutputThreadProc done.\n";
}

bool StartNodeServer()
{
    std::wstring exePath=L"stremio-runtime.exe";
    std::wstring scriptPath=L"server.js";
    if(!FileExists(exePath)){
        AppendToCrashLog(L"[NODE]: Missing stremio-runtime.exe");
        return false;
    }
    if(!FileExists(scriptPath)){
        AppendToCrashLog(L"[NODE]: Missing server.js");
        return false;
    }

    if (!g_serverJob) {
        g_serverJob = CreateJobObject(nullptr, nullptr);
        if (!g_serverJob) {
            std::cerr << "Failed to create Job Object.\n";
            AppendToCrashLog(L"[NODE]: Failed to create Job Object.");
            return false;
        }

        // Configure the job to kill all child processes when the job closes.
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {0};
        jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(g_serverJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo))) {
            std::cerr << "Failed to set Job Object limits.\n";
            CloseHandle(g_serverJob);
            g_serverJob = nullptr;
            return false;
        }
    }

    SECURITY_ATTRIBUTES sa;ZeroMemory(&sa,sizeof(sa));
    sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    HANDLE outR=nullptr,outW=nullptr;
    if(!CreatePipe(&outR,&outW,&sa,0)){
        AppendToCrashLog(L"[NODE]: CreatePipe fail1");
        return false;
    }
    SetHandleInformation(outR,HANDLE_FLAG_INHERIT,0);

    HANDLE inR=nullptr,inW=nullptr;
    if(!CreatePipe(&inR,&inW,&sa,0)){
        AppendToCrashLog(L"[NODE]: CreatePipe fail2");
        CloseHandle(outR);CloseHandle(outW);
        return false;
    }
    SetHandleInformation(inW,HANDLE_FLAG_INHERIT,0);

    STARTUPINFOW si;ZeroMemory(&si,sizeof(si));
    si.cb=sizeof(si);
    si.hStdOutput=outW;si.hStdError=outW;si.hStdInput=inR;
    si.dwFlags=STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;ZeroMemory(&pi,sizeof(pi));
    std::wstring cmdLine=L"\"stremio-runtime.exe\" \"server.js\"";

    SetEnvironmentVariableW(L"NO_CORS", L"1");
    BOOL success=CreateProcessW(nullptr,&cmdLine[0],nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi);
    CloseHandle(inR);CloseHandle(outW);
    if(!success){
        std::wstring err=L"Failed to launch stremio-runtime.exe\nGetLastError="+std::to_wstring(GetLastError());
        AppendToCrashLog(err);
        CloseHandle(inW);CloseHandle(outR);
        return false;
    }

    if (!AssignProcessToJobObject(g_serverJob, pi.hProcess)) {
        std::cerr << "Failed to assign process to Job Object.\n";
    }

    g_nodeProcess=pi.hProcess;
    CloseHandle(pi.hThread);

    g_nodeRunning=true;
    g_nodeOutPipe=outR;
    g_nodeInPipe =inW;
    g_nodeThread=std::thread(NodeOutputThreadProc);

    std::cout<<"Node server started.\n";
    return true;
}


// -----------------------------------------------------------------------------
// Openssl
// -----------------------------------------------------------------------------

// Helper to write data from CURL callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::string* s = reinterpret_cast<std::string*>(userp);
    s->append(reinterpret_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Download file content as string using CURL
static bool DownloadString(const std::string& url, std::string& outData) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outData);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

// Download file and save to path
static bool DownloadFile(const std::string& url, const std::filesystem::path& dest) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    FILE* fp = _wfopen(dest.c_str(), L"wb");
    if(!fp) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}

// Compute SHA256 checksum of a file
static std::string FileChecksum(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return "";
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    char buf[4096];
    while(file.read(buf, sizeof(buf))) {
        SHA256_Update(&ctx, buf, file.gcount());
    }
    if(file.gcount() > 0) {
        SHA256_Update(&ctx, buf, file.gcount());
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

// Verify signature
static bool VerifySignature(const std::string& data, const std::string& signatureBase64) {
    // Load public key from embedded PEM
    BIO* bio = BIO_new_mem_buf(public_key_pem, -1);
    EVP_PKEY* pubKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if(!pubKey) return false;

    // Remove whitespace from the Base64 signature
    std::string cleanedSig;
    for(char c : signatureBase64) {
        if(!isspace(static_cast<unsigned char>(c))) {
            cleanedSig.push_back(c);
        }
    }

    // Base64 decode signature
    BIO* b64 = BIO_new(BIO_f_base64());
    // Configure decoder to not expect newlines
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bmem = BIO_new_mem_buf(cleanedSig.data(), static_cast<int>(cleanedSig.size()));
    bmem = BIO_push(b64, bmem);

    std::vector<unsigned char> signature(512);
    int sig_len = BIO_read(bmem, signature.data(), static_cast<int>(signature.size()));
    BIO_free_all(bmem);

    if(sig_len <= 0) {
        EVP_PKEY_free(pubKey);
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_PKEY_CTX* pctx = nullptr;
    bool result = false;
    if(EVP_DigestVerifyInit(ctx, &pctx, EVP_sha256(), NULL, pubKey) == 1) {
        if(EVP_DigestVerifyUpdate(ctx, data.data(), data.size()) == 1) {
            result = EVP_DigestVerifyFinal(ctx, signature.data(), sig_len) == 1;
        }
    }
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pubKey);
    return result;
}


// -----------------------------------------------------------------------------
// Auto Updater
// -----------------------------------------------------------------------------
static void RunAutoUpdaterOnce() {
    g_updaterRunning = true;
    std::cout<<"Checking for Updates.\n";
    // Endpoint URLs
    const std::string versionUrl = g_updateUrl;

    std::string versionContent;
    if(!DownloadString(versionUrl, versionContent)) {
        AppendToCrashLog(L"[UPDATER]: Failed to download version.json");
        return;
    }

    // Parse version.json
    json versionJson;
    try { versionJson = json::parse(versionContent); } catch(...) { return; }

    std::string versionDescUrl = versionJson["versionDesc"].get<std::string>();
    std::string signatureBase64 = versionJson["signature"].get<std::string>();

    // Download version-details.json
    std::string detailsContent;
    if(!DownloadString(versionDescUrl, detailsContent)) {
        AppendToCrashLog(L"[UPDATER]:Failed to download version details");
        return;
    }

    // Verify signature
    if(!VerifySignature(detailsContent, signatureBase64)) {
        AppendToCrashLog(L"[UPDATER]:Signature verification failed");
        return;
    }

    // Parse version-details.json
    json detailsJson;
    try { detailsJson = json::parse(detailsContent); } catch(...) { return; }

    // Compare shellVersion
    std::string remoteShellVersion = detailsJson["shellVersion"].get<std::string>();
    bool needsFullUpdate = (remoteShellVersion != APP_VERSION);

    auto files = detailsJson["files"];
    std::vector<std::string> partialUpdateKeys = { "server.js" };

    std::wstring exeDir = GetExeDirectory();
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / L"stremio_updater";
    std::filesystem::create_directories(tempDir);


    // Handle full update scenario
    if(needsFullUpdate || g_autoupdaterForceFull) {
        bool allDownloadsSuccessful = true;

        std::string key = "windows";
        SYSTEM_INFO systemInfo;
        GetNativeSystemInfo(&systemInfo);

        if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
            key = "windows-x64";
        } else if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
            key = "windows-x86";
        } else {
            // Log error if architecture cannot be determined
            std::cerr << "Error: Unsupported processor architecture detected." << std::endl;
            AppendToCrashLog(L"[UPDATER]: Error: Unsupported processor architecture detected.");
            return;
        }

        std::cout<<"Processing full update for: " << key << "\n";

        if(files.contains(key) && files[key].contains("url") && files[key].contains("checksum")) {
            std::string url = files[key]["url"].get<std::string>();
            std::string expectedChecksum = files[key]["checksum"].get<std::string>();

            // Determine filename from URL
            std::string filename = url.substr(url.find_last_of('/') + 1);
            std::filesystem::path installerPath = tempDir / std::wstring(filename.begin(), filename.end());

            std::cout<<"Downloading full update for: " << key << "\n";
            // Check if file already exists with correct checksum
            if(std::filesystem::exists(installerPath)) {
                if(FileChecksum(installerPath) != expectedChecksum) {
                    // File exists but checksum mismatches (possibly incomplete or corrupted)
                    // Remove the faulty file before re-downloading
                    std::filesystem::remove(installerPath);

                    // Attempt re-download after removal
                    if(!DownloadFile(url, installerPath)) {
                        AppendToCrashLog(L"[UPDATER]: Failed to re-download installer");
                        allDownloadsSuccessful = false;
                    }
                }
            } else {
                // File doesn't exist; download
                if(!DownloadFile(url, installerPath)) {
                    AppendToCrashLog(L"[UPDATER]: Failed to download installer");
                    allDownloadsSuccessful = false;
                }
            }

            if(FileChecksum(installerPath) != expectedChecksum) {
                AppendToCrashLog("[UPDATER]: Failed to download. File corrupted: " + installerPath.string());
                allDownloadsSuccessful = false;
            }

            if(allDownloadsSuccessful) {
                g_installerPath = installerPath;
            }
        } else {
            allDownloadsSuccessful = false;
        }

        if(allDownloadsSuccessful) {
            std::cout<<"Full update needed!\n";
            std::cout<<g_installerPath.string()<<"\n";

            json j;
            j["type"] ="requestUpdate";
            g_pendingUpdateMsg = j;
            PostMessage(g_hWnd, WM_NOTIFY_UPDATE, 0, 0);
        } else {
            std::cout<<"Installer download failed. Skipping update prompt.\n";
        }
    }


    // Handle partial updates if shellVersion matches
    if(!needsFullUpdate) {
        for(const auto& key : partialUpdateKeys) {
            if(files.contains(key) && files[key].contains("url") && files[key].contains("checksum")) {
                std::string url = files[key]["url"].get<std::string>();
                std::string expectedChecksum = files[key]["checksum"].get<std::string>();

                // Target path: application's root directory with the same filename as key
                std::filesystem::path localFilePath = std::filesystem::path(exeDir) / std::wstring(key.begin(), key.end());

                std::cout<<"Processing partial update for: " << key << "\n";

                // Compare checksums
                if(std::filesystem::exists(localFilePath)) {
                    if(FileChecksum(localFilePath) == expectedChecksum) {
                        std::cout<<"No update needed for " << key << "\n";
                        continue;
                    }
                }

                // Attempt to download the file
                if(!DownloadFile(url, localFilePath)) {
                    AppendToCrashLog((L"[UPDATER]: Failed to download " + std::wstring(key.begin(), key.end())).c_str());
                } else {
                    std::cout<<"Downloaded " << key << " successfully.\n";
                    if(FileChecksum(localFilePath) != expectedChecksum) {
                        AppendToCrashLog("[UPDATER]: Failed to download. File corrupted: " + localFilePath.string());
                        continue;
                    }
                    // Perform update actions based on the file key
                    if(key == "server.js") {
                        StopNodeServer();
                        StartNodeServer();
                    }
                }
            }
        }
    }
    std::cout<<"[UPDATER]: Update check done! \n";
}

static void RunInstallerAndExit() {
    if(g_installerPath.empty()) {
        AppendToCrashLog(L"[UPDATER]: Installer path not set.");
        return;
    }

    // Use ShellExecute to run the installer
    HINSTANCE result = ShellExecuteW(
        nullptr,
        L"open",
        g_installerPath.c_str(),
        nullptr,
        nullptr,
        SW_HIDE
    );

    // Check if ShellExecute failed
    if ((INT_PTR)result <= 32) {
        AppendToCrashLog(L"[UPDATER]: Failed to start installer.");
    }

    // Clean up and exit the application gracefully
    PostQuitMessage(0);
    exit(0);
}


// -----------------------------------------------------------------------------
// WndProc
// -----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch(message)
    {
    case WM_CREATE: {
        HICON hIconBig = LoadIcon(g_hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
        HICON hIconSmall = LoadIcon(g_hInst, MAKEINTRESOURCE(IDR_MAINFRAME));

        SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

        CreateTrayIcon(hWnd);
        UpdateTheme(g_hWnd);
        break;
    }

    case WM_NOTIFY_UPDATE: {
        if (!g_pendingUpdateMsg.is_null()) {
            SendToJS(g_pendingUpdateMsg);
            g_pendingUpdateMsg = json();
        }
        break;
    }

    case WM_SETTINGCHANGE:
        UpdateTheme(g_hWnd);
        break;

    case WM_TRAYICON:
        if(LOWORD(lParam)==WM_RBUTTONUP) ShowTrayMenu(hWnd);
        if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        }
        break;

    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
        case ID_TRAY_SHOWWINDOW:
            g_showWindow=!g_showWindow;
            ShowWindow(hWnd,g_showWindow?SW_SHOW:SW_HIDE);
            break;
        case ID_TRAY_ALWAYSONTOP:
            g_alwaysOnTop=!g_alwaysOnTop;
            SetWindowPos(hWnd,
                         g_alwaysOnTop?HWND_TOPMOST:HWND_NOTOPMOST,
                         0,0,0,0,
                         SWP_NOMOVE|SWP_NOSIZE);
            break;
        case ID_TRAY_CLOSE_ON_EXIT:
            g_closeOnExit = !g_closeOnExit;
            SaveSettings();
            break;
        case ID_TRAY_USE_DARK_THEME:
            g_useDarkTheme = !g_useDarkTheme;
            SaveSettings();
            UpdateTheme(g_hWnd);
            break;
        case ID_TRAY_PICTURE_IN_PICTURE:
            TogglePictureInPicture(hWnd, !g_isPipMode);
            break;
        case ID_TRAY_PAUSE_FOCUS_LOST:
            g_pauseOnLostFocus = !g_pauseOnLostFocus;
            SaveSettings();
            break;
        case ID_TRAY_PAUSE_MINIMIZED:
            g_pauseOnMinimize = !g_pauseOnMinimize;
            SaveSettings();
            break;
        case ID_TRAY_QUIT:
            if(g_mpv) mpv_command_string(g_mpv,"quit");
            DestroyWindow(hWnd);
            break;
        }
    }
    break;

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
                    SendToJS(j);
                } else {
                    // Handle other media files
                    std::string utf8FilePath = WStringToUtf8(receivedUrl);
                    json j;
                    j["type"] = "OpenFile";
                    j["path"] = utf8FilePath;
                    SendToJS(j);
                }
            } else if (receivedUrl.rfind(L"stremio://", 0) == 0) {
                // Handle stremio:// protocol
                std::string utf8Url = WStringToUtf8(receivedUrl);
                json j;
                j["type"] = "AddonInstall";
                j["path"] = utf8Url;
                SendToJS(j);
            } else if (receivedUrl.rfind(L"magnet:", 0) == 0) {
                std::string utf8Url = WStringToUtf8(receivedUrl);
                json j;
                j["type"] = "OpenTorrent";
                j["magnet"] = utf8Url;
                SendToJS(j);
            } else {
                std::wcout << L"Received URL is neither a valid file nor a stremio:// protocol." << std::endl;
            }
        }
        return 0;
    }

    case WM_CLOSE:
        if (g_closeOnExit) {
            DestroyWindow(hWnd);
        } else {
            ShowWindow(hWnd, SW_HIDE);
            pauseMPV(g_pauseOnMinimize);
            g_showWindow = false;
        }
        return 0;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            pauseMPV(g_pauseOnLostFocus);
        }
    break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            pauseMPV(g_pauseOnMinimize);
        }
        if(g_webviewController){
            RECT rc;GetClientRect(hWnd,&rc);
            g_webviewController->put_Bounds(rc);
        }
        if (g_hSplash)
        {
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);
            SetWindowPos(g_hSplash, nullptr, 0, 0, newWidth, newHeight, SWP_NOZORDER);
        }
        break;

    case WM_MPV_WAKEUP:
        HandleMpvEvents();
        break;

    case WM_DESTROY:
        std::cout<<"WM_DESTROY => stopping mpv + node + tray.\n";
        // release mutex
        if(g_hMutex){CloseHandle(g_hMutex);g_hMutex=nullptr;}
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd,message,wParam,lParam);
    }
    return 0;
}

void ParseCommandLineArgs(int argc, char* argv[]) {
    const std::string webuiPrefix = "--webui-url=";
    const std::string autoupdaterPrefix = "--autoupdater-endpoint=";
    const std::string streamingServerFlag = "--streaming-server-disabled";
    const std::string autoupdaterForceFlag = "--autoupdater-force-full";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.find(webuiPrefix) == 0) {
            std::string urlPart = arg.substr(webuiPrefix.length());
            g_webuiUrl = Utf8ToWstring(urlPart);
            std::cout<<"g_webuiUrl="<<urlPart<<"\n";
        }
        else if (arg.find(autoupdaterPrefix) == 0) {
            g_updateUrl =  arg.substr(autoupdaterPrefix.length());
        }
        else if (arg == streamingServerFlag) {
            g_streamingServer = false;
        }
        else if (arg == autoupdaterForceFlag) {
            g_autoupdaterForceFull = true;
        }
    }
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    SetUnhandledExceptionFilter(ExceptionFilter);
    atexit(Cleanup);

    // Parse command-line arguments
    ParseCommandLineArgs(argc, argv);

    // Single-instance
    if(!CheckSingleInstance(argc, argv)){
        return 0;
    }

    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok) {
        std::cerr << "Failed to initialize GDI+.\n";
        AppendToCrashLog(L"[BOOT]: Failed to initialize GDI+");
        return 1;
    }

    // Load ini settings
    LoadSettings();
    LoadCustomMenuFont();

    g_updaterThread = std::thread(RunAutoUpdaterOnce);
    g_updaterThread.detach();

    g_hInst=GetModuleHandle(nullptr);
    g_darkBrush = CreateSolidBrush(RGB(0, 0, 0));

    WNDCLASSEX wcex;ZeroMemory(&wcex,sizeof(wcex));
    wcex.cbSize=sizeof(WNDCLASSEX);
    wcex.style=CS_HREDRAW|CS_VREDRAW;
    wcex.lpfnWndProc=WndProc;
    wcex.hInstance=g_hInst;
    wcex.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wcex.hbrBackground=g_darkBrush;
    wcex.lpszClassName=szWindowClass;

    if(!RegisterClassEx(&wcex)){
        AppendToCrashLog(L"[BOOT]: RegisterClassEx failed!");
        return 1;
    }

    g_hWnd=CreateWindow(szWindowClass,
                        szTitle,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,CW_USEDEFAULT,
                        1200,900,
                        nullptr,nullptr,
                        g_hInst,nullptr);
    if(!g_hWnd){
        AppendToCrashLog(L"[BOOT]: CreateWindow failed!");
        return 1;
    }

    ShowWindow(g_hWnd,SW_SHOW);
    UpdateWindow(g_hWnd);

    // Show splash screen
    CreateSplashScreen(g_hWnd);

    // mpv init
    if(!InitMPV(g_hWnd)){
        DestroyWindow(g_hWnd);
        return 1;
    }

    // Node
    if(g_streamingServer){
        StartNodeServer();
    }


    // Update Theme
    UpdateTheme(g_hWnd);

    // WebView2
    InitWebView2(g_hWnd);

    SetForegroundWindow(g_hWnd);
    SetFocus(g_hWnd);

    MSG msg;
    while(GetMessage(&msg,nullptr,0,0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(g_darkBrush){
        DeleteObject(g_darkBrush);
        g_darkBrush=nullptr;
    }

    std::cout<<"Exiting...\n";
    return (int)msg.wParam;
}
