#include "globals.h"

// Window & instance
TCHAR  szWindowClass[]   = APP_NAME;
TCHAR  szTitle[]         = APP_TITLE;

HINSTANCE g_hInst   = nullptr;
HWND      g_hWnd    = nullptr;
HBRUSH    g_darkBrush = nullptr;
HANDLE    g_hMutex  = nullptr;
HHOOK     g_hMouseHook = nullptr;

std::wstring g_webuiUrl = L"https://zaarrg.github.io/stremio-web-shell-fixes/";
std::string  g_updateUrl= "https://raw.githubusercontent.com/Zaarrg/stremio-desktop-v5/refs/heads/webview-windows/version/version.json";

// Command-line args
bool g_streamingServer      = true;
bool g_autoupdaterForceFull = false;

// mpv
mpv_handle* g_mpv = nullptr;
std::set<std::string> g_observedProps;
bool g_initialSet = false;
std::string g_initialVO = "gpu-next";
int g_currentVolume = 50;

// Node
std::atomic_bool g_nodeRunning = false;
std::thread      g_nodeThread;
HANDLE           g_nodeProcess = nullptr;
HANDLE           g_nodeOutPipe = nullptr;
HANDLE           g_nodeInPipe  = nullptr;

// WebView2
wil::com_ptr<ICoreWebView2Controller4> g_webviewController;
wil::com_ptr<ICoreWebView2Profile8>    g_webviewProfile;
wil::com_ptr<ICoreWebView2_21>         g_webview;

// Tray
std::vector<MenuItem> g_menuItems;
NOTIFYICONDATA  g_nid        = {0};
bool            g_showWindow = true;
bool            g_alwaysOnTop= false;
bool            g_isFullscreen = false;
bool            g_closeOnExit = false;
bool            g_useDarkTheme = false;
bool            g_isPipMode = false;
int             g_thumbFastHeight = 0;
int             g_hoverIndex = -1;
HFONT           g_hMenuFont = nullptr;
HANDLE          g_serverJob  = nullptr;
HWND            g_trayHwnd   = nullptr;

// Ini Settings
bool g_pauseOnMinimize   = true;
bool g_pauseOnLostFocus  = false;
bool g_allowZoom         = false;

// Tray sizes
int g_tray_itemH = 31;
int g_tray_sepH  = 8;
int g_tray_w     = 200;

// Splash
HWND       g_hSplash      = nullptr;
HBITMAP    g_hSplashImage = nullptr;
float      g_splashOpacity= 1.0f;
int        g_pulseDirection = -1;
ULONG_PTR  g_gdiplusToken = 0;

// Pending messages
std::vector<nlohmann::json> g_outboundMessages;
std::wstring g_launchProtocol;
std::atomic<bool>  g_isAppReady   = false;
std::atomic<bool>  g_waitStarted(false);

// Updater
std::atomic_bool       g_updaterRunning = false;
std::filesystem::path  g_installerPath;
std::thread            g_updaterThread;
const char* public_key_pem = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoXoJRQ81xOT3Gx6+hsWM
ZiD4PwtLdxxNhEdL/iK0yp6AdO/L0kcSHk9YCPPx0XPK9sssjSV5vCbNE/2IJxnh
/mV+3GAMmXgMvTL+DZgrHafnxe1K50M+8Z2z+uM5YC9XDLppgnC6OrUjwRqNHrKI
T1vcgKf16e/TdKj8xlgadoHBECjv6dr87nbHW115bw8PVn2tSk/zC+QdUud+p6KV
zA6+FT9ZpHJvdS3R0V0l7snr2cwapXF6J36aLGjJ7UviRFVWEEsQaKtAAtTTBzdD
4B9FJ2IJb/ifdnVzeuNTDYApCSE1F89XFWN9FoDyw7Jkk+7u4rsKjpcnCDTd9ziG
kwIDAQAB
-----END PUBLIC KEY-----)";

// ThumbFast
std::atomic<bool> g_ignoreHover(false);
std::chrono::steady_clock::time_point g_ignoreUntil;
