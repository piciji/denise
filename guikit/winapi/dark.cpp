
#define DARK_FG_COL         RGB(0xe0, 0xe0, 0xe0)
#define DARK_BG_COL         RGB(0x19, 0x19, 0x19)
#define DARK_EDGE_COL       RGB(0x9b, 0x9b, 0x9b)
#define DARK_DISABLE_COL    RGB(0x80, 0x80, 0x80)
#define DARK_BG_SOFTER_COL  RGB(0x38, 0x38, 0x38)

#ifndef WM_UAHDRAWMENU
#define WM_UAHDRAWMENU 0x0091
#endif

#ifndef WM_UAHDRAWMENUITEM
#define WM_UAHDRAWMENUITEM 0x0092
#endif

#ifdef __GNUC__
#define WINAPI_LAMBDA WINAPI
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#else
#define WINAPI_LAMBDA
#endif

typedef struct tagUAHMENU
{
    HMENU hmenu;
    HDC hdc;
    DWORD dwFlags;
} UAHMENU;

typedef union tagUAHMENUITEMMETRICS
{
    struct {
        DWORD cx;
        DWORD cy;
    } rgsizeBar[2];
    struct {
        DWORD cx;
        DWORD cy;
    } rgsizePopup[4];
} UAHMENUITEMMETRICS;

typedef struct tagUAHMENUPOPUPMETRICS
{
    DWORD rgcx[4];
    DWORD fUpdateMaxWidths : 2;
} UAHMENUPOPUPMETRICS;

typedef struct tagUAHMENUITEM
{
    int iPosition;
    UAHMENUITEMMETRICS umim;
    UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

typedef struct UAHDRAWMENUITEM
{
    DRAWITEMSTRUCT dis;
    UAHMENU um;
    UAHMENUITEM umi;
} UAHDRAWMENUITEM;

auto pApplication::preferDarkTheme() -> bool {
    uint8_t buffer[4];
    DWORD cbData = 4;
    auto res = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD, nullptr, &buffer, &cbData);

    if (res != ERROR_SUCCESS)
        return false;

    auto i = int(buffer[3] << 24 |
        buffer[2] << 16 |
        buffer[1] << 8 |
        buffer[0]);

    return i != 1;
}

auto pApplication::setDarkMode(Application::DarkMode darkMode) -> void {
    useDark = false;

    if (darkMode == Application::DarkMode::Off)
        return;

    if (darkMode == Application::DarkMode::On)
        initDarkTheme(true);
    else if (preferDarkTheme())
        initDarkTheme(false);
}

auto pApplication::initDarkTheme(bool force) -> void {
    auto version = getVersionNew();
    auto buildNumber = getVersionNew(true);

    if ((version < Windows10) || (buildNumber < 19041))
        return;

    loadThemedFunctions();

    if (pOpenThemeData &&
        pRefreshImmersiveColorPolicyState &&
        pShouldAppsUseDarkMode &&
        pAllowDarkModeForWindow &&
        (pAllowDarkModeForApp || pSetPreferredAppMode) &&
        pIsDarkModeAllowedForWindow) {

            pApplication::useDark = true;

            if (pSetPreferredAppMode)
                pSetPreferredAppMode(force ? PreferredAppMode::ForceDark : PreferredAppMode::AllowDark);
            else if (pAllowDarkModeForApp)
                pAllowDarkModeForApp(true);
            
            pRefreshImmersiveColorPolicyState();          

            darkBGBrush = CreateSolidBrush(DARK_BG_COL);

            darkBGHotBrush = CreateSolidBrush(RGB(0x45, 0x45, 0x45));

            darkEdgeBrush = CreateSolidBrush(DARK_EDGE_COL);

            darkBGSofterBrush = CreateSolidBrush(DARK_BG_SOFTER_COL);

            darkBGTabBrush = CreateSolidBrush(RGB(0x20, 0x20, 0x20));

            darkEdgePen = ::CreatePen(PS_SOLID, 1, RGB(0x64, 0x64, 0x64));

            darkDisabledEdgeBrush = CreateSolidBrush(RGB(0x48, 0x48, 0x48));
    }
}
