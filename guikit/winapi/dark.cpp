
#ifndef WM_UAHDRAWMENU
#define WM_UAHDRAWMENU 0x0091
#endif

#ifndef WM_UAHDRAWMENUITEM
#define WM_UAHDRAWMENUITEM 0x0092
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

typedef struct tagUAHMENU
{
    HMENU hmenu;
    HDC hdc;
    DWORD dwFlags; // no idea what these mean, in my testing it's either 0x00000a00 or sometimes 0x00000a10
} UAHMENU;

typedef union tagUAHMENUITEMMETRICS
{
    // cx appears to be 14 / 0xE less than rcItem's width!
    // cy 0x14 seems stable, i wonder if it is 4 less than rcItem's height which is always 24 atm
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
    DWORD fUpdateMaxWidths : 2; // from kernel symbols, padded to full dword
} UAHMENUPOPUPMETRICS;

typedef struct tagUAHMENUITEM
{
    int iPosition; // 0-based position of menu item in menubar
    UAHMENUITEMMETRICS umim;
    UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

typedef struct UAHDRAWMENUITEM
{
    DRAWITEMSTRUCT dis; // itemID looks uninitialized
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

auto pApplication::initDarkTheme() -> void {
    auto version = getVersionNew();

    if (version < Windows10)
        return;

    if (pOpenThemeData &&
        pRefreshImmersiveColorPolicyState &&
        pShouldAppsUseDarkMode &&
        pAllowDarkModeForWindow &&
        (pAllowDarkModeForApp || pSetPreferredAppMode) &&
        pIsDarkModeAllowedForWindow) {

            pApplication::useDark = true;

            if (pSetPreferredAppMode)
                pSetPreferredAppMode(PreferredAppMode::AllowDark);
            else if (pAllowDarkModeForApp)
                pAllowDarkModeForApp(true);
            
            pRefreshImmersiveColorPolicyState();

            getDarkmodeColors();
    }
}

auto pApplication::getDarkmodeColors() -> void {
    if (!pOpenThemeData)
        return;

    HTHEME hTheme = pOpenThemeData(nullptr, L"ItemsView");

    if (hTheme) {
        GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &darkFG);

        COLORREF color;

        if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
            darkBG = CreateSolidBrush(color);

        darkBGHot = CreateSolidBrush(RGB(50, 50, 50));
        
        CloseThemeData(hTheme);
    }

}
